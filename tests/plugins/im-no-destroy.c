/* im-no-destroy.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Plugin used to verify handling of a missing vtable.destroy slot.
 */

#include "cim.h"
#include <stdio.h>
#include <stdlib.h>

struct CimIcImpl
{
  int dummy;
};

static const CimInfo no_destroy_info = {
  .name = "no-destroy",
  .desc = "Plugin whose vtable.destroy is NULL",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo *
no_destroy_get_info (void)
{
  return &no_destroy_info;
}

static int
no_destroy_init (void)
{
  fprintf (stderr, "no_destroy_init\n");
  return 0;
}

static void
no_destroy_fini (void)
{
  fprintf (stderr, "no_destroy_fini\n");
}

static CimIcHandle
no_destroy_create (void)
{
  struct CimIcImpl *ic;

  fprintf (stderr, "no_destroy_create\n");

  ic = (struct CimIcImpl *) calloc (1, sizeof (struct CimIcImpl));
  if (ic == NULL)
    return NULL;

  ic->dummy = 1;
  return (CimIcHandle) ic;
}

static CimIcVTable no_destroy_vtable = {
  .create = no_destroy_create,
  .destroy = NULL
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding = 0,
  .get_info = no_destroy_get_info,
  .init = no_destroy_init,
  .fini = no_destroy_fini,
  .vtable = &no_destroy_vtable,
  .reserved = { NULL, NULL }
};
