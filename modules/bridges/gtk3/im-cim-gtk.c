/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * im-cim-gtk.c
 * This file is part of Cim.
 *
 * Copyright (C) 2023-2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <gtk/gtkimmodule.h>
#include <glib/gi18n.h>
#include <limits.h>
#include "cim.h"
#include "c-candidate.h"

#define CIM_GIC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), cim_gic_get_type (), CimGic))

typedef struct _CimGic      CimGic;
typedef struct _CimGicClass CimGicClass;

struct _CimGic
{
  GtkIMContext  parent_instance;

  CimIcHandle   ic;
  GtkIMContext* simple;
  GdkWindow*    client_window;
  CimSurround   surround;
  GdkRectangle  cursor_pos;
  /* candidate window */
  CCandidate* ccandidate;
};

struct _CimGicClass
{
  GtkIMContextClass parent_class;
};

G_DEFINE_DYNAMIC_TYPE (CimGic, cim_gic, GTK_TYPE_IM_CONTEXT)

static CimCallbacks callbacks;

static gboolean cim_gic_filter_keypress (GtkIMContext* context, GdkEventKey* event)
{
  gboolean retval;
  CimEvent cevent;

  if (event->type == GDK_KEY_PRESS)
    cevent.type = CIM_EVENT_KEY_PRESS;
  else
    cevent.type = CIM_EVENT_KEY_RELEASE;

  cevent.state   = event->state;
  cevent.keyval  = event->keyval;
  cevent.keycode = event->hardware_keycode;

  retval = cim_ic_filter_event (CIM_GIC (context)->ic, &cevent);

  if (!retval)
    return gtk_im_context_filter_keypress (CIM_GIC (context)->simple, event);

  return retval;
}

static void cim_gic_reset (GtkIMContext* context)
{
  cim_ic_reset (CIM_GIC (context)->ic);
  gtk_im_context_reset (CIM_GIC (context)->simple);
}

static void cim_gic_set_client_window (GtkIMContext* context, GdkWindow* window)
{
  CimGic* gic = CIM_GIC (context);

  if (gic->client_window)
  {
    g_object_unref (gic->client_window);
    gic->client_window = NULL;
  }

  if (window)
    gic->client_window = g_object_ref (window);
}

static void cim_gic_get_preedit_string (GtkIMContext*   context,
                                        char**          text,
                                        PangoAttrList** attrs,
                                        int*            cursor_pos)
{
  const CimPreedit* preedit;

  preedit = cim_ic_get_preedit (CIM_GIC (context)->ic);

  if (!preedit)
  {
    if (text)
      *text = g_strdup ("");

    if (cursor_pos)
      *cursor_pos = 0;

    if (attrs)
      *attrs = pango_attr_list_new ();

    return;
  }

  if (text)
    *text = g_strdup (preedit->text);

  if (cursor_pos)
  {
    g_return_if_fail (preedit->cursor_pos <= (uint32_t) INT_MAX);
    *cursor_pos = (int) preedit->cursor_pos;
  }

  if (attrs)
  {
    PangoAttribute* attr;
    char* p1;
    char* p2;

    *attrs = pango_attr_list_new ();

    for (uint32_t i = 0; i < preedit->attrs_len; i++)
    {
      p1 = g_utf8_offset_to_pointer (preedit->text, preedit->attrs[i].pos);
      p2 = g_utf8_offset_to_pointer (preedit->text, preedit->attrs[i].pos +
                                                    preedit->attrs[i].n_chars);
      switch (preedit->attrs[i].type)
      {
        case CIM_TEXT_ATTR_UNDERLINE:
          attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
          break;
        case CIM_TEXT_ATTR_HIGHLIGHT:
          attr = pango_attr_background_new (0, 0xffff, 0);
          attr->start_index = (guint) (p1 - preedit->text);
          attr->end_index   = (guint) (p2 - preedit->text);
          pango_attr_list_insert (*attrs, attr);

          attr = pango_attr_foreground_new (0, 0, 0);
          break;
        default:
          attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
          break;
      }

      attr->start_index = (guint) (p1 - preedit->text);
      attr->end_index   = (guint) (p2 - preedit->text);
      pango_attr_list_insert (*attrs, attr);
    }
  }
}

static void cim_gic_focus_in (GtkIMContext* context)
{
  cim_ic_focus_in (CIM_GIC (context)->ic);
}

static void cim_gic_focus_out (GtkIMContext* context)
{
  cim_ic_focus_out (CIM_GIC (context)->ic);
}

static void cim_gic_set_cursor_pos (GtkIMContext* context, GdkRectangle* area)
{
  CimGic* gic = CIM_GIC (context);

  gic->cursor_pos = *area;

  /* If the function below doesn't work for wayland,
     try gtk_widget_translate_coordinates(). */
  if (gic->client_window)
    gdk_window_get_root_coords (gic->client_window,
                                area->x,
                                area->y,
                                &gic->cursor_pos.x,
                                &gic->cursor_pos.y);

  cim_ic_set_cursor_pos (CIM_GIC (context)->ic, (const CimRect*) &gic->cursor_pos);
}

static void cb_preedit_start (CimIcHandle unused, void* gic)
{
  (void) unused;
  g_signal_emit_by_name (gic, "preedit-start");
}

static void cb_preedit_end (CimIcHandle unused, void* gic)
{
  (void) unused;
  g_signal_emit_by_name (gic, "preedit-end");
}

static void cb_preedit_changed (CimIcHandle unused1,
                                const CimPreedit* unused2,
                                void* gic)
{
  (void) unused1;
  (void) unused2;
  g_signal_emit_by_name (gic, "preedit-changed");
}

static void cb_change_page (CCandidate* unused,
                            uint32_t    page_index,
                            CimIcHandle ic)
{
  (void) unused;
  cim_ic_change_candidate_page (ic, page_index);
}

static void cb_activate_item (CCandidate* unused,
                              uint32_t    row,
                              uint32_t    col,
                              CimIcHandle ic)
{
  (void) unused;
  cim_ic_activate_candidate_item (ic, row, col);
}

static void cb_candidate_show (CimIcHandle unused,
                               uint32_t    n_rows,
                               uint32_t    n_cols,
                               bool        show_aux,
                               void*       user_data)
{
  (void) unused;
  CimGic* gic =(CimGic*) user_data;

  if (!gic->ccandidate)
  {
    gic->ccandidate = c_candidate_new ();
    c_candidate_set_callbacks (gic->ccandidate,
      C_CANDIDATE_CB_CHANGE_PAGE,   cb_change_page,   gic->ic,
      C_CANDIDATE_CB_ACTIVATE_ITEM, cb_activate_item, gic->ic, -1);
  }

  c_candidate_show (gic->ccandidate, n_rows, n_cols, show_aux);
  c_candidate_move (gic->ccandidate,
                    gic->cursor_pos.x - gic->cursor_pos.width,
                    gic->cursor_pos.y + gic->cursor_pos.height);
}

static void cb_candidate_hide (CimIcHandle unused, void* gic)
{
  (void) unused;
  c_candidate_hide (((CimGic*) gic)->ccandidate);
}

static void cb_candidate_changed (CimIcHandle unused,
                                  const CimCandidate* candidate,
                                  void* gic)
{
  (void) unused;
  c_candidate_change (((CimGic*) gic)->ccandidate, candidate);
}

static void cb_candidate_selected (CimIcHandle unused,
                                   const CimSelection* selection,
                                   void* gic)
{
  (void) unused;
  c_candidate_select (((CimGic*) gic)->ccandidate, selection);
}

static void cim_gic_set_use_preedit (GtkIMContext* context,
                                     gboolean      use_preedit)
{
  CimGic* gic = CIM_GIC (context);

  if (use_preedit)
  {
    callbacks.preedit_start   = cb_preedit_start;
    callbacks.preedit_end     = cb_preedit_end;
    callbacks.preedit_changed = cb_preedit_changed;
  }
  else
  {
    callbacks.preedit_start   = nullptr;
    callbacks.preedit_end     = nullptr;
    callbacks.preedit_changed = nullptr;
  }

  cim_ic_set_callbacks (gic->ic, &callbacks, gic);
}

static void cim_gic_set_surround (GtkIMContext* context,
                                  const char*   text,
                                  int           len,
                                  int           cursor_index_in_bytes)
{
  CimGic* gic = CIM_GIC (context);
  glong    cursor_pos;

  g_return_if_fail (text != NULL);
  g_return_if_fail (len >= 0);
  g_return_if_fail (cursor_index_in_bytes >= 0);

  cursor_pos = g_utf8_strlen (text, cursor_index_in_bytes);

  g_return_if_fail (cursor_pos >= 0);
  g_return_if_fail ((uint64_t) cursor_pos <= UINT32_MAX);

  gic->surround.text       = text;
  gic->surround.len        = (uint32_t) len;
  gic->surround.cursor_pos = (uint32_t) cursor_pos;
  gic->surround.anchor_pos = (uint32_t) cursor_pos;
}

GtkIMContext* cim_gic_new ()
{
  return g_object_new (cim_gic_get_type (), NULL);
}

static void cb_commit (CimIcHandle unused, const char* text, void* gic)
{
  (void) unused;
  g_signal_emit_by_name (gic, "commit", text);
}

static gboolean cb_delete_surround (CimIcHandle unused,
                                    int   offset,
                                    int   n_chars,
                                    void* gic)
{
  (void) unused;
  gboolean retval;
  g_signal_emit_by_name (((CimGic*) gic), "delete-surrounding",
                         offset, n_chars, &retval);
  return retval;
}

static bool cb_delete_surround2 (CimIcHandle unused,
                                 int32_t     offset,
                                 uint32_t    n_chars,
                                 void* gic)
{
  (void) unused;
  gboolean retval;
  g_signal_emit_by_name (((CimGic*) gic), "delete-surrounding",
                         offset, n_chars, &retval);
  return retval;
}

static gboolean cb_retrieve_surround (CimIcHandle unused, CimGic* gic)
{
  (void) unused;
  gboolean retval;
  g_signal_emit_by_name (gic, "retrieve-surrounding", &retval);

  return retval;
}

static const CimSurround* cb_get_surround (CimIcHandle unused, void* user_data)
{
  (void) unused;
  CimGic* gic = (CimGic*) user_data;
  gboolean retval;
  g_signal_emit_by_name (gic, "retrieve-surrounding", &retval);

  if (retval)
    return &gic->surround;

  return NULL;
}

static void cim_gic_init (CimGic* gic)
{
  gic->ic = cim_ic_create ();
  gic->simple = gtk_im_context_simple_new ();

  g_signal_connect (gic->simple, "commit", G_CALLBACK (cb_commit), gic);
  g_signal_connect (gic->simple, "delete-surrounding",
                    G_CALLBACK (cb_delete_surround), gic);
  g_signal_connect (gic->simple, "preedit-changed",
                    G_CALLBACK (cb_preedit_changed), gic);
  g_signal_connect (gic->simple, "preedit-end",
                    G_CALLBACK (cb_preedit_end), gic);
  g_signal_connect (gic->simple, "preedit-start",
                    G_CALLBACK (cb_preedit_start), gic);
  g_signal_connect (gic->simple, "retrieve-surrounding",
                    G_CALLBACK (cb_retrieve_surround), gic);

  callbacks.preedit_start      = cb_preedit_start;
  callbacks.preedit_end        = cb_preedit_end;
  callbacks.preedit_changed    = cb_preedit_changed;
  callbacks.commit             = cb_commit;
  callbacks.get_surround       = cb_get_surround;
  callbacks.delete_surround    = cb_delete_surround2;
  callbacks.candidate_show     = cb_candidate_show;
  callbacks.candidate_hide     = cb_candidate_hide;
  callbacks.candidate_changed  = cb_candidate_changed;
  callbacks.candidate_selected = cb_candidate_selected;

  cim_ic_set_callbacks (gic->ic, &callbacks, gic);
}

static void cim_gic_finalize (GObject* object)
{
  CimGic* gic = CIM_GIC (object);

  cim_ic_destroy (gic->ic);
  g_object_unref (gic->simple);

  if (gic->client_window)
    g_object_unref (gic->client_window);

  if (gic->ccandidate)
    c_candidate_free (gic->ccandidate);

  G_OBJECT_CLASS (cim_gic_parent_class)->finalize (object);
}

static void cim_gic_class_init (CimGicClass* class)
{
  GObjectClass* object_class  = G_OBJECT_CLASS (class);
  GtkIMContextClass* ic_class = GTK_IM_CONTEXT_CLASS (class);

  ic_class->set_client_window   = cim_gic_set_client_window;
  ic_class->get_preedit_string  = cim_gic_get_preedit_string;
  ic_class->filter_keypress     = cim_gic_filter_keypress;
  ic_class->focus_in            = cim_gic_focus_in;
  ic_class->focus_out           = cim_gic_focus_out;
  ic_class->reset               = cim_gic_reset;
  ic_class->set_cursor_location = cim_gic_set_cursor_pos;
  ic_class->set_use_preedit     = cim_gic_set_use_preedit;
  ic_class->set_surrounding     = cim_gic_set_surround;

  object_class->finalize = cim_gic_finalize;
}

static void cim_gic_class_finalize (CimGicClass* class)
{
  (void) class;
}

static const GtkIMContextInfo cim_info = {
  "cim",
  N_("Common Input Method"),
  "cim",
  CIM_LOCALE_DIR,
  "*"
};

static const GtkIMContextInfo* info_list[] = { &cim_info };

G_MODULE_EXPORT void im_module_init (GTypeModule* type_module)
{
  cim_gic_register_type (type_module);
}

G_MODULE_EXPORT void im_module_exit (void)
{
}

G_MODULE_EXPORT void im_module_list (const GtkIMContextInfo*** contexts,
                                     int* n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

G_MODULE_EXPORT GtkIMContext* im_module_create (const char* context_id)
{
  if (!g_strcmp0 (context_id, "cim"))
    return cim_gic_new ();

  return NULL;
}
