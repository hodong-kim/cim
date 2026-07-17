// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * cim-internal.c
 * This file is part of Cim.
 * Copyright (C) 2023-2026 Hodong Kim <hodong@nimfsoft.com>
 */

#include "cim.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= *
 * C helper
 * ========================================================================= */

void
cim_contract_violation_c (const char *message)
{
  if (message == NULL)
    message = "unspecified contract violation";

  fprintf (stderr, "Cim contract violation: %s\n", message);
  fflush (stderr);
  abort ();
}

void
cim_unhandled_exception_c (const char *operation)
{
  if (operation == NULL)
    operation = "unknown C API entry point";

  fprintf (stderr,
           "Cim fatal error: unhandled Ada exception in %s\n",
           operation);
  fflush (stderr);
  abort ();
}

CimError
cim_get_plugin_path_c (char **path_out)
{
  static const char suffix[] = "/.config/cim/plugin";
  const char *env;
  size_t env_len;
  size_t path_len;
  char *path;

  if (path_out == NULL)
    cim_contract_violation_c ("cim_get_plugin_path_c: path_out is NULL");

  *path_out = NULL;

  env = getenv ("CIM_PLUGIN");

  if (env != NULL && env[0] != '\0')
  {
    path = strdup (env);

    if (path == NULL)
      return CIM_ERROR_ALLOCATION_FAILED;

    *path_out = path;
    return CIM_ERROR_NONE;
  }

  env = getenv ("HOME");

  if (env == NULL || env[0] == '\0')
    return CIM_ERROR_HOME_NOT_SET;

  env_len = strlen (env);

  if (env_len > SIZE_MAX - sizeof (suffix))
    return CIM_ERROR_PLUGIN_PATH_TOO_LONG;

  path_len = env_len + sizeof (suffix);
  path = malloc (path_len);

  if (path == NULL)
    return CIM_ERROR_ALLOCATION_FAILED;

  memcpy (path, env, env_len);
  memcpy (path + env_len, suffix, sizeof (suffix));

  *path_out = path;
  return CIM_ERROR_NONE;
}

void
cim_free_plugin_path_c (char *path)
{
  free (path);
}

char *
cim_get_plugin_symbol_c (void)
{
  static char symbol[] = "cim_plugin";

  return symbol;
}
