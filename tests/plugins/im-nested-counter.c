// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * im-nested-counter.c
 * Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */

#include "cim.h"

#include <stdatomic.h>
#include <stdlib.h>

struct CimIcImpl
{
  unsigned int marker;
};

static atomic_int init_calls;
static atomic_int fini_calls;
static atomic_int live_attachments;
static atomic_int create_calls;
static atomic_int destroy_calls;
static atomic_int live_contexts;

static const CimInfo counter_info = {
  .name = "nested-counter",
  .desc = "Nested Cim host lifecycle counter",
  .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CimInfo *
counter_get_info (void)
{
  return &counter_info;
}

static int
counter_init (void)
{
  atomic_fetch_add_explicit (&init_calls, 1, memory_order_relaxed);
  atomic_fetch_add_explicit (&live_attachments, 1, memory_order_relaxed);
  return 0;
}

static void
counter_fini (void)
{
  int previous;

  atomic_fetch_add_explicit (&fini_calls, 1, memory_order_relaxed);
  previous = atomic_fetch_sub_explicit
    (&live_attachments, 1, memory_order_relaxed);

  if (previous <= 0)
    abort ();
}

static CimIcHandle
counter_create (void)
{
  struct CimIcImpl *ic = calloc (1, sizeof (*ic));

  if (ic == NULL)
    return NULL;

  ic->marker = 0xc1U;
  atomic_fetch_add_explicit (&create_calls, 1, memory_order_relaxed);
  atomic_fetch_add_explicit (&live_contexts, 1, memory_order_relaxed);
  return ic;
}

static void
counter_destroy (CimIcHandle ic)
{
  int previous;

  if (ic == NULL)
    abort ();

  free (ic);
  atomic_fetch_add_explicit (&destroy_calls, 1, memory_order_relaxed);
  previous = atomic_fetch_sub_explicit
    (&live_contexts, 1, memory_order_relaxed);

  if (previous <= 0)
    abort ();
}

static CimIcVTable counter_vtable = {
  .create  = counter_create,
  .destroy = counter_destroy
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding       = 0,
  .get_info      = counter_get_info,
  .init          = counter_init,
  .fini          = counter_fini,
  .vtable        = &counter_vtable,
  .reserved      = { NULL, NULL }
};

int
nested_counter_reset (void)
{
  if (atomic_load_explicit
        (&live_attachments, memory_order_relaxed) != 0 ||
      atomic_load_explicit (&live_contexts, memory_order_relaxed) != 0)
    return -1;

  atomic_store_explicit (&init_calls, 0, memory_order_relaxed);
  atomic_store_explicit (&fini_calls, 0, memory_order_relaxed);
  atomic_store_explicit (&create_calls, 0, memory_order_relaxed);
  atomic_store_explicit (&destroy_calls, 0, memory_order_relaxed);
  return 0;
}

int
nested_counter_init_calls (void)
{
  return atomic_load_explicit (&init_calls, memory_order_relaxed);
}

int
nested_counter_fini_calls (void)
{
  return atomic_load_explicit (&fini_calls, memory_order_relaxed);
}

int
nested_counter_live_attachments (void)
{
  return atomic_load_explicit (&live_attachments, memory_order_relaxed);
}

int
nested_counter_create_calls (void)
{
  return atomic_load_explicit (&create_calls, memory_order_relaxed);
}

int
nested_counter_destroy_calls (void)
{
  return atomic_load_explicit (&destroy_calls, memory_order_relaxed);
}

int
nested_counter_live_contexts (void)
{
  return atomic_load_explicit (&live_contexts, memory_order_relaxed);
}
