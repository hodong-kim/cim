/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * cim.c
 * This file is part of Cim.
 *
 * Copyright (C) 2023,2024 Hodong Kim <hodong@nimfsoft.art>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "cim.h"
#include <dlfcn.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "c-utils.h"
#include "c-str.h"
#include "c-mem.h"
#include "c-log.h"

static void*    cim_plugin;
static CimIc* (*cim_plugin_new_ic)  ();
static void   (*cim_plugin_free_ic) (CimIc*);
static atomic_uint cim_ref_count;

/*
 * Returns the newly allocated cim.so path string on success,
 * or nullptr on failure.
 * Free it with free().
 */
char* cim_get_cim_so_path ()
{
  char* path;
  char* conf_dir;

  conf_dir = c_get_user_config_dir ();

  if (!conf_dir)
    return nullptr;

  path = c_str_join (conf_dir, "/cim.so", nullptr);

  free (conf_dir);

  return path;
}

CimIc* cim_ic_new ()
{
  cim_ref_count++;

  if (cim_ref_count == 1)
  {
    char* path = cim_get_cim_so_path ();

    if (!path)
      goto fallback;

    cim_plugin = dlopen (path, RTLD_LAZY | RTLD_LOCAL);
    free (path);

    if (!cim_plugin)
    {
      c_log_warning ("Faild to open cim plugin: %s", dlerror ());
      goto fallback;
    }

    void (*cim_plugin_get_version) (int*, int*, int*);

    cim_plugin_get_version = dlsym (cim_plugin, "cim_plugin_get_version");
    cim_plugin_new_ic      = dlsym (cim_plugin, "cim_plugin_new_ic");
    cim_plugin_free_ic     = dlsym (cim_plugin, "cim_plugin_free_ic");

    bool version_check = false;

    if (cim_plugin_get_version)
    {
      int plugin_major;

      cim_plugin_get_version (&plugin_major, nullptr, nullptr);

      if (plugin_major == CIM_MAJOR_VERSION)
      {
        version_check = true;
      }
      else
      {
        const int cim_major = CIM_MAJOR_VERSION;
        c_log_warning (
          "Major version mismatch: cim plugin major version is %d, "
          "but this cim major version is %d.", plugin_major, cim_major);
      }
    }
    else
    {
      c_log_warning ("Symbol not found: cim_plugin_get_version");
    }

    if (!version_check || !cim_plugin_new_ic || !cim_plugin_free_ic)
    {
      if (!cim_plugin_new_ic)
        c_log_warning ("Symbol not found: cim_plugin_new_ic");

      if (!cim_plugin_free_ic)
        c_log_warning ("Symbol not found: cim_plugin_free_ic");

      dlclose (cim_plugin);
      cim_plugin         = nullptr;
      cim_plugin_new_ic  = nullptr;
      cim_plugin_free_ic = nullptr;

      goto fallback;
    }
  }

  if (cim_plugin)
    return cim_plugin_new_ic ();

  fallback:

  return c_calloc (1, sizeof (CimIc));
}

void cim_ic_free (CimIc* ic)
{
  cim_ref_count--;

  if (cim_plugin)
    cim_plugin_free_ic (ic);
  else
    free (ic);

  if (cim_ref_count == 0)
  {
    if (cim_plugin)
      dlclose (cim_plugin);

    cim_plugin         = nullptr;
    cim_plugin_new_ic  = nullptr;
    cim_plugin_free_ic = nullptr;
  }
}

void cim_ic_focus_in (CimIc* ic)
{
  if (ic->focus_in)
    ic->focus_in (ic);
}

void cim_ic_focus_out (CimIc* ic)
{
  if (ic->focus_out)
    ic->focus_out (ic);
}

void cim_ic_reset (CimIc* ic)
{
  if (ic->reset)
    ic->reset (ic);
}

bool cim_ic_filter_event (CimIc* ic, const CimEvent* event)
{
  if (ic->filter_event)
    return ic->filter_event (ic, event);

  return false;
}

void cim_ic_set_cursor_pos (CimIc* ic, const CimRect* area)
{
  if (ic->set_cursor_pos)
    ic->set_cursor_pos (ic, area);
}

const CimPreedit* cim_ic_get_preedit (CimIc* ic)
{
  if (ic->get_preedit)
    return ic->get_preedit (ic);

  c_log_critical ("cim_ic_get_preedit() must be implemented in the IM plugin.");

  static const CimPreedit preedit = { .text = "",
                                      .attrs = nullptr,
                                      .attrs_len = 0,
                                      .cursor_pos = 0 };
  return &preedit;
}

const CimCandidate* cim_ic_get_candidate (CimIc* ic)
{
  if (ic->get_candidate)
    return ic->get_candidate (ic);

  c_log_critical ("get_candidate() must be implemented in the IM plugin.");
  return nullptr;
}

void cim_ic_set_callbacks (CimIc* ic, ...)
{
  va_list ap;

  va_start (ap, ic);

  if (ic->set_vcallbacks)
    ic->set_vcallbacks (ic, ap);
  else
    c_log_critical ("set_vcallbacks() must be implemented in the IM plugin.");

  va_end (ap);
}

void cim_ic_activate_candidate_item (CimIc* ic, int row, int col)
{
  if (ic->activate_candidate_item)
    ic->activate_candidate_item (ic, row, col);
}

void cim_ic_change_candidate_page (CimIc* ic, int page_index)
{
  if (ic->change_candidate_page)
    ic->change_candidate_page (ic, page_index);
}
