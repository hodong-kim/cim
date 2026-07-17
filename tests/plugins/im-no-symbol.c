/* im-no-symbol.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 *
 * Plugin used to verify missing cim_plugin symbol handling.
 */

/* This shared object intentionally does not export cim_plugin. */

static int some_other_symbol (void) __attribute__((unused));
static int some_other_symbol (void) { return 0; }
