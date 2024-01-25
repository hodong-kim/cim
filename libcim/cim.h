/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * cim.h
 * This file is part of Cim.
 *
 * Copyright (C) 2023,2024 Hodong Kim <hodong@nimfsoft.art>
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
#ifndef __CIM_H__
#define __CIM_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Version policy
 *
 * If binary compatibility is broken:
 *
 *     major = major + 1
 *     minor = 0
 *     micro = 0
 *
 * When binary compatibility is preserved:
 *
 *   When API is added:
 *
 *     minor = minor + 1
 *     micro = 0
 *
 *   When API is the same:
 *
 *     micro = micro + 1
 */
#define CIM_MAJOR_VERSION  1
#define CIM_MINOR_VERSION  0
#define CIM_MICRO_VERSION  0

enum _CimEventType : int32_t {
  CIM_EVENT_KEY_PRESS   = 0, /* This means that the key has been pressed. */
  CIM_EVENT_KEY_RELEASE = 1  /* This means that the key has been released. */
};
typedef enum _CimEventType CimEventType;

typedef struct _CimEvent CimEvent;
struct _CimEvent {
  CimEventType type;
  uint32_t     state;
  uint32_t     keyval;
  uint32_t     keycode;
};

enum _CimTextAttrType : int {
  CIM_TEXT_ATTR_UNDERLINE,
  CIM_TEXT_ATTR_HIGHLIGHT
};
typedef enum _CimTextAttrType CimTextAttrType;

typedef struct _CimTextAttr CimTextAttr;
struct _CimTextAttr {
  CimTextAttrType type;
  int pos;     /* starting position to apply attribute in characters */
  int n_chars; /* number of characters to apply attribute to */
};

typedef struct _CimPreedit CimPreedit;
struct _CimPreedit {
  char* text;
  CimTextAttr* attrs;
  int attrs_len;
  int cursor_pos;
};

typedef struct _CimSurround CimSurround;
struct _CimSurround {
  char* text;
  int   len;
  int   cursor_pos; /* cursor position in characters */
  int   anchor_pos; /* anchor position in characters */
};

typedef struct _CimRect CimRect;
struct _CimRect {
  int x;
  int y;
  int width;
  int height;
};

enum _CimItemType : int {
  CIM_ITEM_STRING,
  CIM_ITEM_N_TYPES
};
typedef enum _CimItemType CimItemType;

typedef struct _CimItem CimItem;
struct _CimItem {
  CimItemType type;
  void* data;
};

typedef struct _CimCandidate CimCandidate;
struct _CimCandidate {
  /* Page index means the current page. Page index starts at 0. */
  int       page_index;
  int       n_pages;
  CimItem** table;
  int       n_rows;
  int       n_cols;
  /*
   * If the aux_text variable is not NULL, the candidate window should show a
   * auxiliary text area and set the auxiliary text.
   */
  char* aux_text;
  int   aux_cursor_pos;
};

typedef struct _CimSelection CimSelection;
struct _CimSelection {
  int start_row;
  int start_col;
  int end_row;
  int end_col;
};

#if (defined(__GNUC__) && (GCC_VERSION < 13)) || \
    (defined(__clang__) && (__clang_major__ < 16))
enum _CimCbType {
  CIM_CB_PREEDIT_START,
  CIM_CB_PREEDIT_END,
  CIM_CB_PREEDIT_CHANGED,
  CIM_CB_COMMIT,
  CIM_CB_GET_SURROUND,
  CIM_CB_DELETE_SURROUND,
  CIM_CB_CANDIDATE_SHOW,
  CIM_CB_CANDIDATE_HIDE,
  CIM_CB_CANDIDATE_CHANGED,
  CIM_CB_CANDIDATE_SELECTED,
  CIM_CB_N_TYPES
};
typedef int CimCbType;
#else
enum _CimCbType : int {
  CIM_CB_PREEDIT_START,
  CIM_CB_PREEDIT_END,
  CIM_CB_PREEDIT_CHANGED,
  CIM_CB_COMMIT,
  CIM_CB_GET_SURROUND,
  CIM_CB_DELETE_SURROUND,
  CIM_CB_CANDIDATE_SHOW,
  CIM_CB_CANDIDATE_HIDE,
  CIM_CB_CANDIDATE_CHANGED,
  CIM_CB_CANDIDATE_SELECTED,
  CIM_CB_N_TYPES
};
typedef enum _CimCbType CimCbType;
#endif

enum _CimCbMask {
  CIM_CB_PREEDIT_START_MASK      = 1 << CIM_CB_PREEDIT_START,
  CIM_CB_PREEDIT_END_MASK        = 1 << CIM_CB_PREEDIT_END,
  CIM_CB_PREEDIT_CHANGED_MASK    = 1 << CIM_CB_PREEDIT_CHANGED,
  CIM_CB_COMMIT_MASK             = 1 << CIM_CB_COMMIT,
  CIM_CB_GET_SURROUND_MASK       = 1 << CIM_CB_GET_SURROUND,
  CIM_CB_DELETE_SURROUND_MASK    = 1 << CIM_CB_DELETE_SURROUND,
  CIM_CB_CANDIDATE_SHOW_MASK     = 1 << CIM_CB_CANDIDATE_SHOW,
  CIM_CB_CANDIDATE_HIDE_MASK     = 1 << CIM_CB_CANDIDATE_HIDE,
  CIM_CB_CANDIDATE_CHANGED_MASK  = 1 << CIM_CB_CANDIDATE_CHANGED,
  CIM_CB_CANDIDATE_SELECTED_MASK = 1 << CIM_CB_CANDIDATE_SELECTED,
  CIM_CB_PREEDIT_MASK = CIM_CB_PREEDIT_START_MASK |
                        CIM_CB_PREEDIT_END_MASK   |
                        CIM_CB_PREEDIT_CHANGED_MASK
};
typedef enum _CimCbMask CimCbMask;

typedef struct _CimIc CimIc;
typedef struct _CimCallbacks CimCallbacks;
struct _CimCallbacks {
  void (*preedit_start)     (CimIc* ic, void* user_data);
  void (*preedit_end)       (CimIc* ic, void* user_data);
  void (*preedit_changed)   (CimIc* ic,
                             const CimPreedit* preedit,
                             void* user_data);
  void (*commit)            (CimIc* ic,
                             const char* text,
                             void* user_data);
  /* Do not free CimSurround and its text */
  const CimSurround* (*get_surround) (CimIc* ic, void* user_data);
  bool (*delete_surround)   (CimIc* ic,
                             int    offset,
                             int    n_chars,
                             void*  user_data);
  /* candidate */
  void (*candidate_show)     (CimIc* ic, int n_rows, int n_cols, bool show_aux,
                              void* user_data);
  void (*candidate_hide)     (CimIc* ic, void* user_data);
  void (*candidate_changed)  (CimIc* ic,
                              const CimCandidate* candidate,
                              void* user_data);
  void (*candidate_selected) (CimIc* ic,
                              const CimSelection* selection,
                              void* user_data);
  /* reserved */
  void (*reserved_1) ();
  void (*reserved_2) ();
  void (*reserved_3) ();
  void (*reserved_4) ();
  void (*reserved_5) ();
  void (*reserved_6) ();
};

struct _CimIc {
  void (*focus_in)       (CimIc* ic);
  void (*focus_out)      (CimIc* ic);
  void (*reset)          (CimIc* ic);
  bool (*filter_event)   (CimIc* ic, const CimEvent* event);
  void (*set_cursor_pos) (CimIc* ic, const CimRect*  area);
  const CimPreedit*   (*get_preedit)   (CimIc* ic);
  const CimCandidate* (*get_candidate) (CimIc* ic);
  void (*set_vcallbacks)          (CimIc* ic, va_list ap);
  void (*activate_candidate_item) (CimIc* ic, int row, int col);
  void (*change_candidate_page)   (CimIc* ic, int page_index);
  /* reserved */
  void (*reserved_1) ();
  void (*reserved_2) ();
  void (*reserved_3) ();
  void (*reserved_4) ();
  void (*reserved_5) ();
  void (*reserved_6) ();
};

/* Returns a newly allocated CimIc. Free it with cim_ic_free */
CimIc* cim_ic_new (void);
void   cim_ic_free           (CimIc* ic);
void   cim_ic_focus_in       (CimIc* ic);
void   cim_ic_focus_out      (CimIc* ic);
void   cim_ic_reset          (CimIc* ic);
bool   cim_ic_filter_event   (CimIc* ic, const CimEvent* event);
void   cim_ic_set_cursor_pos (CimIc* ic, const CimRect*  area);
/*
 * Variadic argument list must end with -1.
   cim_ic_set_callbacks (ic,
     cb_type1, cb1, user_data1,
     cb_type2, cb2, user_data2, -1);
 */
void   cim_ic_set_callbacks  (CimIc* ic, ...);
const CimPreedit*   cim_ic_get_preedit   (CimIc* ic);
const CimCandidate* cim_ic_get_candidate (CimIc* ic);
void  cim_ic_activate_candidate_item (CimIc* ic, int row, int col);
void  cim_ic_change_candidate_page   (CimIc* ic, int page_index);
/* utility functions */
char* cim_get_cim_so_path (void);

/*
 * The cim plugin must implement the following functions.
 *
 * CimIc* cim_plugin_new_ic ();
 * void   cim_plugin_free_ic (CimIc* ic);
 * void   cim_plugin_get_version (int* major, int* minor, int* micro);
 */

#ifdef __cplusplus
}
#endif

#endif /* __CIM_H__ */
