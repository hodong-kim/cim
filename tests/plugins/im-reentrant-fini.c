// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * im-reentrant-fini.c
 * Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */

#include "cim.h"

#include <stddef.h>
#include <stdlib.h>

struct CimIcImpl
{
  unsigned int marker;
};

static int
reentrant_init (void)
{
  return 0;
}

static void
reentrant_fini (void)
{
  (void) cim_ic_create ();
}

static CimIcHandle
reentrant_create (void)
{
  struct CimIcImpl *ic = calloc (1, sizeof (*ic));

  if (ic == NULL)
    return NULL;

  ic->marker = 0xc1U;
  return ic;
}

static void
reentrant_destroy (CimIcHandle ic)
{
  free (ic);
}

static CimIcVTable reentrant_vtable = {
  .create  = reentrant_create,
  .destroy = reentrant_destroy
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding       = 0,
  .get_info      = NULL,
  .init          = reentrant_init,
  .fini          = reentrant_fini,
  .vtable        = &reentrant_vtable,
  .reserved      = { NULL, NULL }
};
