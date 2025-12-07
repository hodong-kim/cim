/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * test-cim.c
 * This file is part of Cim.
 *
 * Copyright (C) 2023 Hodong Kim <hodong@nimfsoft.art>
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

#include <stdio.h>
#include "cim.h"

void cb_commit (CimIcHandle ic, const char* text, void* user_data)
{
  puts (text);
}

static CimCallbacks callbacks = {
  .commit = cb_commit
};

int main ()
{
  CimIcHandle ic = cim_ic_create ();

  cim_ic_set_callbacks (ic, &callbacks, NULL);

  cim_ic_focus_in (ic);

  /* CimEvent event = { 0 }; */
  CimEvent event = { 0, 0, 0xff31, 108 };
  cim_ic_filter_event (ic, &event);

  for (int i = 0; i < 1000; i++)
  {
    event.keyval  = 100;
    event.keycode = 40;
    cim_ic_filter_event (ic, &event);

    event.keyval  = 107;
    event.keycode = 45;
    cim_ic_filter_event (ic, &event);

    event.keyval  = 115;
    event.keycode = 39;
    cim_ic_filter_event (ic, &event);

    event.keyval  = 115;
    event.keycode = 39;
    cim_ic_filter_event (ic, &event);

    event.keyval  = 117;
    event.keycode = 30;
    cim_ic_filter_event (ic, &event);

    event.keyval  = 100;
    event.keycode = 40;
    cim_ic_filter_event (ic, &event);
  }

  cim_ic_focus_out (ic);

  cim_ic_destroy (ic);

  return 0;
}
