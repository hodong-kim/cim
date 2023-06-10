/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-array.h
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
#ifndef __C_ARRAY_H__
#define __C_ARRAY_H__

#include "c-macros.h"
#include "c-types.h"
#include <stdbool.h>

C_BEGIN_DECLS

typedef struct _CArray CArray;
struct _CArray {
  void     **data;
  unsigned   len;
};

CArray *c_array_new          (CFreeFunc free_func, bool free_data);
void  **c_array_free         (CArray *array);
void    c_array_clear        (CArray *array);
void    c_array_add          (CArray *array, void *data);
bool    c_array_remove_index (CArray *array, unsigned i);
bool    c_array_remove       (CArray *array, void *data);
void   *c_array_index        (CArray *array, unsigned i);
void    c_array_sort         (CArray *array, CCompareFunc compare);
bool    c_array_find         (CArray     *array,
                              const void *needle,
                              CEqualFunc  equal_func,
                              unsigned   *index);
C_END_DECLS

#endif /* __C_ARRAY_H__ */
