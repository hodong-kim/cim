/* im-no-create.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Plugin used to verify handling of a missing vtable.create slot.
 */

#include "cim.h"
#include <stdio.h>

struct CimIcImpl
{
  int dummy;
};

static const CimInfo no_create_info = {
  .name = "no-create",
  .desc = "Plugin whose vtable.create is NULL",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo *
no_create_get_info (void)
{
  return &no_create_info;
}

static int
no_create_init (void)
{
  fprintf (stderr, "no_create_init\n");
  return 0;
}

static void
no_create_fini (void)
{
  fprintf (stderr, "no_create_fini\n");
}

static void
no_create_destroy (CimIcHandle ic)
{
  (void) ic;
  fprintf (stderr, "no_create_destroy\n");
}

static CimIcVTable no_create_vtable = {
  .create = NULL,
  .destroy = no_create_destroy
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding = 0,
  .get_info = no_create_get_info,
  .init = no_create_init,
  .fini = no_create_fini,
  .vtable = &no_create_vtable,
  .reserved = { NULL, NULL }
};
