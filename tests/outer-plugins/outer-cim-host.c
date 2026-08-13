// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * outer-cim-host.c
 * Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */

#include "cim.h"

#include <stddef.h>

#ifndef CIM_OUTER_OPEN
#error "CIM_OUTER_OPEN must name the exported open function"
#endif

#ifndef CIM_OUTER_CLOSE
#error "CIM_OUTER_CLOSE must name the exported close function"
#endif

#if defined(__GNUC__) || defined(__clang__)
#define CIM_TEST_EXPORT __attribute__((visibility("default")))
#else
#define CIM_TEST_EXPORT
#endif

static CimIcHandle outer_ic;

CIM_TEST_EXPORT int
CIM_OUTER_OPEN (void)
{
  CimError error;

  if (outer_ic != NULL)
    return -1;

  outer_ic = cim_ic_create ();
  if (outer_ic != NULL)
    return 0;

  error = cim_get_last_error ();
  return error == CIM_ERROR_NONE ? -3 : (int) error;
}

CIM_TEST_EXPORT int
CIM_OUTER_CLOSE (void)
{
  if (outer_ic == NULL)
    return -2;

  cim_ic_destroy (outer_ic);
  outer_ic = NULL;
  return 0;
}
