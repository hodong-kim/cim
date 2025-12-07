// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * cim.h
 * This file is part of Cim.
 *
 * Copyright (C) 2023-2025 Hodong Kim <hodong@nimfsoft.art>
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

#if (defined(__GNUC__) && (__GNUC__ >= 13)) || \
    (defined(__clang__) && (__clang_major__ >= 16))
#define USE_C23_ENUM
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
#define CIM_MAJOR_VERSION  (uint32_t) 2
#define CIM_MINOR_VERSION  (uint32_t) 0
#define CIM_MICRO_VERSION  (uint32_t) 0

#ifdef USE_C23_ENUM
enum _CimEventType : uint32_t {
#else
enum _CimEventType {
#endif
  CIM_EVENT_KEY_PRESS   = 0, /* This means that the key has been pressed. */
  CIM_EVENT_KEY_RELEASE = 1  /* This means that the key has been released. */
};
#ifdef USE_C23_ENUM
typedef enum _CimEventType CimEventType;
#else
typedef uint32_t CimEventType;
#endif

typedef struct _CimEvent CimEvent;
struct _CimEvent {
  CimEventType type;
  uint32_t     state;
  uint32_t     keyval;
  uint32_t     keycode;
};

#ifdef USE_C23_ENUM
enum _CimTextAttrType : uint32_t {
#else
enum _CimTextAttrType {
#endif
  CIM_TEXT_ATTR_UNDERLINE,
  CIM_TEXT_ATTR_HIGHLIGHT
};
#ifdef USE_C23_ENUM
typedef enum _CimTextAttrType CimTextAttrType;
#else
typedef uint32_t CimTextAttrType;
#endif

typedef struct _CimTextAttr CimTextAttr;
struct _CimTextAttr {
  CimTextAttrType type;
  uint32_t pos;     /* starting position to apply attribute in characters */
  uint32_t n_chars; /* number of characters to apply attribute to */
};

/*
  CimPreedit must always have valid values
  because we don't know when cim_ic_get_preedit will be called.
*/
typedef struct _CimPreedit CimPreedit;
struct _CimPreedit {
  /* text is always UTF-8 encoded. */
  char* text;
  CimTextAttr* attrs;
  uint32_t attrs_len;
  uint32_t cursor_pos;
};

typedef struct _CimSurround CimSurround;
struct _CimSurround {
  /* text is always UTF-8 encoded. */
  char* text;
  uint32_t len;
  uint32_t cursor_pos; /* cursor position in characters */
  uint32_t anchor_pos; /* anchor position in characters */
};

typedef struct _CimRect CimRect;
struct _CimRect {
  int32_t  x;
  int32_t  y;
  uint32_t width;
  uint32_t height;
};

#ifdef USE_C23_ENUM
enum _CimItemType : uint32_t {
#else
enum _CimItemType  {
#endif
  CIM_ITEM_STRING,
  CIM_ITEM_N_TYPES
};
#ifdef USE_C23_ENUM
typedef enum _CimItemType CimItemType;
#else
typedef uint32_t CimItemType;
#endif

typedef struct _CimItem  CimItem;
struct _CimItem {
  void*       data;
  CimItemType type;
  uint32_t    padding;
};

/*
  CimCandidate must always have valid values
  because we don't know when cim_ic_get_candidate will be called.
*/
typedef struct _CimCandidate CimCandidate;
struct _CimCandidate {
  /* Page index means the current page. Page index starts at 0. */
  uint32_t  page_index;
  // total number of pages
  uint32_t  n_pages;
  /*
   * A table consists of n rows and n columns stored in a flat array.
   * Access element at (row, col) as:
   *   table[row * n_cols + col]
   * The index range of rows is from 0 to n_rows - 1, and
   * the index range of columns is from 0 to n_cols - 1.
   */
  CimItem* table;
  uint32_t n_rows;
  uint32_t n_cols;
  /*
   * If the aux_text variable is not NULL, the candidate window should show a
   * auxiliary text area and set the auxiliary text.
   */
  char*    aux_text;
  uint32_t aux_cursor_pos;
  uint32_t padding;
};

typedef struct _CimSelection CimSelection;
struct _CimSelection {
  uint32_t start_row;
  uint32_t start_col;
  uint32_t end_row;
  uint32_t end_col;
};

struct CimIcImpl;
typedef struct CimIcImpl* CimIcHandle;
typedef struct _CimIcVTable CimIcVTable;

#ifdef USE_C23_ENUM
enum _CimNotificationType : uint32_t {
#else
enum _CimNotificationType  {
#endif
  /* User Input Feedback */
  CIM_NOTIFICATION_COMPOSE_CANCELLED,
/* examples:
  CIM_NOTIFICATION_INVALID_KEY_PRESS,
  CIM_NOTIFICATION_SELECTION_WRAPPED,
  // State and Mode Changes
  CIM_NOTIFICATION_INPUT_MODE_CHANGED,
  CIM_NOTIFICATION_HANJA_MODE_ACTIVATED,
  CIM_NOTIFICATION_FULL_WIDTH_MODE_CHANGED,
  CIM_NOTIFICATION_COMPOSITION_CLEARED_AUTOMATICALLY,
  // Engine and System Events
  CIM_NOTIFICATION_CONFIG_RELOADED,
  CIM_NOTIFICATION_DICTIONARY_UPDATED,
  // Error and Warning Conditions
  CIM_NOTIFICATION_DICTIONARY_LOOKUP_FAILED,
  CIM_NOTIFICATION_MAX_PREEDIT_LENGTH_REACHED
*/
};
#ifdef USE_C23_ENUM
typedef enum _CimNotificationType  CimNotificationType;
#else
typedef uint32_t CimNotificationType;
#endif

typedef struct _CimCallbacks CimCallbacks;
struct _CimCallbacks {
  void (*preedit_start)     (CimIcHandle ic, void* user_data);
  void (*preedit_end)       (CimIcHandle ic, void* user_data);
/* * The 'preedit' data is owned by the IME.
   * The 'preedit->text' is always UTF-8 encoded.
   * The IM client must NOT free 'preedit', 'preedit->text', or 'preedit->attrs'.
   */
  void (*preedit_changed)   (CimIcHandle ic,
                             const CimPreedit* preedit,
                             void* user_data);
/* * The committed 'text' is always UTF-8 encoded and is owned by the IME.
   * The IM client must NOT free the text pointer.
   */
  void (*commit)            (CimIcHandle ic,
                             const char* text,
                             void* user_data);
  /* Do not free CimSurround and its text. The text is always UTF-8 encoded. */
  const CimSurround* (*get_surround) (CimIcHandle ic, void* user_data);
  bool (*delete_surround)   (CimIcHandle ic,
                             int32_t  offset,
                             uint32_t n_chars,
                             void*  user_data);
  /* candidate */
  void (*candidate_show)     (CimIcHandle ic,
                              uint32_t n_rows,
                              uint32_t n_cols,
                              bool show_aux,
                              void* user_data);
  void (*candidate_hide)     (CimIcHandle ic, void* user_data);
  void (*candidate_changed)  (CimIcHandle ic,
                              const CimCandidate* candidate,
                              void* user_data);
  void (*candidate_selected) (CimIcHandle ic,
                              const CimSelection* selection,
                              void* user_data);
  void (*notify) (CimIcHandle ic,
                  CimNotificationType type,
                  void* user_data);
  /* reserved */
  void* reserved[5];
};

struct _CimIcVTable {
  CimIcHandle (*create)  (void);
  void (*destroy)        (CimIcHandle ic);
  void (*focus_in)       (CimIcHandle ic);
  void (*focus_out)      (CimIcHandle ic);
  void (*reset)          (CimIcHandle ic);
  bool (*filter_event)   (CimIcHandle ic, const CimEvent* event);
  void (*set_cursor_pos) (CimIcHandle ic, const CimRect*  area);
/* * The returned CimPreedit and its fields are owned by the IME.
   * The IM client must NOT free them. */
  const CimPreedit*   (*get_preedit)   (CimIcHandle ic);
/* * The returned CimCandidate and its fields are owned by the IME.
   * The IM client must NOT free them. */
  const CimCandidate* (*get_candidate) (CimIcHandle ic);
  void (*set_callbacks)  (CimIcHandle ic,
                          const CimCallbacks* callbacks,
                          void* user_data);
  void (*activate_candidate_item) (CimIcHandle ic, uint32_t row, uint32_t col);
  void (*change_candidate_page)   (CimIcHandle ic, uint32_t page_index);
  /* reserved */
  void* reserved[4];
};

typedef struct _CimInfo  CimInfo;
struct _CimInfo {
  const char* name;
  const char* desc;
  void* reserved[6];
};

typedef struct _CimPlugin  CimPlugin;
struct _CimPlugin {
  uint32_t cim_api_major;
  uint32_t cim_api_minor;
  uint32_t cim_api_micro;
  uint32_t padding;
  const CimInfo* (*get_info) (void);
  int  (*init) (void);
  void (*fini) (void);
  CimIcVTable* vtable;
  void* reserved[2];
};

CimIcHandle cim_ic_create (void);
void  cim_ic_destroy        (CimIcHandle ic);
void  cim_ic_focus_in       (CimIcHandle ic);
void  cim_ic_focus_out      (CimIcHandle ic);
void  cim_ic_reset          (CimIcHandle ic);
bool  cim_ic_filter_event   (CimIcHandle ic, const CimEvent* event);
void  cim_ic_set_cursor_pos (CimIcHandle ic, const CimRect*  area);
void  cim_ic_set_callbacks  (CimIcHandle ic,
                             const CimCallbacks* callbacks,
                             void* user_data);
const CimPreedit*   cim_ic_get_preedit   (CimIcHandle ic);
const CimCandidate* cim_ic_get_candidate (CimIcHandle ic);
void  cim_ic_activate_candidate_item (CimIcHandle ic,
                                      uint32_t row,
                                      uint32_t col);
void  cim_ic_change_candidate_page   (CimIcHandle ic, uint32_t page_index);

/* utility functions */
char* cim_get_cim_so_path (void);

#ifdef __cplusplus
}
#endif

#endif /* __CIM_H__ */
