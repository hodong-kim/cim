/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-candidate.h
 * This file is part of Nimf.
 *
 * Copyright (C) 2015-2023 Hodong Kim <hodong@nimfsoft.art>
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
#ifndef __C_CANDIDATE_H__
#define __C_CANDIDATE_H__

#include <gtk/gtk.h>
#include <cim.h>

G_BEGIN_DECLS

/*****************************************************************************/
#if GTK_CHECK_VERSION (3, 12, 0)
typedef struct _TableItem       TableItem;
typedef struct _TableItemClass  TableItemClass;

struct _TableItem
{
  GtkBin item;
  GdkWindow* event_window;
};

struct _TableItemClass
{
  GtkBinClass parent_class;
};

GType      table_item_get_type ();
GtkWidget* table_item_new ();
void       table_item_select   (TableItem* item);
void       table_item_deselect (TableItem* item);
#endif
/*****************************************************************************/

#include <stdbool.h>

#if (defined(__GNUC__) && (GCC_VERSION < 13)) || \
    (defined(__clang__) && (__clang_major__ < 16))
enum _CCandidateCbType {
  C_CANDIDATE_CB_CHANGE_PAGE,
  C_CANDIDATE_CB_ACTIVATE_ITEM,
  C_CANDIDATE_CB_N_TYPES
};
typedef int CCandidateCbType;
#else
enum _CCandidateCbType : int {
  C_CANDIDATE_CB_CHANGE_PAGE,
  C_CANDIDATE_CB_ACTIVATE_ITEM,
  C_CANDIDATE_CB_N_TYPES
};
typedef enum _CCandidateCbType CCandidateCbType;
#endif

typedef struct _CCandidate           CCandidate;
typedef struct _CCandidateCallbacks  CCandidateCallbacks;

struct _CCandidateCallbacks
{
  void (*change_page) (CCandidate* ccandidate,
                       int page_index,
                       void* user_data);
  void (*activate_item) (CCandidate* ccandidate,
                         int row,
                         int col,
                         void* user_data);
};

struct _CCandidate
{
  GtkWidget* window;
  GtkWidget* table;
  GtkWidget* scrollbar;
  GtkWidget* entry;
#if !GTK_CHECK_VERSION (3, 12, 0)
  GtkWidget** items;
#endif
  int cell_height;
  int n_rows;
  int n_cols;
  int n_pages;
  int page_index;
  bool show_aux;
  CCandidateCallbacks cb;
  void* cb_user_data[C_CANDIDATE_CB_N_TYPES];
};

CCandidate* c_candidate_new ();
void c_candidate_free   (CCandidate* ccandidate);
void c_candidate_show   (CCandidate* ccandidate,
                         int n_rows,
                         int n_cols,
                         bool show_aux);
void c_candidate_move   (CCandidate* ccandidate, int x, int y);
void c_candidate_hide   (CCandidate* ccandidate);
void c_candidate_change (CCandidate* ccandidate, const CimCandidate* candidate);
void c_candidate_select (CCandidate* ccandidate, const CimSelection* selection);
void c_candidate_set_callbacks (CCandidate* ccandidate, ...);

G_END_DECLS

#endif /* _C_CANDIDATE_H_ */
