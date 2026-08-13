// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * cim-c-api.c
 * This file is part of Cim.
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */

#include "cim.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef CIM_STATIC_RUNTIME
#define CIM_STATIC_RUNTIME 0
#endif

extern CimIcHandle cim_private_ic_create (void);
extern void cim_private_ic_destroy (CimIcHandle ic);
extern void cim_private_ic_focus_in (CimIcHandle ic);
extern void cim_private_ic_focus_out (CimIcHandle ic);
extern void cim_private_ic_reset (CimIcHandle ic);
extern bool cim_private_ic_filter_event (CimIcHandle ic,
                                         const CimEvent *event);
extern void cim_private_ic_set_cursor_pos (CimIcHandle ic,
                                           const CimRect *area);
extern void cim_private_ic_set_callbacks (CimIcHandle ic,
                                          const CimCallbacks *callbacks,
                                          void *user_data);
extern const CimPreedit *cim_private_ic_get_preedit (CimIcHandle ic);
extern const CimCandidate *cim_private_ic_get_candidate (CimIcHandle ic);
extern void cim_private_ic_activate_candidate_item (CimIcHandle ic,
                                                    uint32_t row,
                                                    uint32_t col);
extern void cim_private_ic_change_candidate_page (CimIcHandle ic,
                                                  uint32_t page_index);
extern char *cim_private_dup_plugin_path (void);
extern CimError cim_private_get_last_error (void);
extern const char *cim_private_strerror (unsigned int error);
extern void cim_private_runtime_require_idle_for_finalization (void);

#if CIM_STATIC_RUNTIME

#if defined(__GNUC__) || defined(__clang__)
#define CIM_CONSTRUCTOR __attribute__((constructor(101), used))
#define CIM_DESTRUCTOR  __attribute__((destructor(101), used))
#else
#error "static Cim embedding requires constructor and destructor attributes"
#endif

extern void ciminit (void);
extern void cimfinal (void);

static bool cim_static_runtime_initialized;

static void cim_static_runtime_initialize (void) CIM_CONSTRUCTOR;
static void cim_static_runtime_finalize (void) CIM_DESTRUCTOR;

static void
cim_static_runtime_initialize (void)
{
  ciminit ();
  cim_static_runtime_initialized = true;
}

static void
cim_static_runtime_finalize (void)
{
  if (!cim_static_runtime_initialized)
    return;

  cim_private_runtime_require_idle_for_finalization ();
  cimfinal ();
  cim_static_runtime_initialized = false;
}

#endif

CimIcHandle
cim_ic_create (void)
{
  return cim_private_ic_create ();
}

void
cim_ic_destroy (CimIcHandle ic)
{
  cim_private_ic_destroy (ic);
}

void
cim_ic_focus_in (CimIcHandle ic)
{
  cim_private_ic_focus_in (ic);
}

void
cim_ic_focus_out (CimIcHandle ic)
{
  cim_private_ic_focus_out (ic);
}

void
cim_ic_reset (CimIcHandle ic)
{
  cim_private_ic_reset (ic);
}

bool
cim_ic_filter_event (CimIcHandle ic, const CimEvent *event)
{
  return cim_private_ic_filter_event (ic, event);
}

void
cim_ic_set_cursor_pos (CimIcHandle ic, const CimRect *area)
{
  cim_private_ic_set_cursor_pos (ic, area);
}

void
cim_ic_set_callbacks (CimIcHandle ic,
                      const CimCallbacks *callbacks,
                      void *user_data)
{
  cim_private_ic_set_callbacks (ic, callbacks, user_data);
}

const CimPreedit *
cim_ic_get_preedit (CimIcHandle ic)
{
  return cim_private_ic_get_preedit (ic);
}

const CimCandidate *
cim_ic_get_candidate (CimIcHandle ic)
{
  return cim_private_ic_get_candidate (ic);
}

void
cim_ic_activate_candidate_item (CimIcHandle ic,
                                uint32_t row,
                                uint32_t col)
{
  cim_private_ic_activate_candidate_item (ic, row, col);
}

void
cim_ic_change_candidate_page (CimIcHandle ic, uint32_t page_index)
{
  cim_private_ic_change_candidate_page (ic, page_index);
}

char *
cim_dup_plugin_path (void)
{
  return cim_private_dup_plugin_path ();
}

CimError
cim_get_last_error (void)
{
  return cim_private_get_last_error ();
}

const char *
cim_strerror (CimError error)
{
  return cim_private_strerror ((unsigned int) error);
}
