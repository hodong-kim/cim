/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-str.c
 * This file is part of Clair.
 *
 * Copyright (C) 2020-2024 Hodong Kim <hodong@nimfsoft.art>
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
#include "c-str.h"
#include "c-mem.h"
#include "c-array.h"
#include "c-log.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define C_STRING_DEFAULT_CAPA  16

char *c_str_strip (const char *str)
{
  if (!str)
    return NULL;

  while (*str && isspace (*str))
    str++;

  char *new_str = c_strdup (str);

  c_str_chomp (new_str);

  return new_str;
}

bool c_str_chomp (char *str)
{
  char *p;
  bool  retval = false;

  p = str + strlen (str);

  while (p > str)
  {
    p--;

    if (*p == '\t' ||
        *p == '\n' ||
        *p == '\v' ||
        *p == '\f' ||
        *p == '\r' ||
        *p == ' ')
    {
      *p = 0;
      retval = true;
    }
    else
    {
      break;
    }
  }

  return retval;
}

bool c_str_contains_c (const char *str, char c)
{
  for (const char *p = str; *p; p++)
    if (*p == c)
      return true;

  return false;
}

bool c_str_contains_s (const char *str, char *s)
{
  if (!*s)
    return false;

  return strstr (str, s);
}

/* Returns a newly allocated string.
 * example:
 * linkpath = c_str_join (autostart_dir, "/nimf.desktop", nullptr);
 * Free it with free() */
char* c_str_join (const char *str, ...)
{
  va_list ap;
  size_t  offset = 0;
  char*   result = NULL;

  va_start (ap, str);

  for (const char* s = str; s != nullptr; s = va_arg (ap, const char*))
  {
    size_t len = strlen (s);
    result = c_realloc (result, offset + len + 1);
    memcpy (result + offset, s, len);
    offset = offset + len;
  }

  va_end (ap);

  if (result)
    result[offset] = '\0';

  return result;
}

char* c_str_sprintf (const char* restrict format, ...)
{
  char*   result;
  int     len;
  va_list ap;

  va_start (ap, format);
  len = vsnprintf (NULL, 0, format, ap);
  va_end (ap);

  result = c_malloc (len + 1);

  va_start (ap, format);
  vsprintf (result, format, ap);
  va_end   (ap);

  return result;
}

bool c_str_equal (const char *a, const char *b)
{
  if (!a)
  {
    a = "";
    c_log_warning ("The left argument is NULL.");
  }

  if (!b)
  {
    b = "";
    c_log_warning ("The right argument is NULL.");
  }

  return strcmp (a, b) == 0;
}

bool c_str_starts_with (const char *str, const char *prefix)
{
  while (*prefix)
  {
    if (*str != *prefix)
      return false;

    str++;
    prefix++;
  }

  return true;
}

bool c_str_ends_with (const char *str, const char *suffix)
{
  size_t len1 = strlen (str);
  size_t len2 = strlen (suffix);

  if (len1 < len2)
    return false;

  return !strncmp (str + len1 - len2, suffix, len2);
}

char *c_strdup (const char *str)
{
  void *mem = strdup (str);

  if (mem)
    return mem;

  perror (__PRETTY_FUNCTION__);
  abort ();
}

char *c_strndup (const char *str, size_t len)
{
  void *mem = strndup (str, len);

  if (mem)
    return mem;

  perror (__PRETTY_FUNCTION__);
  abort ();
}

char **c_str_split (const char *str, char c)
{
  CArray *array;
  const char *p;
  const char *mark;

  array = c_array_new (NULL, false);
  p = str;

  while (1)
  {
    mark = strchr (p, c);

    if (mark)
    {
      c_array_add (array, c_strndup (p, mark - p));
      p = mark + 1;
    }
    else
    {
      c_array_add (array, c_strdup (p));
      break;
    }
  }

  c_array_add (array, NULL);

  return (char **) c_array_free (array);
}

static char* c_str_resize_capa (char* str, size_t* capa, size_t req_len)
{
  size_t old_capa = *capa;

  while (req_len > *capa)
    *capa *= 2;

  while (req_len + C_STRING_DEFAULT_CAPA < *capa / 4)
    *capa = *capa / 2;

  if (*capa != old_capa)
    str = c_realloc (str, *capa);

  return str;
}

char* c_str_rep (const char* s, const char* s1, const char* s2)
{
  char*  str;
  char*  p;
  size_t capa = C_STRING_DEFAULT_CAPA;
  size_t str_len = 0;
  size_t s1_len = strlen (s1);
  size_t s2_len = strlen (s2);

  str = c_malloc (capa);

  while ((p = strstr (s, s1)))
  {
    size_t diff = p - s;

    if (diff)
    {
      str = c_str_resize_capa (str, &capa, str_len + diff + 1);
      /* Copy the front part. */
      memcpy (str + str_len, s, diff);
      str_len = str_len + diff;
    }

    /* Insert the middle part. */
    str = c_str_resize_capa (str, &capa, str_len + s2_len + 1);
    memcpy (str + str_len, s2, s2_len);
    str_len = str_len + s2_len;

    s = p + s1_len;
  }

  /* Copy the back part. */
  size_t s_len = strlen (s);
  str = c_str_resize_capa (str, &capa, str_len + s_len + 1);
  memcpy (str + str_len, s, s_len);
  str_len = str_len + s_len;

  str[str_len] = '\0';
  str = c_realloc (str, str_len + 1);

  return str;
}

char **c_strv_dup (char **strv)
{
  if (strv == NULL)
    return NULL;

  CArray *array = c_array_new (NULL, false);

  for (int i = 0; strv[i]; i++)
    c_array_add (array, c_strdup (strv[i]));

  c_array_add (array, NULL);

  return (char **) c_array_free (array);
}

void c_strv_free (char **strv)
{
  if (strv == NULL)
    return;

  for (int i = 0; strv[i]; i++)
    free (strv[i]);

  free (strv);
}

unsigned c_strv_len (char **strv)
{
  unsigned i = 0;
  while (strv[i]) i++;
  return i;
}

bool c_strv_contains (const char **strv, const char *str)
{
  for (int i = 0; strv[i]; i++)
    if (c_str_equal (strv[i], str))
      return true;

  return false;
}

char* c_strv_join (const char** strv, const char* separator)
{
  CString* string = c_string_new ("", false);

  for (int i = 0; strv[i]; i++)
  {
    if (i && separator)
      c_string_append (string, separator);

    c_string_append (string, strv[i]);
  }

  return c_string_free (string);
}

size_t c_utf8_strlen (const char *utf8)
{
  size_t len = 0;

  while (*utf8)
  {
    if ((*utf8 & 0b11000000) != 0b10000000)
      len++;

    utf8++;
  }

  return len;
}

size_t c_utf8_strnlen (const char *utf8, size_t max_n_bytes)
{
  size_t len = 0;
  const char *p = utf8;

  while (*p && p - utf8 < max_n_bytes)
  {
    if ((*p & 0b11000000) != 0b10000000)
      len++;

    p++;
  }

  return len;
}

void c_utf8_strncpy (char * restrict dst,
                     const char * restrict src,
                     size_t n_chars)
{
  if (!n_chars)
    return;

  while (*src && n_chars)
  {
    *dst = *src;

    dst++;
    src++;

    if ((*src & 0b11000000) != 0b10000000)
      n_chars--;
  }

  *dst = 0;
}

char *c_utf8_prev_char (const char *utf8)
{
  if (!utf8)
    return NULL;

  do {
    utf8--;
  } while ((*utf8 & 0b11000000) == 0b10000000);

  return (char *) utf8;
}

char *c_utf8_next_char (const char *utf8)
{
  if (!utf8)
    return NULL;

  while (*utf8)
  {
    utf8++;

    if ((*utf8 & 0b11000000) != 0b10000000)
      break;
  }

  if (*utf8)
    return (char *) utf8;

  return NULL;
}

char *c_utf8_offset_to_pointer (const char *utf8, size_t offset_in_chars)
{
  while (*utf8 && offset_in_chars > 0)
  {
    utf8++;
    if ((*utf8 & 0b11000000) != 0b10000000)
      offset_in_chars--;
  }

  return (char *) utf8;
}

char32_t *c_utf8_to_char32 (const char *utf8)
{
  if (!utf8)
    return NULL;

  size_t len  = 0;
  size_t capa = 8;
  char32_t *c32 = c_malloc (capa * sizeof (char32_t));

  while (*utf8)
  {
    if (capa < len + 5)
    {
      capa *= 2;
      c32 = c_realloc (c32, capa * sizeof (char32_t));
    }

    if ((*utf8 & 0b10000000) == 0)
    {
      c32[len++] = *utf8++;
    }
    else
    {
      if ((*utf8 & 0b11110000) == 0b11110000)
      {
        c32[len]    = (*utf8++ & 0b00000111) << 18;
        c32[len]   |= (*utf8++ & 0b00111111) << 12;
        c32[len]   |= (*utf8++ & 0b00111111) << 6;
        c32[len++] |= (*utf8++ & 0b00111111);
      }
      else if ((*utf8 & 0b11100000) == 0b11100000)
      {
        c32[len]    = (*utf8++ & 0b00001111) << 12;
        c32[len]   |= (*utf8++ & 0b00111111) << 6;
        c32[len++] |= (*utf8++ & 0b00111111);
      }
      else if ((*utf8 & 0b11000000) == 0b11000000)
      {
        c32[len]    = (*utf8++ & 0b00011111) << 6;
        c32[len++] |= (*utf8++ & 0b00111111);
      }
    }
  }

  c32 = c_realloc (c32, (len + 1) * sizeof (char32_t));
  c32[len] = 0;

  return c32;
}

int c_char32_strcmp (const char32_t * restrict a, const char32_t * restrict b)
{
  while (*a)
  {
    if (*a != *b)
      break;

    a++;
    b++;
  }

  return *a - *b;
}

int c_utf8_collate (const char * restrict s1, const char * restrict s2)
{
  char32_t *a, *b;

  a = c_utf8_to_char32 (s1);
  b = c_utf8_to_char32 (s2);

  int retval = c_char32_strcmp (a, b);

  free (a);
  free (b);

  return retval;
}

int c_char32_to_utf8_with_buf (char32_t char32, char *utf8)
{
  int len = 0;

  if (char32 == 0)
  {
    /* do nothing */
  }
  else if (char32 < 0x0080)
  { /* 1-byte */
    utf8[len++] = char32;
  }
  else if (char32 < 0x0800)
  { /* 2-byte */
    utf8[len++] = 0b11000000 | (char32 >> 6);
    utf8[len++] = 0b10000000 | (char32 & 0b00111111);
  }
  else if (char32 < 0x10000)
  { /* 3-byte */
    utf8[len++] = 0b11100000 | (char32 >> 12);
    utf8[len++] = 0b10000000 | (char32 >>  6 & 0b00111111);
    utf8[len++] = 0b10000000 | (char32       & 0b00111111);
  }
  else if (char32 < 0x110000)
  { /* 4-byte */
    utf8[len++] = 0b11110000 | (char32 >> 18);
    utf8[len++] = 0b10000000 | (char32 >> 12 & 0b00111111);
    utf8[len++] = 0b10000000 | (char32 >>  6 & 0b00111111);
    utf8[len++] = 0b10000000 | (char32       & 0b00111111);
  }
  else
  {
    c_log_warning ("Cannot convert 0x%x to UTF-8.", char32);
    len += c_char32_to_utf8_with_buf (0xfffd, utf8);
  }

  utf8[len] = 0;

  return len;
}

char *c_char32_to_utf8 (const char32_t *char32, ssize_t n_char32s)
{
  if (!char32)
    return NULL;

  char *utf8;
  int   len  = 0;
  int   capa = 8;

  utf8 = c_malloc (capa * sizeof (char));
  utf8[0] = 0;

  for (int i = 0; (n_char32s < 0 || i < n_char32s) && char32[i]; i++)
  {
    if (capa < len + 5)
    {
      capa *= 2;
      utf8 = c_realloc (utf8, capa * sizeof (char));
    }

    len += c_char32_to_utf8_with_buf (char32[i], utf8 + len);
  }

  return c_realloc (utf8, len + 1);
}

CString* c_string_new (const char* str, bool free_str)
{
  CString* string = c_malloc (sizeof (CString));

  string->capa     = C_STRING_DEFAULT_CAPA;
  string->str      = c_malloc (string->capa);
  string->len      = 0;
  string->free_str = free_str;

  c_string_assign (string, str);

  return string;
}

char *c_string_free (CString *string)
{
  if (!string)
    return NULL;

  char *str;

  string->str = c_realloc (string->str, string->len + 1);
  str = string->str;

  if (string->free_str)
  {
    free (str);
    str = NULL;
  }

  free (string);

  return str;
}

static void c_string_resize_capa (CString *string, size_t req_len)
{
  size_t old_capa = string->capa;

  while (req_len > string->capa)
    string->capa *= 2;

  while (req_len + C_STRING_DEFAULT_CAPA < string->capa / 4)
    string->capa = string->capa / 2;

  if (string->capa != old_capa)
    string->str = c_realloc (string->str, string->capa);
}

void c_string_append (CString *string, const char *str)
{
  size_t len = strlen (str);

  if (len > 0)
  {
    c_string_resize_capa (string, string->len + len + 1);
    memcpy (string->str + string->len, str, len + 1);
    string->len += len;
  }
}

void c_string_append_c (CString *string, char c)
{
  string->len++;
  c_string_resize_capa (string, string->len + 1);
  string->str[string->len - 1] = c;
  string->str[string->len] = 0;
}

void c_string_assign (CString *string, const char *str)
{
  string->len = strlen (str);
  c_string_resize_capa (string, string->len + 1);
  memcpy (string->str, str, string->len + 1);
}

void c_string_assign_n (CString* string, const char* str, size_t n)
{
  c_string_resize_capa (string, n + 1);
  memcpy (string->str, str, n);
  string->str[n] = '\0';
}

void c_string_insert_c (CString *string, ssize_t pos, char c)
{
  /* save */
  uint8_t *save = c_malloc (string->len - pos);
  memcpy (save, string->str + pos, string->len - pos);
  /* realloc */
  string->len += 1;
  c_string_resize_capa (string, string->len + 1);
  /* copy */
  memcpy (string->str + pos, &c, 1);
  memcpy (string->str + pos + 1, save, string->len - 1 - pos);
  string->str[string->len] = 0;

  free (save);
}

void c_string_erase (CString *string, ssize_t pos, ssize_t len)
{
  if (len == 0)
    return;

  if (pos > string->len)
  {
    c_log_warning ("pos  >  string->len");
    return;
  }

  if (len == -1)
  {
    string->len = pos;
    c_string_resize_capa (string, string->len + 1);
    string->str[string->len] = 0;
    return;
  }

  memmove (string->str + pos,
           string->str + pos + len,
           string->len - pos - len);
  string->len -= len;
  c_string_resize_capa (string, string->len + 1);
  string->str[string->len] = 0;
}

void c_string_insert (CString *string, ssize_t pos, const char *str)
{
  /* save */
  uint8_t *save = c_malloc (string->len - pos);
  memcpy (save, string->str + pos, string->len - pos);
  /* realloc */
  size_t len = strlen (str);
  string->len += len;
  c_string_resize_capa (string, string->len + 1);
  /* copy */
  memcpy (string->str + pos, str, len);
  memcpy (string->str + pos + len, save, string->len - len - pos);
  string->str[string->len] = 0;

  free (save);
}

void c_string_overwrite (CString *string, size_t pos, const char *str)
{
  size_t len = strlen (str);

  if (len > string->len - pos)
  {
    string->len = len + pos;
    c_string_resize_capa (string, string->len + 1);
  }

  memcpy (string->str + pos, str, len);
  string->str[string->len] = 0;
}
