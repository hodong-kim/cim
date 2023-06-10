/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-types.h
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
#ifndef __CLAIR_TYPES_H__
#define __CLAIR_TYPES_H__

#include <stdbool.h>
#include "c-macros.h"

C_BEGIN_DECLS

typedef void (* CFreeFunc)     (void *data);
typedef int  (* CCompareFunc)  (const void *a, const void *b);
typedef bool (* CEqualFunc)    (const void *a, const void *b);
typedef void (* CCallback)     ();

typedef struct _CNode  CNode;
struct _CNode {
  CNode *next;
  CNode *prev;
  void  *data;
};

typedef struct
{
  int x;
  int y;
  int width;
  int height;
} CRect;

typedef struct {
  CCallback  func;
  void      *user_data;
} CCallbackInfo;

C_END_DECLS

#endif /* __CLAIR_TYPES_H__ */
