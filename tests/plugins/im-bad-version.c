/* im-bad-version.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Plugin used to verify API major version mismatch handling.
 */

#include "cim.h"
#include <stdio.h>

struct CimIcImpl
{
  int dummy;
};

static const CimInfo bad_version_info = {
  .name = "bad-version",
  .desc = "Plugin whose cim_api_major is intentionally invalid",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo *
bad_version_get_info (void)
{
  return &bad_version_info;
}

static int
bad_version_init (void)
{
  fprintf (stderr, "bad_version_init\n");
  return 0;
}

static void
bad_version_fini (void)
{
  fprintf (stderr, "bad_version_fini\n");
}

static CimIcHandle
bad_version_create (void)
{
  fprintf (stderr, "bad_version_create\n");
  return NULL;
}

static void
bad_version_destroy (CimIcHandle ic)
{
  (void) ic;
  fprintf (stderr, "bad_version_destroy\n");
}

static CimIcVTable bad_version_vtable = {
  .create = bad_version_create,
  .destroy = bad_version_destroy
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION + 1,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding = 0,
  .get_info = bad_version_get_info,
  .init = bad_version_init,
  .fini = bad_version_fini,
  .vtable = &bad_version_vtable,
  .reserved = { NULL, NULL }
};
