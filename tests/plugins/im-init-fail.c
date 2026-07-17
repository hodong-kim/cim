/* im-init-fail.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Plugin used to verify init-failure handling.
 */

#include "cim.h"
#include <stdio.h>

struct CimIcImpl
{
  int dummy;
};

static const CimInfo init_fail_info = {
  .name = "init-fail",
  .desc = "Plugin whose init() always fails",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo *
init_fail_get_info (void)
{
  return &init_fail_info;
}

static int
init_fail_init (void)
{
  fprintf (stderr, "init_fail_init\n");
  return -1;
}

static void
init_fail_fini (void)
{
  fprintf (stderr, "init_fail_fini\n");
}

static CimIcHandle
init_fail_create (void)
{
  fprintf (stderr, "init_fail_create\n");
  return NULL;
}

static void
init_fail_destroy (CimIcHandle ic)
{
  (void) ic;
  fprintf (stderr, "init_fail_destroy\n");
}

static CimIcVTable init_fail_vtable = {
  .create = init_fail_create,
  .destroy = init_fail_destroy
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding = 0,
  .get_info = init_fail_get_info,
  .init = init_fail_init,
  .fini = init_fail_fini,
  .vtable = &init_fail_vtable,
  .reserved = { NULL, NULL }
};
