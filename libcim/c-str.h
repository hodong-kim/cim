/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-str.h
 * This file is part of Clair.
 *
 * Copyright (C) 2020-2025 Hodong Kim <hodong@nimfsoft.art>
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
#ifndef __C_STR_H__
#define __C_STR_H__

#include "c-macros.h"
#include <stdbool.h>
#include <stddef.h>
#include <uchar.h>
#include <sys/types.h>

#ifdef __cplusplus
#define restrict __restrict__
#endif

C_BEGIN_DECLS

char     *c_strdup          (const char *str);
char     *c_strndup         (const char *str, size_t len);

char     *c_str_strip       (const char *str);
bool      c_str_chomp       (char *str);
bool      c_str_contains_c  (const char *str, char c);
bool      c_str_contains_s  (const char *str, char *s);
bool      c_str_equal       (const char *a,   const char *b);
/* terminated with nullptr */
char     *c_str_join        (const char *str, ...);
bool      c_str_starts_with (const char *str, const char *prefix);
bool      c_str_ends_with   (const char *str, const char *suffix);
char     *c_str_rep         (const char *s, const char *s1, const char *s2);
char    **c_str_split       (const char *str, char delim);
char     *c_str_sprintf     (const char * restrict format, ...);

char    **c_strv_dup        (char **strv);
void      c_strv_free       (char **strv);
unsigned  c_strv_len        (char **strv);
bool      c_strv_contains   (const char **strv, const char *str);
char*     c_strv_join       (const char** strv, const char* separator);

size_t    c_utf8_strlen     (const char *utf8);
size_t    c_utf8_strnlen    (const char *utf8, size_t max_n_bytes);
void      c_utf8_strncpy    (char * restrict dst,
                             const char * restrict src,
                             size_t n_chars);
char     *c_utf8_prev_char  (const char *utf8);
char     *c_utf8_next_char  (const char *utf8);
const char* c_utf8_offset_to_pointer (const char *utf8, size_t offset_in_chars);
char     *c_char32_to_utf8  (const char32_t *char32, int n_char32s);
int       c_char32_to_utf8_buf (char32_t* char32, char* buf, int n_char32s);
char32_t* c_utf8_to_char32  (const char* utf8);
int       c_utf8_collate    (const char* restrict s1,
                             const char* restrict s2);

typedef struct _CString  CString;
struct _CString {
  char*  str;
  size_t len;
  size_t capa;
  bool   free_str;
};

CString* c_string_new       (const char* str, bool free_str);
char*    c_string_free      (CString* string);
void     c_string_append    (CString* string, const char* str);
void     c_string_append_c  (CString* string, char c);
void     c_string_assign    (CString* string, const char* str);
void     c_string_assign_n  (CString* string, const char* str, size_t n);
void     c_string_insert_c  (CString* string, ssize_t pos, char c);
void     c_string_erase     (CString* string, ssize_t pos, ssize_t len);
void     c_string_insert    (CString* string, ssize_t pos, const char* str);
void     c_string_overwrite (CString* string, size_t  pos, const char* str);

C_END_DECLS

#endif /* __C_STR_H__ */
