/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-macros.h
 * This file is part of Clair.
 *
 * Copyright (C) 2019-2025 Hodong Kim <hodong@nimfsoft.art>
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
#ifndef __C_MACROS_H__
#define __C_MACROS_H__

#ifdef __cplusplus
  #define C_BEGIN_DECLS extern "C" {
  #define C_END_DECLS   }
#else
  #define C_BEGIN_DECLS
  #define C_END_DECLS
#endif

#define C_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define C_MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define C_UINT_TO_VOIDP(u)  ((void*) (uintptr_t) (u))
#define C_INT_TO_VOIDP(i)   ((void*) (intptr_t)  (i))
#define C_VOIDP_TO_INT(p)   ((int)   (intptr_t)  (p))

#ifndef N_
  #define N_(text)  (text) /* only mark for translation */
#endif

#ifndef __cplusplus
#if (defined(__GNUC__) && (__GNUC__ < 13)) || \
    (defined(__clang__) && (__clang_major__ < 16))
#define nullptr ((void*) 0)
#endif
#endif

#define C_N_ELEMENTS(arr)  (sizeof (arr) / sizeof ((arr)[0]))
#define C_PADDING(offset, align)  (-(offset) & ((align) - 1))

#endif /* __C_MACROS_H__ */
