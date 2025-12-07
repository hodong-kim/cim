/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-str.c
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
#include "c-str.h"
#include "c-mem.h"
#include "c-array.h"
#include "c-log.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define C_STRING_MIN_CAPA  8

char *c_str_strip (const char *str)
{
  if (!str)
    return nullptr;

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
  char*   result = nullptr;

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

/**
 * c_str_sprintf:
 * @format: A printf-style format string.
 * @...:    Additional arguments as required by @format.
 *
 * Allocates a new string via c_malloc and fills it with the formatted
 * output. First, the required length is computed with vsnprintf, then
 * memory is allocated (including space for the null terminator), and finally
 * vsnprintf writes the output into the buffer.
 *
 * Note:
 * - This function assumes proper input; if vsnprintf returns a negative
 *   value, it is likely due to a formatting error and c_malloc will abort.
 * - c_malloc aborts if memory allocation fails.
 *
 * Returns: A pointer to the newly allocated string.
 */
char* c_str_sprintf (const char* restrict format, ...)
{
  char*   result;
  int     len;
  va_list ap;

  va_start (ap, format);
  len = vsnprintf (nullptr, 0, format, ap);
  va_end (ap);

  result = c_malloc (len + 1);

  va_start (ap, format);
  vsnprintf (result, len + 1, format, ap);
  va_end (ap);

  return result;
}

bool c_str_equal (const char *a, const char *b)
{
  if (!a)
  {
    a = "";
    c_log_warning ("The left argument is nullptr.");
  }

  if (!b)
  {
    b = "";
    c_log_warning ("The right argument is nullptr.");
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

char* c_strdup (const char* str)
{
  void* mem = strdup (str);

  if (mem)
    return mem;

  perror (__func__);
  abort ();
}

char* c_strndup (const char* str, size_t len)
{
  void* mem = strndup (str, len);

  if (mem)
    return mem;

  perror (__func__);
  abort ();
}

char **c_str_split (const char *str, char c)
{
  CArray *array;
  const char *p;
  const char *mark;

  array = c_array_new (nullptr, false);
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

  c_array_add (array, nullptr);

  return (char **) c_array_free (array);
}

static char* c_str_resize_capa (char* str, size_t* capa, size_t req_len)
{
  size_t old_capa = *capa;

  while (req_len > *capa)
    *capa *= 2;

  while (req_len + C_STRING_MIN_CAPA <= *capa / 2)
    *capa = *capa / 2;

  if (*capa != old_capa)
    str = c_realloc (str, *capa);

  return str;
}

char* c_str_rep (const char* s, const char* s1, const char* s2)
{
  char*  str;
  char*  p;
  size_t capa = C_STRING_MIN_CAPA;
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
  if (strv == nullptr)
    return nullptr;

  CArray *array = c_array_new (nullptr, false);

  for (int i = 0; strv[i]; i++)
    c_array_add (array, c_strdup (strv[i]));

  c_array_add (array, nullptr);

  return (char **) c_array_free (array);
}

void c_strv_free (char **strv)
{
  if (strv == nullptr)
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

/**
 * c_utf8_strncpy:
 * @dst:     Pointer to the destination buffer.
 * @src:     Pointer to a valid, null-terminated UTF-8 string.
 * @n_chars: The maximum number of UTF-8 code points to copy.
 *
 * Copies up to @n_chars UTF-8 code points from @src to @dst.
 * The copy stops when either @n_chars code points have been
 * copied or the end of the @src string is reached.
 * A null terminator is appended to @dst.
 *
 * Note:
 *  - @src must be a valid UTF-8 string.
 *
 * Returns: Nothing.
 */
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
    return nullptr;

  do {
    utf8--;
  } while ((*utf8 & 0b11000000) == 0b10000000);

  return (char *) utf8;
}

char *c_utf8_next_char (const char *utf8)
{
  if (!utf8)
    return nullptr;

  while (*utf8)
  {
    utf8++;

    if ((*utf8 & 0b11000000) != 0b10000000)
      break;
  }

  if (*utf8)
    return (char *) utf8;

  return nullptr;
}

/**
 * @brief Returns a pointer to the code point
 *        at a given index in a UTF-8 string.
 *
 * This function scans a null-terminated UTF-8 string and returns a pointer
 * to the start of the code point at the specified zero-based index. The
 * function assumes that the input string is non-NULL and valid UTF-8.
 * If the offset equals the number of code points in the string, a pointer
 * to the terminating null is returned.
 *
 * @param utf8            A valid, null-terminated UTF-8 string.
 * @param offset_in_chars The zero-based index of the target code point.
 *
 * @return const char*    A pointer to the beginning of the code point at
 *                        the specified index, or to the null terminator if
 *                        the offset equals the total number of code points.
 */
const char* c_utf8_offset_to_pointer (const char* utf8, size_t offset_in_chars)
{
  while (*utf8 && offset_in_chars > 0)
  {
    utf8++;
    if ((*utf8 & 0b11000000) != 0b10000000)
      offset_in_chars--;
  }

  return utf8;
}

char32_t *c_utf8_to_char32 (const char *utf8)
{
  if (!utf8)
    return nullptr;

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

static int c_1char32_to_utf8_buf (char32_t char32, char* buf)
{
  int len = 0;

  if (char32 == 0)
  {
    /* do nothing */
  }
  else if (char32 < 0x0080)
  { /* 1-byte */
    buf[len++] = char32;
  }
  else if (char32 < 0x0800)
  { /* 2-byte */
    buf[len++] = 0b11000000 | (char32 >> 6);
    buf[len++] = 0b10000000 | (char32 & 0b00111111);
  }
  else if (char32 < 0x10000)
  { /* 3-byte */
    buf[len++] = 0b11100000 | (char32 >> 12);
    buf[len++] = 0b10000000 | (char32 >>  6 & 0b00111111);
    buf[len++] = 0b10000000 | (char32       & 0b00111111);
  }
  else if (char32 < 0x110000)
  { /* 4-byte */
    buf[len++] = 0b11110000 | (char32 >> 18);
    buf[len++] = 0b10000000 | (char32 >> 12 & 0b00111111);
    buf[len++] = 0b10000000 | (char32 >>  6 & 0b00111111);
    buf[len++] = 0b10000000 | (char32       & 0b00111111);
  }
  else
  {
    c_log_warning ("Cannot convert 0x%x to UTF-8.", char32);
    len += c_1char32_to_utf8_buf (0xfffd, buf);
  }

  buf[len] = 0;

  return len;
}

int c_char32_to_utf8_buf (char32_t* char32, char* buf, int n_char32s)
{
  if (!char32)
    return 0;

  int len = 0;

  for (int i = 0; (n_char32s < 0 || i < n_char32s) && char32[i]; i++)
    len += c_1char32_to_utf8_buf (char32[i], buf + len);

  buf[len] = '\0';

  return len;
}

/**
 * @brief Convert a char32_t string to a UTF-8 encoded string.
 *
 * This function converts a UTF-32 input string to a UTF-8 string.
 * If n_char32s is negative, the input is assumed to be a null-
 * terminated string. Any invalid Unicode code point (greater than
 * 0x10FFFF or in the UTF-16 surrogate range) is replaced with U+FFFD.
 *
 * Memory is allocated for the maximum size (count * 4 + 1) and then
 * reallocated to the actual size used. If any overflow occurs during
 * counting or memory size calculation, the function returns nullptr.
 *
 * @param char32    Input char32_t string pointer.
 * @param n_char32s Number of code points to convert. If negative,
 *                  the input is assumed to be null-terminated.
 *
 * @return Dynamically allocated UTF-8 string (free() must be called)
 *         or nullptr on failure. In particular, if an overflow occurs,
 *         nullptr is returned.
 */
char* c_char32_to_utf8 (const char32_t* char32, int n_char32s)
{
  if (!char32)
    return nullptr;

  size_t count = 0;
  if (n_char32s < 0)
  {
    const char32_t* p = char32;
    while (*p)
    {
      /* Check for potential overflow in count */
      if (count == SIZE_MAX)
        return nullptr;
      count++;
      p++;
    }
  }
  else
  {
    count = (size_t) n_char32s;
  }

  /* Check for potential overflow:
     (count * 4 + 1) must not exceed SIZE_MAX. */
  if (count > (SIZE_MAX - 1) / 4)
    return nullptr;

  size_t max_size = count * 4 + 1;
  char* result = (char*) malloc (max_size);
  if (!result)
    return nullptr;

  char* dst = result;
  for (size_t i = 0; i < count; i++)
  {
    char32_t cp = char32[i];

    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
      cp = 0xFFFD;

    if (cp < 0x80)
    {
      *dst++ = (char) cp;
    }
    else if (cp < 0x800)
    {
      *dst++ = (char) (0xC0 | (cp >> 6));
      *dst++ = (char) (0x80 | (cp & 0x3F));
    }
    else if (cp < 0x10000)
    {
      *dst++ = (char) (0xE0 | (cp >> 12));
      *dst++ = (char) (0x80 | ((cp >> 6) & 0x3F));
      *dst++ = (char) (0x80 | (cp & 0x3F));
    }
    else
    {
      *dst++ = (char) (0xF0 | (cp >> 18));
      *dst++ = (char) (0x80 | ((cp >> 12) & 0x3F));
      *dst++ = (char) (0x80 | ((cp >> 6) & 0x3F));
      *dst++ = (char) (0x80 | (cp & 0x3F));
    }
  }

  *dst = '\0';

  size_t used_size = (size_t) (dst - result + 1);
  char* tmp = realloc (result, used_size);
  /* On realloc failure, the original memory block is still valid. */
  if (tmp)
      result = tmp;
  return result;
}

CString* c_string_new (const char* str, bool free_str)
{
  CString* string = c_malloc (sizeof (CString));

  string->capa     = C_STRING_MIN_CAPA;
  string->str      = c_malloc (string->capa);
  string->len      = 0;
  string->free_str = free_str;

  c_string_assign (string, str);

  return string;
}

char *c_string_free (CString *string)
{
  if (!string)
    return nullptr;

  char *str;

  string->str = c_realloc (string->str, string->len + 1);
  str = string->str;

  if (string->free_str)
  {
    free (str);
    str = nullptr;
  }

  free (string);

  return str;
}

static void c_string_resize_capa (CString *string, size_t req_len)
{
  size_t old_capa = string->capa;

  while (req_len > string->capa)
    string->capa *= 2;

  while (req_len + C_STRING_MIN_CAPA <= string->capa / 2)
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
