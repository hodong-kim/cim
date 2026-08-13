/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * im-bridge-callback.c
 * Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */

#include "cim.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
  BRIDGE_WORKER_GET_SURROUND,
  BRIDGE_WORKER_DELETE_SURROUND,
  BRIDGE_WORKER_TEARDOWN,
  BRIDGE_WORKER_PREEDIT
} BridgeWorkerKind;

struct CimIcImpl
{
  const CimCallbacks* callbacks;
  void* user_data;
  pthread_t worker;
  bool worker_started;
  BridgeWorkerKind worker_kind;
  char preedit_text[7];
  CimTextAttr preedit_attrs[2];
  CimPreedit preedit;
};

static const char EXPECTED_SURROUND[] =
  "A\xF0\x9F\x98\x80" "B";

static const CimInfo bridge_info = {
  .name = "bridge-callback",
  .desc = "Bridge callback threading test fixture",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo*
bridge_get_info (void)
{
  return &bridge_info;
}

static int
bridge_init (void)
{
  return 0;
}

static void
bridge_fini (void)
{
}

static bool
surround_matches (const CimSurround* surround)
{
  return surround != NULL &&
         surround->text != NULL &&
         surround->len == 6 &&
         surround->cursor_pos == 2 &&
         surround->anchor_pos == 2 &&
         memcmp (surround->text, EXPECTED_SURROUND, 6) == 0;
}

static void*
bridge_worker (void* data)
{
  struct CimIcImpl* impl = data;
  const char* result_text;

  if (impl == NULL || impl->callbacks == NULL)
    abort ();

  switch (impl->worker_kind)
  {
    case BRIDGE_WORKER_GET_SURROUND:
      if (impl->callbacks->get_surround == NULL ||
          impl->callbacks->commit == NULL)
        abort ();

      result_text = surround_matches
        (impl->callbacks->get_surround (impl, impl->user_data)) ?
          "get-surround-ok" : "get-surround-failed";
      impl->callbacks->commit (impl, result_text, impl->user_data);
      break;

    case BRIDGE_WORKER_DELETE_SURROUND:
      if (impl->callbacks->delete_surround == NULL ||
          impl->callbacks->commit == NULL)
        abort ();

      result_text = impl->callbacks->delete_surround
        (impl, -1, 1, impl->user_data) ?
          "delete-surround-ok" : "delete-surround-failed";
      impl->callbacks->commit (impl, result_text, impl->user_data);
      break;

    case BRIDGE_WORKER_TEARDOWN:
      if (impl->callbacks->get_surround == NULL)
        abort ();

      (void) impl->callbacks->get_surround (impl, impl->user_data);
      break;

    case BRIDGE_WORKER_PREEDIT:
    {
      char text[] = "A\xF0\x9F\x98\x80" "B";
      CimTextAttr attrs[2] = {
        {
          .type = CIM_TEXT_ATTR_HIGHLIGHT,
          .pos = 0,
          .n_chars = 1
        },
        {
          .type = CIM_TEXT_ATTR_UNDERLINE,
          .pos = 1,
          .n_chars = 1
        }
      };
      CimPreedit preedit = {
        .text = text,
        .attrs = attrs,
        .attrs_len = 2,
        .cursor_pos = 2
      };

      if (impl->callbacks->preedit_changed == NULL ||
          impl->callbacks->commit == NULL)
        abort ();

      impl->callbacks->preedit_changed
        (impl, &preedit, impl->user_data);
      memset (text, 'x', sizeof (text) - 1);
      memset (attrs, 0, sizeof (attrs));
      impl->callbacks->commit
        (impl, "preedit-sent", impl->user_data);
      break;
    }
  }

  return NULL;
}

static CimIcHandle
bridge_create (void)
{
  struct CimIcImpl* impl = calloc (1, sizeof (*impl));

  if (impl == NULL)
    return NULL;

  memcpy (impl->preedit_text, EXPECTED_SURROUND,
          sizeof (impl->preedit_text));
  impl->preedit_attrs[0].type = CIM_TEXT_ATTR_HIGHLIGHT;
  impl->preedit_attrs[0].pos = 0;
  impl->preedit_attrs[0].n_chars = 1;
  impl->preedit_attrs[1].type = CIM_TEXT_ATTR_UNDERLINE;
  impl->preedit_attrs[1].pos = 1;
  impl->preedit_attrs[1].n_chars = 1;
  impl->preedit.text = impl->preedit_text;
  impl->preedit.attrs = impl->preedit_attrs;
  impl->preedit.attrs_len = 2;
  impl->preedit.cursor_pos = 2;
  return impl;
}

static void
bridge_destroy (CimIcHandle ic)
{
  struct CimIcImpl* impl = ic;

  if (impl == NULL)
    abort ();

  if (impl->worker_started && pthread_join (impl->worker, NULL) != 0)
    abort ();

  free (impl);
}

static void
bridge_focus_in (CimIcHandle ic)
{
  (void) ic;
}

static void
bridge_focus_out (CimIcHandle ic)
{
  (void) ic;
}

static void
bridge_reset (CimIcHandle ic)
{
  (void) ic;
}

static bool
bridge_filter_event (CimIcHandle ic, const CimEvent* event)
{
  struct CimIcImpl* impl = ic;

  if (impl == NULL || event == NULL || impl->callbacks == NULL)
    abort ();

  if (impl->worker_started)
    abort ();

  switch (event->keyval)
  {
    case 'G':
      impl->worker_kind = BRIDGE_WORKER_GET_SURROUND;
      break;
    case 'D':
      impl->worker_kind = BRIDGE_WORKER_DELETE_SURROUND;
      break;
    case 'T':
      impl->worker_kind = BRIDGE_WORKER_TEARDOWN;
      break;
    case 'P':
      impl->worker_kind = BRIDGE_WORKER_PREEDIT;
      break;
    default:
      return false;
  }

  if (pthread_create (&impl->worker, NULL, bridge_worker, impl) != 0)
    abort ();

  impl->worker_started = true;
  return true;
}

static void
bridge_set_cursor_pos (CimIcHandle ic, const CimRect* area)
{
  (void) ic;
  (void) area;
}

static const CimPreedit*
bridge_get_preedit (CimIcHandle ic)
{
  struct CimIcImpl* impl = ic;

  if (impl == NULL)
    abort ();

  return &impl->preedit;
}

static const CimCandidate*
bridge_get_candidate (CimIcHandle ic)
{
  (void) ic;
  return NULL;
}

static void
bridge_set_callbacks (CimIcHandle ic,
                      const CimCallbacks* callbacks,
                      void* user_data)
{
  struct CimIcImpl* impl = ic;

  if (impl == NULL || callbacks == NULL)
    abort ();

  impl->callbacks = callbacks;
  impl->user_data = user_data;
}

static void
bridge_activate_candidate_item (CimIcHandle ic,
                                uint32_t row,
                                uint32_t col)
{
  (void) ic;
  (void) row;
  (void) col;
}

static void
bridge_change_candidate_page (CimIcHandle ic, uint32_t page_index)
{
  (void) ic;
  (void) page_index;
}

static CimIcVTable bridge_vtable = {
  .create                  = bridge_create,
  .destroy                 = bridge_destroy,
  .focus_in                = bridge_focus_in,
  .focus_out               = bridge_focus_out,
  .reset                   = bridge_reset,
  .filter_event            = bridge_filter_event,
  .set_cursor_pos          = bridge_set_cursor_pos,
  .get_preedit             = bridge_get_preedit,
  .get_candidate           = bridge_get_candidate,
  .set_callbacks           = bridge_set_callbacks,
  .activate_candidate_item = bridge_activate_candidate_item,
  .change_candidate_page   = bridge_change_candidate_page,
  .reserved                = { NULL, NULL, NULL, NULL }
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding       = 0,
  .get_info      = bridge_get_info,
  .init          = bridge_init,
  .fini          = bridge_fini,
  .vtable        = &bridge_vtable,
  .reserved      = { NULL, NULL }
};
