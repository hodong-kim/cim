/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-mem.c
 * This file is part of Clair.
 *
 * Copyright (C) 2021,2022 Hodong Kim <hodong@nimfsoft.art>
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
#include "c-mem.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void* c_malloc (size_t size)
{
  if (!size)
    return NULL;

  void* mem = malloc (size);

  if (mem)
    return mem;

  perror (__PRETTY_FUNCTION__);
  abort ();
}

void* c_calloc (size_t number, size_t size)
{
  if (!(number * size))
    return NULL;

  void* mem = calloc (number, size);

  if (mem)
    return mem;

  perror (__PRETTY_FUNCTION__);
  abort ();
}

void* c_realloc (void* ptr, size_t size)
{
  if (!size)
  {
    free (ptr);
    return NULL;
  }

  void* mem = realloc (ptr, size);

  if (mem)
    return mem;

  perror (__PRETTY_FUNCTION__);
  abort ();
}

void *c_memdup (const void *src, size_t size)
{
  if (!size)
    return NULL;

  void *dst = c_malloc (size);
  return memcpy (dst, src, size);
}

CRef *c_ref_new (uint8_t *data, CFreeFunc data_free_func)
{
  CRef *ref = c_malloc (sizeof (CRef));

  ref->data      = data;
  ref->free_func = data_free_func;
  ref->count = 1;

  return ref;
}

void c_ref_inc (CRef *ref)
{
  if (ref == NULL)
    return;

  ref->count++;
}

void c_ref_dec (CRef *ref)
{
  if (ref == NULL)
    return;

  ref->count--;

  if (ref->count == 0)
  {
    if (ref->free_func)
      ref->free_func (ref->data);

    free (ref);
  }
}
