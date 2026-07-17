/* im-dummy.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Minimal Cim dummy plugin for testing:
 *   - cim_plugin symbol export
 *   - init/fini
 *   - create/destroy
 *
 * This plugin is only for validating libcim's flow.
 */

#include "cim.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

static bool
dummy_is_verbose_enabled (void)
{
  const char *value = getenv ("TEST_VERBOSE");

  if (value == NULL || value[0] == '\0')
    return false;

  if (strcmp (value, "0") == 0)
    return false;

  return true;
}

#define DUMMY_LOG(...)                         \
  do {                                         \
    if (dummy_is_verbose_enabled ())           \
      fprintf (stderr, __VA_ARGS__);           \
  } while (0)

struct CimIcImpl
{
  int dummy;
  const CimCallbacks *callbacks;
  void *user_data;
};

static CimTextAttr dummy_attrs[] = {
  { .type = CIM_TEXT_ATTR_UNDERLINE, .pos = 0, .n_chars = 5 }
};

static CimPreedit dummy_preedit = {
  .text = "dummy",
  .attrs = dummy_attrs,
  .attrs_len = 1,
  .cursor_pos = 5
};

static CimItem dummy_items[] = {
  { .data = "one", .type = CIM_ITEM_STRING, .padding = 0 },
  { .data = "two", .type = CIM_ITEM_STRING, .padding = 0 }
};

static CimCandidate dummy_candidate = {
  .page_index = 0,
  .n_pages = 1,
  .table = dummy_items,
  .n_rows = 1,
  .n_cols = 2,
  .aux_text = "aux",
  .aux_cursor_pos = 0,
  .padding = 0
};

static const CimInfo dummy_info = {
  .name = "dummy",
  .desc = "Minimal dummy plugin",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo *
dummy_get_info (void)
{
  return &dummy_info;
}

static int dummy_init (void)
{
  DUMMY_LOG ("dummy_init\n");
  return 0;
}

static void dummy_fini (void)
{
  DUMMY_LOG ("dummy_fini\n");
}

static CimIcHandle dummy_create (void)
{
  struct CimIcImpl *ic;

  DUMMY_LOG ("dummy_create\n");

  ic = (struct CimIcImpl *) calloc (1, sizeof (struct CimIcImpl));
  if (!ic)
    return NULL;

  ic->dummy = 1;
  return (CimIcHandle) ic;
}

static void dummy_destroy (CimIcHandle ic)
{
  DUMMY_LOG ("dummy_destroy\n");
  free (ic);
}

static void dummy_focus_in (CimIcHandle ic)
{
  (void) ic;
  DUMMY_LOG ("dummy_focus_in\n");
}

static void dummy_focus_out (CimIcHandle ic)
{
  (void) ic;
  DUMMY_LOG ("dummy_focus_out\n");
}

static void dummy_reset (CimIcHandle ic)
{
  (void) ic;
  DUMMY_LOG ("dummy_reset\n");
}

static bool dummy_filter_event (CimIcHandle ic, const CimEvent *event)
{
  (void) ic;
  (void) event;
  DUMMY_LOG ("dummy_filter_event\n");
  return false;
}

static void dummy_set_cursor_pos (CimIcHandle ic, const CimRect *area)
{
  (void) ic;

  if (!area)
    return;

  DUMMY_LOG ("dummy_set_cursor_pos: x=%d y=%d w=%u h=%u\n",
             area->x, area->y, area->width, area->height);
}

static const CimPreedit *dummy_get_preedit (CimIcHandle ic)
{
  (void) ic;
  DUMMY_LOG ("dummy_get_preedit\n");
  return &dummy_preedit;
}

static const CimCandidate *dummy_get_candidate (CimIcHandle ic)
{
  (void) ic;
  DUMMY_LOG ("dummy_get_candidate\n");
  return &dummy_candidate;
}

static void dummy_set_callbacks (CimIcHandle ic,
                                 const CimCallbacks *callbacks,
                                 void *user_data)
{
  struct CimIcImpl *impl = (struct CimIcImpl *) ic;

  DUMMY_LOG ("dummy_set_callbacks\n");

  if (!impl)
    return;

  impl->callbacks = callbacks;
  impl->user_data = user_data;
}

static void dummy_activate_candidate_item (CimIcHandle ic,
                                           uint32_t row,
                                           uint32_t col)
{
  (void) ic;
  DUMMY_LOG ("dummy_activate_candidate_item: row=%u col=%u\n",
             row, col);
}

static void dummy_change_candidate_page (CimIcHandle ic, uint32_t page_index)
{
  (void) ic;
  DUMMY_LOG ("dummy_change_candidate_page: page=%u\n", page_index);
}

static CimIcVTable dummy_vtable = {
  .create                  = dummy_create,
  .destroy                 = dummy_destroy,
  .focus_in                = dummy_focus_in,
  .focus_out               = dummy_focus_out,
  .reset                   = dummy_reset,
  .filter_event            = dummy_filter_event,
  .set_cursor_pos          = dummy_set_cursor_pos,
  .get_preedit             = dummy_get_preedit,
  .get_candidate           = dummy_get_candidate,
  .set_callbacks           = dummy_set_callbacks,
  .activate_candidate_item = dummy_activate_candidate_item,
  .change_candidate_page   = dummy_change_candidate_page
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding  = 0,
  .get_info = dummy_get_info,
  .init     = dummy_init,
  .fini     = dummy_fini,
  .vtable   = &dummy_vtable,
  .reserved = { NULL, NULL }
};
