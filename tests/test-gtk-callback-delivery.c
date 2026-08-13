#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

void im_module_init (GTypeModule* type_module);
GtkIMContext* im_module_create (const char* context_id);
gboolean cim_gic_test_filter_event (GtkIMContext* context, uint32_t keyval);
gboolean cim_gic_test_wait_for_sync_pending (GtkIMContext* context);
void cim_gic_test_invoke_commit (GtkIMContext* context, const char* text);

typedef struct _TestTypeModule TestTypeModule;
typedef struct _TestTypeModuleClass TestTypeModuleClass;

struct _TestTypeModule
{
  GTypeModule parent_instance;
};

struct _TestTypeModuleClass
{
  GTypeModuleClass parent_class;
};

G_DEFINE_TYPE (TestTypeModule, test_type_module, G_TYPE_TYPE_MODULE)

static gboolean
test_type_module_load (GTypeModule* module)
{
  (void) module;
  return TRUE;
}

typedef struct
{
  GThread* owner_thread;
  guint retrieve_count;
  guint delete_count;
  gboolean retrieve_on_owner;
  gboolean delete_on_owner;
  gint delete_offset;
  gint delete_n_chars;
} SurroundState;

static void
test_type_module_unload (GTypeModule* module)
{
  (void) module;
}

static void
test_type_module_class_init (TestTypeModuleClass* klass)
{
  GTypeModuleClass* module_class = G_TYPE_MODULE_CLASS (klass);

  module_class->load = test_type_module_load;
  module_class->unload = test_type_module_unload;
}

static void
test_type_module_init (TestTypeModule* module)
{
  (void) module;
}

typedef struct
{
  GThread* owner_thread;
  guint commit_count;
  gboolean delivered_on_owner;
  char text[64];
} CommitState;

typedef struct
{
  GThread* owner_thread;
  guint changed_count;
  gboolean delivered_on_owner;
  gboolean payload_valid;
} PreeditState;

typedef struct
{
  GtkIMContext* context;
  char* text;
} WorkerData;

static void
on_commit (GtkIMContext* context, const char* text, gpointer user_data)
{
  CommitState* state = user_data;

  (void) context;

  state->commit_count++;
  state->delivered_on_owner = g_thread_self () == state->owner_thread;
  g_strlcpy (state->text, text, sizeof (state->text));
}

static gboolean
has_attribute_range (PangoAttrList* attrs,
                     PangoAttrType type,
                     guint start_index,
                     guint end_index)
{
  PangoAttrIterator* iterator = pango_attr_list_get_iterator (attrs);
  gboolean found = FALSE;

  do
  {
    PangoAttribute* attr = pango_attr_iterator_get (iterator, type);

    if (attr &&
        attr->start_index == start_index &&
        attr->end_index == end_index)
    {
      found = TRUE;
      break;
    }
  } while (pango_attr_iterator_next (iterator));

  pango_attr_iterator_destroy (iterator);
  return found;
}

static void
on_preedit_changed (GtkIMContext* context, gpointer user_data)
{
  static const char expected[] = "A\xF0\x9F\x98\x80" "B";
  PreeditState* state = user_data;
  char* text = NULL;
  PangoAttrList* attrs = NULL;
  int cursor_pos = -1;

  state->changed_count++;
  state->delivered_on_owner = g_thread_self () == state->owner_thread;
  gtk_im_context_get_preedit_string
    (context, &text, &attrs, &cursor_pos);

  state->payload_valid =
    text != NULL &&
    attrs != NULL &&
    strcmp (text, expected) == 0 &&
    cursor_pos == 2 &&
    has_attribute_range (attrs, PANGO_ATTR_BACKGROUND, 0, 1) &&
    has_attribute_range (attrs, PANGO_ATTR_FOREGROUND, 0, 1) &&
    has_attribute_range (attrs, PANGO_ATTR_UNDERLINE, 1, 5);

  g_free (text);
  if (attrs)
    pango_attr_list_unref (attrs);
}

static gpointer
invoke_commit_from_worker (gpointer user_data)
{
  WorkerData* data = user_data;
  size_t text_length = strlen (data->text);

  cim_gic_test_invoke_commit (data->context, data->text);
  memset (data->text, 'x', text_length);
  return NULL;
}

static gboolean
on_retrieve_surrounding (GtkIMContext* context, gpointer user_data)
{
  static const char text[] = "A\xF0\x9F\x98\x80" "B";
  SurroundState* state = user_data;

  state->retrieve_count++;
  state->retrieve_on_owner = g_thread_self () == state->owner_thread;
  gtk_im_context_set_surrounding (context, text, -1, 5);
  return TRUE;
}

static gboolean
on_delete_surrounding (GtkIMContext* context,
                       gint offset,
                       gint n_chars,
                       gpointer user_data)
{
  SurroundState* state = user_data;

  (void) context;

  state->delete_count++;
  state->delete_on_owner = g_thread_self () == state->owner_thread;
  state->delete_offset = offset;
  state->delete_n_chars = n_chars;
  return TRUE;
}

static int
set_plugin (const char* plugin_name)
{
  const char* plugin_dir = g_getenv ("CIM_TEST_PLUGIN_DIR");
  char* plugin_path;

  if (!plugin_dir || plugin_dir[0] == '\0' ||
      !plugin_name || plugin_name[0] == '\0')
  {
    fprintf (stderr, "GTK callback test plugin is not configured\n");
    return 1;
  }

  plugin_path = g_build_filename (plugin_dir, plugin_name, NULL);

  if (!g_setenv ("CIM_PLUGIN", plugin_path, TRUE))
  {
    fprintf (stderr, "could not configure CIM_PLUGIN\n");
    g_free (plugin_path);
    return 1;
  }

  g_free (plugin_path);
  return 0;
}

static int
run_delivery_case (GMainContext* main_context)
{
  static const char expected[] = "cross-thread-owned-payload";
  CommitState state = {
    .owner_thread = g_thread_self ()
  };
  GtkIMContext* context;
  char payload[] = "cross-thread-owned-payload";
  WorkerData worker_data;
  GThread* worker;

  context = im_module_create ("cim");
  if (!context)
  {
    fprintf (stderr, "could not create GTK Cim context\n");
    return 1;
  }

  g_signal_connect (context, "commit", G_CALLBACK (on_commit), &state);

  worker_data.context = context;
  worker_data.text = payload;
  worker = g_thread_new
    ("cim-gtk-callback", invoke_commit_from_worker, &worker_data);
  g_thread_join (worker);

  while (state.commit_count == 0 &&
         g_main_context_pending (main_context))
    g_main_context_iteration (main_context, FALSE);

  if (state.commit_count != 1 ||
      !state.delivered_on_owner ||
      strcmp (state.text, expected) != 0)
  {
    fprintf (stderr, "GTK cross-thread callback delivery failed\n");
    g_object_unref (context);
    return 1;
  }

  g_object_unref (context);
  return 0;
}

static int
run_pending_teardown_case (GMainContext* main_context)
{
  CommitState state = {
    .owner_thread = g_thread_self ()
  };
  GtkIMContext* context;
  char payload[] = "must-not-be-delivered";
  WorkerData worker_data;
  GThread* worker;

  context = im_module_create ("cim");
  if (!context)
  {
    fprintf (stderr, "could not create GTK Cim context\n");
    return 1;
  }

  g_signal_connect (context, "commit", G_CALLBACK (on_commit), &state);

  worker_data.context = context;
  worker_data.text = payload;
  worker = g_thread_new
    ("cim-gtk-pending", invoke_commit_from_worker, &worker_data);
  g_thread_join (worker);

  if (!g_main_context_pending (main_context))
  {
    fprintf (stderr, "GTK callback delivery was not left pending\n");
    g_object_unref (context);
    return 1;
  }

  g_object_unref (context);

  while (g_main_context_pending (main_context))
    g_main_context_iteration (main_context, FALSE);

  if (state.commit_count != 0)
  {
    fprintf (stderr, "GTK pending callback survived teardown\n");
    return 1;
  }

  return 0;
}

static int
run_get_surround_case (GMainContext* main_context)
{
  CommitState commit_state = {
    .owner_thread = g_thread_self ()
  };
  SurroundState surround_state = {
    .owner_thread = g_thread_self ()
  };
  GtkIMContext* context = im_module_create ("cim");

  if (!context)
  {
    fprintf (stderr, "could not create GTK Cim context\n");
    return 1;
  }

  g_signal_connect
    (context, "commit", G_CALLBACK (on_commit), &commit_state);
  g_signal_connect
    (context,
     "retrieve-surrounding",
     G_CALLBACK (on_retrieve_surrounding),
     &surround_state);

  if (!cim_gic_test_filter_event (context, 'G') ||
      !cim_gic_test_wait_for_sync_pending (context))
  {
    fprintf (stderr, "GTK get-surround callback was not queued\n");
    g_object_unref (context);
    return 1;
  }

  while (commit_state.commit_count == 0)
    g_main_context_iteration (main_context, TRUE);

  if (commit_state.commit_count != 1 ||
      !commit_state.delivered_on_owner ||
      strcmp (commit_state.text, "get-surround-ok") != 0 ||
      surround_state.retrieve_count != 1 ||
      !surround_state.retrieve_on_owner)
  {
    fprintf (stderr, "GTK synchronous get-surround callback failed\n");
    g_object_unref (context);
    return 1;
  }

  g_object_unref (context);
  return 0;
}

static int
run_delete_surround_case (GMainContext* main_context)
{
  CommitState commit_state = {
    .owner_thread = g_thread_self ()
  };
  SurroundState surround_state = {
    .owner_thread = g_thread_self ()
  };
  GtkIMContext* context = im_module_create ("cim");

  if (!context)
  {
    fprintf (stderr, "could not create GTK Cim context\n");
    return 1;
  }

  g_signal_connect
    (context, "commit", G_CALLBACK (on_commit), &commit_state);
  g_signal_connect
    (context,
     "delete-surrounding",
     G_CALLBACK (on_delete_surrounding),
     &surround_state);

  if (!cim_gic_test_filter_event (context, 'D') ||
      !cim_gic_test_wait_for_sync_pending (context))
  {
    fprintf (stderr, "GTK delete-surround callback was not queued\n");
    g_object_unref (context);
    return 1;
  }

  while (commit_state.commit_count == 0)
    g_main_context_iteration (main_context, TRUE);

  if (commit_state.commit_count != 1 ||
      !commit_state.delivered_on_owner ||
      strcmp (commit_state.text, "delete-surround-ok") != 0 ||
      surround_state.delete_count != 1 ||
      !surround_state.delete_on_owner ||
      surround_state.delete_offset != -1 ||
      surround_state.delete_n_chars != 1)
  {
    fprintf (stderr, "GTK synchronous delete-surround callback failed\n");
    g_object_unref (context);
    return 1;
  }

  g_object_unref (context);
  return 0;
}

static int
run_sync_teardown_case (GMainContext* main_context)
{
  GtkIMContext* context = im_module_create ("cim");

  if (!context)
  {
    fprintf (stderr, "could not create GTK Cim context\n");
    return 1;
  }

  if (!cim_gic_test_filter_event (context, 'T') ||
      !cim_gic_test_wait_for_sync_pending (context))
  {
    fprintf (stderr, "GTK teardown callback was not left pending\n");
    g_object_unref (context);
    return 1;
  }

  g_object_unref (context);

  while (g_main_context_pending (main_context))
    g_main_context_iteration (main_context, FALSE);

  return 0;
}

static int
run_multiple_preedit_context_case (GMainContext* main_context)
{
  CommitState disabled_commit = {
    .owner_thread = g_thread_self ()
  };
  CommitState enabled_commit = {
    .owner_thread = g_thread_self ()
  };
  PreeditState disabled_preedit = {
    .owner_thread = g_thread_self ()
  };
  PreeditState enabled_preedit = {
    .owner_thread = g_thread_self ()
  };
  GtkIMContext* disabled = im_module_create ("cim");
  GtkIMContext* enabled = im_module_create ("cim");
  int retval = 1;

  if (!disabled || !enabled)
  {
    fprintf (stderr, "could not create simultaneous GTK contexts\n");
    goto Exit;
  }

  g_signal_connect
    (disabled, "commit", G_CALLBACK (on_commit), &disabled_commit);
  g_signal_connect
    (disabled,
     "preedit-changed",
     G_CALLBACK (on_preedit_changed),
     &disabled_preedit);
  g_signal_connect
    (enabled, "commit", G_CALLBACK (on_commit), &enabled_commit);
  g_signal_connect
    (enabled,
     "preedit-changed",
     G_CALLBACK (on_preedit_changed),
     &enabled_preedit);

  gtk_im_context_set_use_preedit (disabled, FALSE);
  gtk_im_context_set_use_preedit (enabled, TRUE);

  if (!cim_gic_test_filter_event (disabled, 'P') ||
      !cim_gic_test_filter_event (enabled, 'P'))
  {
    fprintf (stderr, "GTK simultaneous preedit callbacks did not start\n");
    goto Exit;
  }

  while (disabled_commit.commit_count == 0 ||
         enabled_commit.commit_count == 0)
    g_main_context_iteration (main_context, TRUE);

  while (g_main_context_pending (main_context))
    g_main_context_iteration (main_context, FALSE);

  if (disabled_commit.commit_count != 1 ||
      !disabled_commit.delivered_on_owner ||
      strcmp (disabled_commit.text, "preedit-sent") != 0 ||
      disabled_preedit.changed_count != 0 ||
      enabled_commit.commit_count != 1 ||
      !enabled_commit.delivered_on_owner ||
      strcmp (enabled_commit.text, "preedit-sent") != 0 ||
      enabled_preedit.changed_count != 1 ||
      !enabled_preedit.delivered_on_owner ||
      !enabled_preedit.payload_valid)
  {
    fprintf (stderr, "GTK simultaneous preedit contexts failed\n");
    goto Exit;
  }

  retval = 0;

Exit:
  if (enabled)
    g_object_unref (enabled);
  if (disabled)
    g_object_unref (disabled);
  return retval;
}

static int
run_creation_failure_case (void)
{
  static const char surrounding[] = "fallback-surrounding";
  GdkRectangle cursor_area = {
    .x = 10,
    .y = 20,
    .width = 30,
    .height = 40
  };
  GtkIMContext* context = im_module_create ("cim");
  PangoAttrList* attrs = NULL;
  char* preedit_text = NULL;
  int cursor_pos = -1;

  if (!context)
  {
    fprintf (stderr, "GTK create-failure fallback was not created\n");
    return 1;
  }

  gtk_im_context_focus_in (context);
  gtk_im_context_set_use_preedit (context, TRUE);
  gtk_im_context_set_cursor_location (context, &cursor_area);
  gtk_im_context_set_surrounding (context, surrounding, -1, 8);
  gtk_im_context_reset (context);
  gtk_im_context_get_preedit_string
    (context, &preedit_text, &attrs, &cursor_pos);

  if (!preedit_text || !attrs || preedit_text[0] != '\0' || cursor_pos != 0)
  {
    fprintf (stderr, "GTK create-failure fallback state is invalid\n");
    g_free (preedit_text);
    if (attrs)
      pango_attr_list_unref (attrs);
    g_object_unref (context);
    return 1;
  }

  g_free (preedit_text);
  pango_attr_list_unref (attrs);
  gtk_im_context_focus_out (context);
  g_object_unref (context);
  return 0;
}

int
main (void)
{
  GMainContext* main_context;
  TestTypeModule* module;
  int retval = 1;

  if (set_plugin ("im-dummy.so") != 0)
    return 1;

  main_context = g_main_context_new ();
  g_main_context_push_thread_default (main_context);

  module = g_object_new (test_type_module_get_type (), NULL);
  if (!g_type_module_use (G_TYPE_MODULE (module)))
  {
    fprintf (stderr, "could not activate GTK test type module\n");
    goto Exit;
  }

  im_module_init (G_TYPE_MODULE (module));

  if (run_delivery_case (main_context) != 0 ||
      run_pending_teardown_case (main_context) != 0)
    goto Unuse_Module;

  if (set_plugin ("im-bridge-callback.so") != 0 ||
      run_get_surround_case (main_context) != 0 ||
      run_delete_surround_case (main_context) != 0 ||
      run_multiple_preedit_context_case (main_context) != 0 ||
      run_sync_teardown_case (main_context) != 0)
    goto Unuse_Module;

  if (set_plugin ("im-create-fail.so") != 0 ||
      run_creation_failure_case () != 0)
    goto Unuse_Module;

  puts ("GTK callback delivery tests passed");
  retval = 0;

Unuse_Module:
  g_type_module_unuse (G_TYPE_MODULE (module));

Exit:
  g_object_unref (module);
  g_main_context_pop_thread_default (main_context);
  g_main_context_unref (main_context);
  g_unsetenv ("CIM_PLUGIN");
  return retval;
}
