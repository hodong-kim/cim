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
#include <string.h>
#include "cim.h"
#include "c-candidate.h"
#include "c-candidate-data.h"
#include "c-preedit.h"

#define CIM_GIC(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), cim_gic_get_type (), CimGic))

typedef struct _CimGic                CimGic;
typedef struct _CimGicClass           CimGicClass;
typedef struct _CimGicCallbackContext CimGicCallbackContext;
typedef struct _CimGicSyncRequest     CimGicSyncRequest;

struct _CimGic
{
  GtkIMContext  parent_instance;

  CimIcHandle   ic;
  CimCallbacks  callbacks;
  CimGicCallbackContext* callback_context;
  GtkIMContext* simple;
  GdkWindow*    client_window;
  char*         surround_text;
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

typedef enum
{
  CIM_GIC_DELIVERY_PREEDIT_START,
  CIM_GIC_DELIVERY_PREEDIT_END,
  CIM_GIC_DELIVERY_PREEDIT_CHANGED,
  CIM_GIC_DELIVERY_COMMIT,
  CIM_GIC_DELIVERY_CANDIDATE_SHOW,
  CIM_GIC_DELIVERY_CANDIDATE_HIDE,
  CIM_GIC_DELIVERY_CANDIDATE_CHANGED,
  CIM_GIC_DELIVERY_CANDIDATE_SELECTED
} CimGicDeliveryType;

typedef struct
{
  CimCandidate candidate;
  CimItem* items;
  gsize n_items;
} CimCandidateSnapshot;

typedef struct
{
  CimGicDeliveryType type;
  union
  {
    struct
    {
      char* text;
    } commit;
    struct
    {
      uint32_t n_rows;
      uint32_t n_cols;
      bool show_aux;
    } candidate_show;
    CimCandidateSnapshot candidate;
    CimSelection selection;
  } payload;
} CimGicDelivery;

typedef enum
{
  CIM_GIC_SYNC_GET_SURROUND,
  CIM_GIC_SYNC_DELETE_SURROUND
} CimGicSyncType;

struct _CimGicCallbackContext
{
  gint ref_count;
  gint closing;
  gint use_preedit;
  GMainContext* main_context;
  GThread* owner_thread;
  GWeakRef owner;
  GMutex queue_mutex;
  GQueue deliveries;
  gboolean source_scheduled;
  GMutex sync_mutex;
  GList* sync_requests;
#ifdef CIM_BRIDGE_TEST
  guint test_sync_ready_count;
  GCond test_sync_condition;
#endif
};

struct _CimGicSyncRequest
{
  gint ref_count;
  CimGicCallbackContext* context;
  CimGicSyncType type;
  int32_t offset;
  uint32_t n_chars;
  GMutex mutex;
  GCond condition;
  gboolean completed;
  gboolean result;
  GSource* source;
};

static void cb_change_page (CCandidate* ccandidate,
                            uint32_t page_index,
                            CimIcHandle ic);
static void cb_activate_item (CCandidate* ccandidate,
                              uint32_t row,
                              uint32_t col,
                              CimIcHandle ic);
static const CimSurround* cim_gic_get_surround_on_owner (CimGic* gic);
static bool cim_gic_delete_surround_on_owner (CimGic* gic,
                                               int32_t offset,
                                               uint32_t n_chars);
static void cim_gic_delivery_free (gpointer data);

static CimGicCallbackContext*
cim_gic_callback_context_ref (CimGicCallbackContext* context)
{
  g_atomic_int_inc (&context->ref_count);
  return context;
}

static void
cim_gic_callback_context_unref (CimGicCallbackContext* context)
{
  if (!g_atomic_int_dec_and_test (&context->ref_count))
    return;

  g_mutex_lock (&context->queue_mutex);

  while (!g_queue_is_empty (&context->deliveries))
    cim_gic_delivery_free (g_queue_pop_head (&context->deliveries));

  context->source_scheduled = FALSE;
  g_mutex_unlock (&context->queue_mutex);
  g_mutex_clear (&context->queue_mutex);

  if (context->sync_requests)
    g_error ("Cim GTK bridge destroyed with synchronous callbacks active");

#ifdef CIM_BRIDGE_TEST
  g_cond_clear (&context->test_sync_condition);
#endif
  g_mutex_clear (&context->sync_mutex);
  g_weak_ref_clear (&context->owner);
  g_thread_unref (context->owner_thread);
  g_main_context_unref (context->main_context);
  g_free (context);
}

static CimGicCallbackContext*
cim_gic_callback_context_new (CimGic* gic)
{
  CimGicCallbackContext* context = g_new0 (CimGicCallbackContext, 1);

  context->ref_count = 1;
  context->use_preedit = TRUE;
  context->main_context = g_main_context_ref_thread_default ();
  context->owner_thread = g_thread_ref (g_thread_self ());
  g_mutex_init (&context->queue_mutex);
  g_queue_init (&context->deliveries);
  g_mutex_init (&context->sync_mutex);
#ifdef CIM_BRIDGE_TEST
  g_cond_init (&context->test_sync_condition);
#endif

  if (!context->main_context)
    g_error ("Cim GTK bridge has no owning main context");

  g_weak_ref_init (&context->owner, G_OBJECT (gic));
  return context;
}

static void
cim_candidate_snapshot_clear (CimCandidateSnapshot* snapshot)
{
  if (!snapshot)
    return;

  for (gsize i = 0; i < snapshot->n_items; i++)
    g_free (snapshot->items[i].data);

  g_free (snapshot->items);
  g_free (snapshot->candidate.aux_text);
  memset (snapshot, 0, sizeof (*snapshot));
}

static void
cim_candidate_snapshot_copy (CimCandidateSnapshot* snapshot,
                             const CimCandidate* candidate)
{
  gsize n_items;
  const char* validation_error;

  if (!snapshot)
    g_error ("Cim GTK bridge candidate snapshot is NULL");

  validation_error = c_candidate_validation_error (candidate);
  if (validation_error)
    g_error ("Cim GTK bridge received invalid candidate data: %s",
             validation_error);

  memset (snapshot, 0, sizeof (*snapshot));

  snapshot->candidate.page_index = candidate->page_index;
  snapshot->candidate.n_pages = candidate->n_pages;
  snapshot->candidate.n_rows = candidate->n_rows;
  snapshot->candidate.n_cols = candidate->n_cols;
  snapshot->candidate.aux_cursor_pos = candidate->aux_cursor_pos;
  snapshot->candidate.padding = candidate->padding;

  if (candidate->aux_text)
    snapshot->candidate.aux_text = g_strdup (candidate->aux_text);

  if (candidate->n_pages == 0)
    return;

  n_items = (gsize) candidate->n_rows * candidate->n_cols;
  snapshot->n_items = n_items;

  if (n_items == 0)
  {
    snapshot->items = g_new0 (CimItem, 1);
    snapshot->candidate.table = snapshot->items;
    return;
  }

  snapshot->items = g_new0 (CimItem, n_items);
  snapshot->candidate.table = snapshot->items;

  for (gsize i = 0; i < n_items; i++)
  {
    const CimItem* source = &candidate->table[i];
    const char* text;

    text = (const char*) source->data;
    snapshot->items[i].type = source->type;
    snapshot->items[i].padding = source->padding;
    snapshot->items[i].data = g_strdup (text);
  }
}

static void
cim_gic_delivery_free (gpointer data)
{
  CimGicDelivery* delivery = data;

  if (!delivery)
    return;

  switch (delivery->type)
  {
    case CIM_GIC_DELIVERY_COMMIT:
      g_free (delivery->payload.commit.text);
      break;
    case CIM_GIC_DELIVERY_CANDIDATE_CHANGED:
      cim_candidate_snapshot_clear (&delivery->payload.candidate);
      break;
    default:
      break;
  }

  g_free (delivery);
}

static CCandidate*
cim_gic_ensure_candidate (CimGic* gic)
{
  if (gic->ccandidate)
    return gic->ccandidate;

  gic->ccandidate = c_candidate_new ();
  if (!gic->ccandidate)
    return NULL;

  c_candidate_set_callbacks
    (gic->ccandidate,
     C_CANDIDATE_CB_CHANGE_PAGE, cb_change_page, gic->ic,
     C_CANDIDATE_CB_ACTIVATE_ITEM, cb_activate_item, gic->ic,
     -1);

  return gic->ccandidate;
}

static void
cim_gic_deliver_one (CimGicCallbackContext* context,
                     CimGicDelivery* delivery)
{
  GObject* owner;
  CimGic* gic;

  if (g_atomic_int_get (&context->closing))
    return;

  owner = g_weak_ref_get (&context->owner);
  if (!owner)
    return;

  gic = CIM_GIC (owner);

  if (g_atomic_int_get (&context->closing))
  {
    g_object_unref (owner);
    return;
  }

  switch (delivery->type)
  {
    case CIM_GIC_DELIVERY_PREEDIT_START:
      if (g_atomic_int_get (&context->use_preedit))
        g_signal_emit_by_name (gic, "preedit-start");
      break;
    case CIM_GIC_DELIVERY_PREEDIT_END:
      if (g_atomic_int_get (&context->use_preedit))
        g_signal_emit_by_name (gic, "preedit-end");
      break;
    case CIM_GIC_DELIVERY_PREEDIT_CHANGED:
      if (g_atomic_int_get (&context->use_preedit))
        g_signal_emit_by_name (gic, "preedit-changed");
      break;
    case CIM_GIC_DELIVERY_COMMIT:
      g_signal_emit_by_name (gic, "commit", delivery->payload.commit.text);
      break;
    case CIM_GIC_DELIVERY_CANDIDATE_SHOW:
      if (cim_gic_ensure_candidate (gic))
      {
        gint64 x;
        gint64 y;

        c_candidate_show
          (gic->ccandidate,
           delivery->payload.candidate_show.n_rows,
           delivery->payload.candidate_show.n_cols,
           delivery->payload.candidate_show.show_aux);

        x = (gint64) gic->cursor_pos.x - gic->cursor_pos.width;
        y = (gint64) gic->cursor_pos.y + gic->cursor_pos.height;

        if (x < G_MININT || x > G_MAXINT || y < G_MININT || y > G_MAXINT)
          g_error ("Cim GTK candidate position exceeds toolkit limits");

        c_candidate_move (gic->ccandidate, (int) x, (int) y);
      }
      break;
    case CIM_GIC_DELIVERY_CANDIDATE_HIDE:
      if (gic->ccandidate)
        c_candidate_hide (gic->ccandidate);
      break;
    case CIM_GIC_DELIVERY_CANDIDATE_CHANGED:
      if (gic->ccandidate)
        c_candidate_change
          (gic->ccandidate, &delivery->payload.candidate.candidate);
      break;
    case CIM_GIC_DELIVERY_CANDIDATE_SELECTED:
      if (gic->ccandidate)
        c_candidate_select (gic->ccandidate, &delivery->payload.selection);
      break;
  }

  g_object_unref (owner);
}

static gboolean
cim_gic_drain_deliveries (gpointer data)
{
  CimGicCallbackContext* context = data;

  while (TRUE)
  {
    CimGicDelivery* delivery;

    g_mutex_lock (&context->queue_mutex);

    if (g_atomic_int_get (&context->closing))
    {
      while ((delivery = g_queue_pop_head (&context->deliveries)) != NULL)
        cim_gic_delivery_free (delivery);

      context->source_scheduled = FALSE;
      g_mutex_unlock (&context->queue_mutex);
      return G_SOURCE_REMOVE;
    }

    delivery = g_queue_pop_head (&context->deliveries);
    if (!delivery)
    {
      context->source_scheduled = FALSE;
      g_mutex_unlock (&context->queue_mutex);
      return G_SOURCE_REMOVE;
    }

    g_mutex_unlock (&context->queue_mutex);
    cim_gic_deliver_one (context, delivery);
    cim_gic_delivery_free (delivery);
  }
}

static void
cim_gic_callback_context_source_unref (gpointer data)
{
  cim_gic_callback_context_unref (data);
}

static void
cim_gic_queue_delivery (CimGic* gic, CimGicDelivery* delivery)
{
  CimGicCallbackContext* context = gic->callback_context;
  gboolean schedule_source = FALSE;
  GSource* source;

  if (!context)
    g_error ("Cim GTK bridge callback context is NULL");

  g_mutex_lock (&context->queue_mutex);

  if (g_atomic_int_get (&context->closing))
  {
    g_mutex_unlock (&context->queue_mutex);
    cim_gic_delivery_free (delivery);
    return;
  }

  g_queue_push_tail (&context->deliveries, delivery);

  if (!context->source_scheduled)
  {
    context->source_scheduled = TRUE;
    cim_gic_callback_context_ref (context);
    schedule_source = TRUE;
  }

  g_mutex_unlock (&context->queue_mutex);

  if (!schedule_source)
    return;

  source = g_idle_source_new ();
  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_set_callback
    (source,
     cim_gic_drain_deliveries,
     context,
     cim_gic_callback_context_source_unref);

  if (g_source_attach (source, context->main_context) == 0)
    g_error ("Cim GTK bridge could not queue callback delivery");

  g_source_unref (source);
}

static CimGicSyncRequest*
cim_gic_sync_request_ref (CimGicSyncRequest* request)
{
  g_atomic_int_inc (&request->ref_count);
  return request;
}

static void
cim_gic_sync_request_unref (gpointer data)
{
  CimGicSyncRequest* request = data;

  if (!g_atomic_int_dec_and_test (&request->ref_count))
    return;

  if (request->source)
    g_source_unref (request->source);

  g_cond_clear (&request->condition);
  g_mutex_clear (&request->mutex);
  cim_gic_callback_context_unref (request->context);
  g_free (request);
}

static CimGicSyncRequest*
cim_gic_sync_request_new (CimGicCallbackContext* context,
                          CimGicSyncType type)
{
  CimGicSyncRequest* request = g_new0 (CimGicSyncRequest, 1);

  request->ref_count = 1;
  request->context = cim_gic_callback_context_ref (context);
  request->type = type;
  g_mutex_init (&request->mutex);
  g_cond_init (&request->condition);
  return request;
}

static void
cim_gic_sync_request_complete (CimGicSyncRequest* request,
                               gboolean result)
{
  g_mutex_lock (&request->mutex);

  if (!request->completed)
  {
    request->result = result;
    request->completed = TRUE;
    g_cond_broadcast (&request->condition);
  }

  g_mutex_unlock (&request->mutex);
}

static gboolean
cim_gic_dispatch_sync_request (gpointer data)
{
  CimGicSyncRequest* request = data;
  CimGicCallbackContext* context = request->context;
  GObject* owner;
  gboolean result = FALSE;

  if (g_atomic_int_get (&context->closing))
  {
    cim_gic_sync_request_complete (request, FALSE);
    return G_SOURCE_REMOVE;
  }

  owner = g_weak_ref_get (&context->owner);
  if (!owner)
  {
    cim_gic_sync_request_complete (request, FALSE);
    return G_SOURCE_REMOVE;
  }

  if (request->type == CIM_GIC_SYNC_GET_SURROUND)
  {
    result = cim_gic_get_surround_on_owner (CIM_GIC (owner)) != NULL;
  }
  else
  {
    result = cim_gic_delete_surround_on_owner
      (CIM_GIC (owner), request->offset, request->n_chars);
  }

  g_object_unref (owner);
  cim_gic_sync_request_complete (request, result);
  return G_SOURCE_REMOVE;
}

static gboolean
cim_gic_run_sync_request (CimGic* gic, CimGicSyncRequest* request)
{
  CimGicCallbackContext* context = gic->callback_context;
  gboolean result;

  if (!context)
    g_error ("Cim GTK bridge callback context is NULL");

  g_mutex_lock (&context->sync_mutex);

  if (g_atomic_int_get (&context->closing))
  {
    g_mutex_unlock (&context->sync_mutex);
    return FALSE;
  }

  context->sync_requests = g_list_prepend (context->sync_requests, request);

  request->source = g_idle_source_new ();
  g_source_set_priority (request->source, G_PRIORITY_DEFAULT);
  g_source_set_callback
    (request->source,
     cim_gic_dispatch_sync_request,
     cim_gic_sync_request_ref (request),
     cim_gic_sync_request_unref);

  if (g_source_attach (request->source, context->main_context) == 0)
    g_error ("Cim GTK bridge could not queue synchronous callback");

#ifdef CIM_BRIDGE_TEST
  context->test_sync_ready_count++;
  g_cond_broadcast (&context->test_sync_condition);
#endif
  g_mutex_unlock (&context->sync_mutex);

  g_mutex_lock (&request->mutex);
  while (!request->completed)
    g_cond_wait (&request->condition, &request->mutex);

  result = request->result;
  g_mutex_unlock (&request->mutex);

  g_mutex_lock (&context->sync_mutex);
  context->sync_requests = g_list_remove (context->sync_requests, request);
#ifdef CIM_BRIDGE_TEST
  if (context->test_sync_ready_count == 0)
    g_error ("Cim GTK bridge test lost a synchronous callback");

  context->test_sync_ready_count--;
#endif
  g_mutex_unlock (&context->sync_mutex);
  return result;
}

static void
cim_gic_cancel_sync_requests (CimGicCallbackContext* context)
{
  GList* link;

  g_mutex_lock (&context->sync_mutex);
  g_atomic_int_set (&context->closing, TRUE);

  for (link = context->sync_requests; link; link = link->next)
  {
    CimGicSyncRequest* request = link->data;

    if (request->source)
      g_source_destroy (request->source);

    cim_gic_sync_request_complete (request, FALSE);
  }

  g_mutex_unlock (&context->sync_mutex);
}

static gboolean cim_gic_filter_keypress (GtkIMContext* context, GdkEventKey* event)
{
  CimGic* gic = CIM_GIC (context);
  gboolean retval;
  CimEvent cevent;

  if (!gic->ic)
    return gtk_im_context_filter_keypress (gic->simple, event);

  if (event->type == GDK_KEY_PRESS)
    cevent.type = CIM_EVENT_KEY_PRESS;
  else
    cevent.type = CIM_EVENT_KEY_RELEASE;

  cevent.state   = event->state;
  cevent.keyval  = event->keyval;
  cevent.keycode = event->hardware_keycode;

  retval = cim_ic_filter_event (gic->ic, &cevent);

  if (!retval)
    return gtk_im_context_filter_keypress (gic->simple, event);

  return retval;
}

static void cim_gic_reset (GtkIMContext* context)
{
  CimGic* gic = CIM_GIC (context);

  if (gic->ic)
    cim_ic_reset (gic->ic);

  gtk_im_context_reset (gic->simple);
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

  gtk_im_context_set_client_window (gic->simple, window);
}

static void cim_gic_get_preedit_string (GtkIMContext*   context,
                                        char**          text,
                                        PangoAttrList** attrs,
                                        int*            cursor_pos)
{
  CimGic* gic = CIM_GIC (context);
  const CimPreedit* preedit;

  if (!gic->ic)
  {
    gtk_im_context_get_preedit_string (gic->simple, text, attrs, cursor_pos);
    return;
  }

  preedit = cim_ic_get_preedit (gic->ic);

  if (!preedit)
  {
    gtk_im_context_get_preedit_string (gic->simple, text, attrs, cursor_pos);
    return;
  }

  if (!c_preedit_to_gtk (preedit, text, attrs, cursor_pos))
    g_error ("Cim GTK bridge received invalid preedit data");
}

static void cim_gic_focus_in (GtkIMContext* context)
{
  CimGic* gic = CIM_GIC (context);

  if (gic->ic)
    cim_ic_focus_in (gic->ic);

  gtk_im_context_focus_in (gic->simple);
}

static void cim_gic_focus_out (GtkIMContext* context)
{
  CimGic* gic = CIM_GIC (context);

  if (gic->ic)
    cim_ic_focus_out (gic->ic);

  gtk_im_context_focus_out (gic->simple);
}

static gboolean
cim_rect_from_gdk (const GdkRectangle* source, CimRect* result)
{
  if (!source || !result || source->width < 0 || source->height < 0)
    return FALSE;

  if (
#if INT_MAX > INT32_MAX
      (gint64) source->x < INT32_MIN ||
      (gint64) source->x > INT32_MAX ||
      (gint64) source->y < INT32_MIN ||
      (gint64) source->y > INT32_MAX ||
#endif
      (guint64) source->width > UINT32_MAX ||
      (guint64) source->height > UINT32_MAX)
    return FALSE;

  result->x      = (int32_t) source->x;
  result->y      = (int32_t) source->y;
  result->width  = (uint32_t) source->width;
  result->height = (uint32_t) source->height;

  return TRUE;
}

static void cim_gic_set_cursor_pos (GtkIMContext* context, GdkRectangle* area)
{
  CimGic* gic = CIM_GIC (context);
  CimRect cim_area;

  if (!area)
    g_error ("Cim GTK bridge received a NULL cursor rectangle");

  gtk_im_context_set_cursor_location (gic->simple, area);

  gic->cursor_pos = *area;

  /* If the function below doesn't work for wayland,
     try gtk_widget_translate_coordinates(). */
  if (gic->client_window)
    gdk_window_get_root_coords (gic->client_window,
                                area->x,
                                area->y,
                                &gic->cursor_pos.x,
                                &gic->cursor_pos.y);

  if (!gic->ic)
    return;

  if (!cim_rect_from_gdk (&gic->cursor_pos, &cim_area))
    g_error ("Cim GTK bridge received an invalid cursor rectangle");

  cim_ic_set_cursor_pos (gic->ic, &cim_area);
}

static void cb_simple_preedit_start (GtkIMContext* unused, CimGic* gic)
{
  (void) unused;
  g_signal_emit_by_name (gic, "preedit-start");
}

static void cb_simple_preedit_end (GtkIMContext* unused, CimGic* gic)
{
  (void) unused;
  g_signal_emit_by_name (gic, "preedit-end");
}

static void cb_simple_preedit_changed (GtkIMContext* unused, CimGic* gic)
{
  (void) unused;
  g_signal_emit_by_name (gic, "preedit-changed");
}

static void cb_simple_commit (GtkIMContext* unused,
                              const char* text,
                              CimGic* gic)
{
  (void) unused;
  g_signal_emit_by_name (gic, "commit", text);
}

static gboolean cb_simple_delete_surround (GtkIMContext* unused,
                                           int offset,
                                           int n_chars,
                                           CimGic* gic)
{
  gboolean retval;

  (void) unused;
  g_signal_emit_by_name
    (gic, "delete-surrounding", offset, n_chars, &retval);
  return retval;
}

static gboolean cb_simple_retrieve_surround (GtkIMContext* unused, CimGic* gic)
{
  gboolean retval;

  (void) unused;
  g_signal_emit_by_name (gic, "retrieve-surrounding", &retval);
  return retval;
}

static void cb_preedit_start (CimIcHandle unused, void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused;

  if (!gic || !gic->callback_context)
    g_error ("Cim GTK bridge received invalid callback user data");

  if (!g_atomic_int_get (&gic->callback_context->use_preedit))
    return;

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_PREEDIT_START;
  cim_gic_queue_delivery (gic, delivery);
}

static void cb_preedit_end (CimIcHandle unused, void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused;

  if (!gic || !gic->callback_context)
    g_error ("Cim GTK bridge received invalid callback user data");

  if (!g_atomic_int_get (&gic->callback_context->use_preedit))
    return;

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_PREEDIT_END;
  cim_gic_queue_delivery (gic, delivery);
}

static void cb_preedit_changed (CimIcHandle unused1,
                                const CimPreedit* unused2,
                                void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused1;
  (void) unused2;

  if (!gic || !gic->callback_context)
    g_error ("Cim GTK bridge received invalid callback user data");

  if (!g_atomic_int_get (&gic->callback_context->use_preedit))
    return;

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_PREEDIT_CHANGED;
  cim_gic_queue_delivery (gic, delivery);
}

static void cb_change_page (CCandidate* unused,
                            uint32_t page_index,
                            CimIcHandle ic)
{
  (void) unused;
  cim_ic_change_candidate_page (ic, page_index);
}

static void cb_activate_item (CCandidate* unused,
                              uint32_t row,
                              uint32_t col,
                              CimIcHandle ic)
{
  (void) unused;
  cim_ic_activate_candidate_item (ic, row, col);
}

static void cb_candidate_show (CimIcHandle unused,
                               uint32_t n_rows,
                               uint32_t n_cols,
                               bool show_aux,
                               void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused;

  if (!gic)
    g_error ("Cim GTK bridge received NULL callback user data");

  if (n_rows > (uint32_t) INT_MAX || n_cols > (uint32_t) INT_MAX)
    g_error ("Cim GTK bridge received oversized candidate dimensions");

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_CANDIDATE_SHOW;
  delivery->payload.candidate_show.n_rows = n_rows;
  delivery->payload.candidate_show.n_cols = n_cols;
  delivery->payload.candidate_show.show_aux = show_aux;
  cim_gic_queue_delivery (gic, delivery);
}

static void cb_candidate_hide (CimIcHandle unused, void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused;

  if (!gic)
    g_error ("Cim GTK bridge received NULL callback user data");

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_CANDIDATE_HIDE;
  cim_gic_queue_delivery (gic, delivery);
}

static void cb_candidate_changed (CimIcHandle unused,
                                  const CimCandidate* candidate,
                                  void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused;

  if (!gic)
    g_error ("Cim GTK bridge received NULL callback user data");

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_CANDIDATE_CHANGED;
  cim_candidate_snapshot_copy (&delivery->payload.candidate, candidate);
  cim_gic_queue_delivery (gic, delivery);
}

static void cb_candidate_selected (CimIcHandle unused,
                                   const CimSelection* selection,
                                   void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused;

  if (!gic || !selection)
    g_error ("Cim GTK bridge received an invalid candidate selection");

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_CANDIDATE_SELECTED;
  delivery->payload.selection = *selection;
  cim_gic_queue_delivery (gic, delivery);
}
static void cim_gic_set_use_preedit (GtkIMContext* context,
                                     gboolean use_preedit)
{
  CimGic* gic = CIM_GIC (context);

  g_atomic_int_set
    (&gic->callback_context->use_preedit, use_preedit ? TRUE : FALSE);

  gtk_im_context_set_use_preedit (gic->simple, use_preedit);
}

static void cim_gic_set_surround (GtkIMContext* context,
                                  const char*   text,
                                  int           len,
                                  int           cursor_index_in_bytes)
{
  CimGic* gic = CIM_GIC (context);
  gsize text_len;
  glong cursor_pos;

  if (!text)
    g_error ("Cim GTK bridge received NULL surrounding text");

  if (len < -1 || cursor_index_in_bytes < 0)
    g_error ("Cim GTK bridge received an invalid surrounding-text range");

  if (len == -1)
    text_len = strlen (text);
  else
    text_len = (gsize) len;

  if (text_len > UINT32_MAX || text_len > G_MAXSSIZE ||
      (gsize) cursor_index_in_bytes > text_len)
    g_error ("Cim GTK bridge received oversized surrounding text");

  if (!g_utf8_validate (text, (gssize) text_len, NULL) ||
      !g_utf8_validate (text, cursor_index_in_bytes, NULL))
    g_error ("Cim GTK bridge received invalid UTF-8 surrounding text");

  cursor_pos = g_utf8_strlen (text, cursor_index_in_bytes);

  if (cursor_pos < 0 || (uint64_t) cursor_pos > UINT32_MAX)
    g_error ("Cim GTK bridge could not convert surrounding-text cursor");

  gtk_im_context_set_surrounding
    (gic->simple, text, len, cursor_index_in_bytes);

  g_free (gic->surround_text);
  gic->surround_text = g_strndup (text, text_len);

  gic->surround.text       = gic->surround_text;
  gic->surround.len        = (uint32_t) text_len;
  gic->surround.cursor_pos = (uint32_t) cursor_pos;
  gic->surround.anchor_pos = (uint32_t) cursor_pos;
}

GtkIMContext* cim_gic_new ()
{
  return g_object_new (cim_gic_get_type (), NULL);
}

#ifdef CIM_BRIDGE_TEST
gboolean
cim_gic_test_filter_event (GtkIMContext* context, uint32_t keyval)
{
  CimGic* gic;
  CimEvent event = {
    .type = CIM_EVENT_KEY_PRESS,
    .state = 0,
    .keyval = keyval,
    .keycode = 0
  };

  if (!context)
    g_error ("Cim GTK bridge test received a NULL context");

  gic = CIM_GIC (context);
  if (!gic->ic)
    g_error ("Cim GTK bridge test input context is unavailable");

  return cim_ic_filter_event (gic->ic, &event);
}

gboolean
cim_gic_test_wait_for_sync_pending (GtkIMContext* context)
{
  CimGic* gic;
  CimGicCallbackContext* callback_context;
  gint64 deadline;
  gboolean ready;

  if (!context)
    g_error ("Cim GTK bridge test received a NULL context");

  gic = CIM_GIC (context);
  callback_context = gic->callback_context;
  if (!callback_context)
    g_error ("Cim GTK bridge test callback context is unavailable");

  deadline = g_get_monotonic_time () + 5 * G_TIME_SPAN_SECOND;
  g_mutex_lock (&callback_context->sync_mutex);

  while (callback_context->test_sync_ready_count == 0 &&
         !g_atomic_int_get (&callback_context->closing))
  {
    if (!g_cond_wait_until
          (&callback_context->test_sync_condition,
           &callback_context->sync_mutex,
           deadline))
      break;
  }

  ready = callback_context->test_sync_ready_count != 0;
  g_mutex_unlock (&callback_context->sync_mutex);
  return ready;
}

void
cim_gic_test_invoke_commit (GtkIMContext* context, const char* text)
{
  CimGic* gic;

  if (!context || !text)
    g_error ("Cim GTK bridge test received invalid commit input");

  gic = CIM_GIC (context);

  if (!gic->ic || !gic->callbacks.commit)
    g_error ("Cim GTK bridge test commit callback is unavailable");

  gic->callbacks.commit (gic->ic, text, gic);
}
#endif

static void cb_commit (CimIcHandle unused,
                       const char* text,
                       void* user_data)
{
  CimGic* gic = user_data;
  CimGicDelivery* delivery;

  (void) unused;

  if (!gic || !text)
    g_error ("Cim GTK bridge received an invalid commit callback");

  if (!g_utf8_validate (text, -1, NULL))
    g_error ("Cim GTK bridge received invalid commit UTF-8");

  delivery = g_new0 (CimGicDelivery, 1);
  delivery->type = CIM_GIC_DELIVERY_COMMIT;
  delivery->payload.commit.text = g_strdup (text);
  cim_gic_queue_delivery (gic, delivery);
}

static bool cb_delete_surround2 (CimIcHandle unused,
                                 int32_t offset,
                                 uint32_t n_chars,
                                 void* gic)
{
  CimGic* context = gic;
  CimGicSyncRequest* request;
  gboolean result;

  (void) unused;

  if (!context || !context->callback_context)
    g_error ("Cim GTK bridge received invalid callback user data");

  if (g_thread_self () == context->callback_context->owner_thread)
  {
    if (g_atomic_int_get (&context->callback_context->closing))
      return false;

    return cim_gic_delete_surround_on_owner (context, offset, n_chars);
  }

  request = cim_gic_sync_request_new
    (context->callback_context, CIM_GIC_SYNC_DELETE_SURROUND);
  request->offset = offset;
  request->n_chars = n_chars;
  result = cim_gic_run_sync_request (context, request);
  cim_gic_sync_request_unref (request);
  return result;
}

static const CimSurround*
cim_gic_get_surround_on_owner (CimGic* gic)
{
  gboolean retval = FALSE;

  g_signal_emit_by_name (gic, "retrieve-surrounding", &retval);

  if (retval)
    return &gic->surround;

  return NULL;
}

static bool
cim_gic_delete_surround_on_owner (CimGic* gic,
                                  int32_t offset,
                                  uint32_t n_chars)
{
  gboolean retval = FALSE;

  if (
#if INT_MAX < INT32_MAX
      (int64_t) offset < INT_MIN || (int64_t) offset > INT_MAX ||
#endif
      n_chars > (uint32_t) INT_MAX)
    g_error ("Cim GTK bridge received an invalid deletion range");

  g_signal_emit_by_name
    (gic,
     "delete-surrounding",
     (gint) offset,
     (gint) n_chars,
     &retval);
  return retval;
}

static const CimSurround*
cb_get_surround (CimIcHandle unused, void* user_data)
{
  CimGic* gic = user_data;
  CimGicSyncRequest* request;
  gboolean result;

  (void) unused;

  if (!gic || !gic->callback_context)
    g_error ("Cim GTK bridge received invalid callback user data");

  if (g_thread_self () == gic->callback_context->owner_thread)
  {
    if (g_atomic_int_get (&gic->callback_context->closing))
      return NULL;

    return cim_gic_get_surround_on_owner (gic);
  }

  request = cim_gic_sync_request_new
    (gic->callback_context, CIM_GIC_SYNC_GET_SURROUND);
  result = cim_gic_run_sync_request (gic, request);
  cim_gic_sync_request_unref (request);

  if (result)
    return &gic->surround;

  return NULL;
}

static void cim_gic_init (CimGic* gic)
{
  gic->simple = gtk_im_context_simple_new ();
  gic->callback_context = cim_gic_callback_context_new (gic);
  gic->surround_text = NULL;
  gic->surround.text = NULL;
  gic->surround.len = 0;
  gic->surround.cursor_pos = 0;
  gic->surround.anchor_pos = 0;

  g_signal_connect
    (gic->simple, "commit", G_CALLBACK (cb_simple_commit), gic);
  g_signal_connect
    (gic->simple,
     "delete-surrounding",
     G_CALLBACK (cb_simple_delete_surround),
     gic);
  g_signal_connect
    (gic->simple,
     "preedit-changed",
     G_CALLBACK (cb_simple_preedit_changed),
     gic);
  g_signal_connect
    (gic->simple, "preedit-end", G_CALLBACK (cb_simple_preedit_end), gic);
  g_signal_connect
    (gic->simple, "preedit-start", G_CALLBACK (cb_simple_preedit_start), gic);
  g_signal_connect
    (gic->simple,
     "retrieve-surrounding",
     G_CALLBACK (cb_simple_retrieve_surround),
     gic);

  gic->callbacks = (CimCallbacks) {
    .preedit_start      = cb_preedit_start,
    .preedit_end        = cb_preedit_end,
    .preedit_changed    = cb_preedit_changed,
    .commit             = cb_commit,
    .get_surround       = cb_get_surround,
    .delete_surround    = cb_delete_surround2,
    .candidate_show     = cb_candidate_show,
    .candidate_hide     = cb_candidate_hide,
    .candidate_changed  = cb_candidate_changed,
    .candidate_selected = cb_candidate_selected
  };

  gic->ic = cim_ic_create ();
  if (gic->ic)
    cim_ic_set_callbacks (gic->ic, &gic->callbacks, gic);
}

static void cim_gic_finalize (GObject* object)
{
  CimGic* gic = CIM_GIC (object);
  CimGicDelivery* delivery;

  cim_gic_cancel_sync_requests (gic->callback_context);
  g_weak_ref_set (&gic->callback_context->owner, NULL);

  g_mutex_lock (&gic->callback_context->queue_mutex);

  while ((delivery =
            g_queue_pop_head (&gic->callback_context->deliveries)) != NULL)
    cim_gic_delivery_free (delivery);

  g_mutex_unlock (&gic->callback_context->queue_mutex);

  if (gic->ic)
  {
    cim_ic_destroy (gic->ic);
    gic->ic = NULL;
  }

  g_mutex_lock (&gic->callback_context->sync_mutex);

  if (gic->callback_context->sync_requests)
    g_error ("Cim GTK plugin destroy did not quiesce callbacks");

  g_mutex_unlock (&gic->callback_context->sync_mutex);

  cim_gic_callback_context_unref (gic->callback_context);
  gic->callback_context = NULL;

  g_object_unref (gic->simple);

  g_free (gic->surround_text);

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
