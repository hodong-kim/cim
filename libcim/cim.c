// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * cim.c
 * This file is part of Cim.
 *
 * Copyright (C) 2023-2025 Hodong Kim <hodong@nimfsoft.art>
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
#include "c-utils.h"
#include "c-str.h"
#include "c-mem.h"
#include "c-log.h"

static CimIcVTable vtable = { 0 };

struct CimIcImpl {
  CimIcVTable* vtable;
};

static void*      cim_handle;
static CimPlugin* cim_plugin;
static uint32_t   cim_ref_count;

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

CimIcHandle cim_ic_create ()
{
  cim_ref_count++;

  if (cim_ref_count == 1)
  {
    char* path = cim_get_cim_so_path ();

    if (!path)
      goto fallback;

    cim_handle = dlopen (path, RTLD_LAZY | RTLD_LOCAL);
    free (path);

    if (!cim_handle)
    {
      c_log_warning ("Faild to open cim plugin: %s", dlerror ());
      goto fallback;
    }

    cim_plugin = dlsym (cim_handle, "cim_plugin");
    if (!cim_plugin)
    {
      c_log_warning ("Can't load cim_plugin: %s", dlerror ());
      goto fallback;
    }

    bool version_check = false;

    if (cim_plugin->cim_api_major == CIM_MAJOR_VERSION)
    {
      version_check = true;
    }
    else
    {
      const int cim_major = CIM_MAJOR_VERSION;
      c_log_warning (
        "Major version mismatch: cim plugin major version is %d, "
        "but this caller major version is %d.",
        cim_plugin->cim_api_major, cim_major);
    }

    if (cim_plugin->init)
    {
      if (!cim_plugin->init ())
      {
        dlclose (cim_handle);
        cim_plugin = nullptr;
        cim_handle = nullptr;

        goto fallback;
      }
    }

    if (!version_check || cim_plugin->vtable ||
        !cim_plugin->vtable->create ||
        !cim_plugin->vtable->destroy)
    {
      if (!cim_plugin->vtable)
        c_log_warning ("Symbol not found: cim_plugin->vtable");

      if (!cim_plugin->vtable->create)
        c_log_warning ("Symbol not found: cim_plugin->vtable->create");

      if (!cim_plugin->vtable->destroy)
        c_log_warning ("Symbol not found: cim_plugin->vtable->destroy");

      dlclose (cim_handle);
      cim_plugin = nullptr;
      cim_handle = nullptr;

      goto fallback;
    }
  }

  if (cim_handle)
    return cim_plugin->vtable->create ();


  fallback:

  struct CimIcImpl* impl = c_malloc (sizeof (struct CimIcImpl));
  impl->vtable = &vtable;
  return impl;
}

void cim_ic_destroy (CimIcHandle ic)
{
  cim_ref_count--;

  if (cim_handle)
    cim_plugin->vtable->destroy (ic);
  else
    free (ic);

  if (cim_ref_count == 0)
  {
    if (cim_handle)
      dlclose (cim_handle);

    cim_handle = nullptr;
    cim_plugin = nullptr;
  }
}

void cim_ic_focus_in (CimIcHandle handle)
{
  if (cim_plugin->vtable->focus_in)
    cim_plugin->vtable->focus_in (handle);
}

void cim_ic_focus_out (CimIcHandle ic)
{
  if (cim_plugin->vtable->focus_out)
    cim_plugin->vtable->focus_out (ic);
}

void cim_ic_reset (CimIcHandle ic)
{
  if (cim_plugin->vtable->reset)
    cim_plugin->vtable->reset (ic);
}

bool cim_ic_filter_event (CimIcHandle ic, const CimEvent* event)
{
  if (cim_plugin->vtable->filter_event)
    cim_plugin->vtable->filter_event (ic, event);

  return false;
}

void cim_ic_set_cursor_pos (CimIcHandle ic, const CimRect* area)
{
  if (cim_plugin->vtable->set_cursor_pos)
    cim_plugin->vtable->set_cursor_pos (ic, area);
}

const CimPreedit* cim_ic_get_preedit (CimIcHandle ic)
{
  if (cim_plugin->vtable->get_preedit)
    cim_plugin->vtable->get_preedit (ic);

  c_log_critical ("cim_ic_get_preedit() must be implemented in the IM plugin.");

  static const CimPreedit preedit = { .text = "",
                                      .attrs = nullptr,
                                      .attrs_len = 0,
                                      .cursor_pos = 0 };
  return &preedit;
}

const CimCandidate* cim_ic_get_candidate (CimIcHandle ic)
{
  if (cim_plugin->vtable->get_candidate)
    cim_plugin->vtable->get_candidate (ic);

  c_log_critical ("get_candidate() must be implemented in the IM plugin.");
  return nullptr;
}

void cim_ic_set_callbacks (CimIcHandle ic,
                           const CimCallbacks* callbacks,
                           void* user_data)
{
  if (cim_plugin->vtable->set_callbacks)
    cim_plugin->vtable->set_callbacks (ic, callbacks, user_data);
  else
    c_log_critical ("set_callbacks() must be implemented in the IM plugin.");
}

void cim_ic_activate_candidate_item (CimIcHandle ic, uint32_t row, uint32_t col)
{
  if (cim_plugin->vtable->activate_candidate_item)
    cim_plugin->vtable->activate_candidate_item (ic, row, col);
}

void cim_ic_change_candidate_page (CimIcHandle ic, uint32_t page_index)
{
  if (cim_plugin->vtable->change_candidate_page)
    cim_plugin->vtable->change_candidate_page (ic, page_index);
}
