/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-mem.h
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
#ifndef __C_MEM_H__
#define __C_MEM_H__

#include <stddef.h>
#include "c-macros.h"
#include <stdint.h>
#include "c-types.h"

C_BEGIN_DECLS

typedef struct
{
  uint8_t   *data;
  unsigned   count;
  CFreeFunc  free_func;
} CRef;

CRef *c_ref_new (uint8_t *data, CFreeFunc free_func);
void  c_ref_inc (CRef *ref);
void  c_ref_dec (CRef *ref);
void *c_malloc  (size_t size);
void *c_calloc  (size_t number, size_t size);
void *c_realloc (void *ptr, size_t size);
void *c_memdup  (const void *src, size_t size);

C_END_DECLS

#endif /* __C_MEM_H__ */
