/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-array.c
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
#include <stdlib.h>
#include "c-array.h"
#include "c-mem.h"

typedef struct _CArrayPrivate CArrayPrivate;
struct _CArrayPrivate {
  void     **data;
  unsigned   len;
  CFreeFunc  free_func;
  unsigned   capa;
  bool       free_data;
};

CArray *c_array_new (CFreeFunc free_func, bool free_data)
{
  CArrayPrivate *array;

  array = c_malloc (sizeof (CArrayPrivate));
  array->capa      = 4;
  array->data      = c_malloc (array->capa * sizeof (void *));
  array->len       = 0;
  array->free_func = free_func;
  array->free_data = free_data;

  return (CArray *) array;
}

void **c_array_free (CArray *array)
{
  CArrayPrivate *priv = (CArrayPrivate *) array;

  if (priv->free_data)
  {
    if (priv->free_func)
      for (int i = 0; i < array->len; i++)
        if (array->data[i])
          priv->free_func (array->data[i]);

    free (array->data);
    free (array);
    return NULL;
  }

  if (array->len != priv->capa)
    array->data = c_realloc (array->data, array->len * sizeof (void *));

  void **data = array->data;
  free (array);
  return data;
}

void c_array_clear (CArray *array)
{
  CArrayPrivate *priv = (CArrayPrivate *) array;

  if (priv->free_func)
    for (int i = 0; i < array->len; i++)
      if (array->data[i])
        priv->free_func (array->data[i]);

  priv->capa  = 1;
  array->data = c_realloc (priv->data, priv->capa * sizeof (void *));
  array->len  = 0;
}

void c_array_add (CArray *array, void *data)
{
  CArrayPrivate *priv = (CArrayPrivate *) array;

  if (array->len == priv->capa)
  {
    priv->capa  = priv->capa * 2;
    array->data = c_realloc (array->data, sizeof (void *) * priv->capa);
  }

  array->data[array->len] = data;
  array->len++;
}

bool c_array_remove_index (CArray *array, unsigned i)
{
  if (!array->len)
    return false;

  CArrayPrivate *priv = (CArrayPrivate *) array;

  if (priv->free_func)
    priv->free_func (array->data[i]);

  array->data[i] = array->data[array->len - 1];
  array->len--;

  if (array->len < priv->capa / 4)
  {
    priv->capa  = priv->capa / 2;
    array->data = c_realloc (array->data, sizeof (void *) * priv->capa);
  }

  return true;
}

bool c_array_remove (CArray *array, void *data)
{
  for (int i = 0; i < array->len; i++)
    if (array->data[i] == data)
      return c_array_remove_index (array, i);

  return false;
}

void *c_array_index (CArray *array, unsigned i)
{
  if (!array->len)
    return NULL;

  return array->data[i];
}

bool c_array_find (CArray     *array,
                   const void *needle,
                   CEqualFunc  equal_func,
                   unsigned   *index)
{
  for (unsigned i = 0; i < array->len; i++)
  {
    if (!equal_func)
    {
      if (needle == array->data[i])
      {
        if (index)
          *index = i;

        return true;
      }
    }
    else if (equal_func (array->data[i], needle))
    {
      if (index)
        *index = i;

      return true;
    }
  }

  return false;
}

void c_array_sort (CArray *array, CCompareFunc compare)
{
  qsort (array->data, array->len, sizeof (void *), compare);
}
