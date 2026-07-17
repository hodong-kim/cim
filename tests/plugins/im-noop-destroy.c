/* im-noop-destroy.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 *
 * Test-only plugin whose destroy callback intentionally leaves the supplied
 * handle untouched. This permits deterministic reference-underflow testing
 * without dereferencing or freeing a stale handle.
 */

#include "cim.h"

#include <stddef.h>

static unsigned char context_token;

static CimIcHandle
noop_destroy_create (void)
{
  return (CimIcHandle) &context_token;
}

static void
noop_destroy_destroy (CimIcHandle ic)
{
  (void) ic;
}

static CimIcVTable noop_destroy_vtable = {
  .create  = noop_destroy_create,
  .destroy = noop_destroy_destroy
};

CimPlugin cim_plugin = {
  .cim_api_major = CIM_MAJOR_VERSION,
  .cim_api_minor = CIM_MINOR_VERSION,
  .cim_api_micro = CIM_MICRO_VERSION,
  .padding  = 0,
  .get_info = NULL,
  .init     = NULL,
  .fini     = NULL,
  .vtable   = &noop_destroy_vtable,
  .reserved = { NULL, NULL }
};
