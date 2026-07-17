/* im-create-fail.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Plugin used to verify create-failure handling.
 */

#include "cim.h"
#include <stdio.h>

struct CimIcImpl
{
  int dummy;
};

static const CimInfo create_fail_info = {
  .name = "create-fail",
  .desc = "Plugin whose create() always fails",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo *
create_fail_get_info (void)
{
  return &create_fail_info;
}

static int
create_fail_init (void)
{
  fprintf (stderr, "create_fail_init\n");
  return 0;
}

static void
create_fail_fini (void)
{
  fprintf (stderr, "create_fail_fini\n");
}

static CimIcHandle
create_fail_create (void)
{
  fprintf (stderr, "create_fail_create\n");
  return NULL;
}

static void
create_fail_destroy (CimIcHandle ic)
{
  (void) ic;
  fprintf (stderr, "create_fail_destroy\n");
}

static CimIcVTable create_fail_vtable = {
  .create = create_fail_create,
  .destroy = create_fail_destroy
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding = 0,
  .get_info = create_fail_get_info,
  .init = create_fail_init,
  .fini = create_fail_fini,
  .vtable = &create_fail_vtable,
  .reserved = { NULL, NULL }
};
