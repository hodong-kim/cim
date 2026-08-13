// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * im-reentrant-init.c
 * Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */

#include "cim.h"

#include <stddef.h>

static int
reentrant_init (void)
{
  (void) cim_ic_create ();
  return -1;
}

static CimIcHandle
reentrant_create (void)
{
  return NULL;
}

static void
reentrant_destroy (CimIcHandle ic)
{
  (void) ic;
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
  .fini          = NULL,
  .vtable        = &reentrant_vtable,
  .reserved      = { NULL, NULL }
};
