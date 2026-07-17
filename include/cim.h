/******************************************************************************
 * cim.h
 * Copyright (C) 2023-2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 ******************************************************************************/
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
#define CIM_MINOR_VERSION  (uint32_t) 1
#define CIM_MICRO_VERSION  (uint32_t) 0

/**
 * @brief Cim text encoding model.
 *
 * All text exposed by the Cim public API is encoded in UTF-8.
 */

/**
 * @brief Threading model.
 *
 * Cim APIs and callbacks may be called from arbitrary threads.
 */

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

/**
 * @brief Preedit text owned by the input method.
 *
 * CimPreedit must always contain valid values because Cim does not know when
 * cim_ic_get_preedit() will be called.
 *
 * The @a text field is always UTF-8 encoded.
 */
typedef struct _CimPreedit CimPreedit;
struct _CimPreedit {
  char* text;
  CimTextAttr* attrs;
  uint32_t attrs_len;
  uint32_t cursor_pos;
};

/**
 * @brief Surrounding text owned by the application.
 *
 * The @a text field is always UTF-8 encoded.
 *
 * The text buffer is borrowed from the application and must not be
 * modified or freed by Cim or plugins.
 */
typedef struct _CimSurround CimSurround;
struct _CimSurround {
  const char* text;
  uint32_t    len;
  uint32_t    cursor_pos;
  uint32_t    anchor_pos;
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

/**
 * @brief Candidate table owned by the input method.
 *
 * CimCandidate must always contain valid values because Cim does not know when
 * cim_ic_get_candidate() will be called.
 *
 * String payloads stored in candidate items are UTF-8 encoded.
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
/**
 * @brief Callback table provided by the Cim client.
 *
 * All text exchanged through these callbacks is UTF-8 encoded.
 */
struct _CimCallbacks {
  void (*preedit_start)     (CimIcHandle ic, void* user_data);
  void (*preedit_end)       (CimIcHandle ic, void* user_data);
  /**
   * @brief Notify that preedit text changed.
   *
   * The @a preedit object is owned by the IME.
   * The IM client must not free @a preedit, @a preedit->text, or
   * @a preedit->attrs.
   */
  void (*preedit_changed)   (CimIcHandle ic,
                             const CimPreedit* preedit,
                             void* user_data);
  /**
   * @brief Deliver committed text.
   *
   * The committed @a text is UTF-8 encoded and is owned by the IME.
   * The IM client must not free the @a text pointer.
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
  /**
   * @brief Filter an input event.
   *
   * This function determines whether the input method consumes the event.
   *
   * @param ic Input context.
   * @param event Event to be filtered.
   *
   * @retval true  The event was consumed by Cim or the active plugin.
   *               The application should not process the event further.
   * @retval false The event was not consumed by Cim or the active plugin.
   *               The application should continue processing the event.
   *
   * This return value does not indicate success or failure. It only indicates
   * whether the event was consumed.
   */
  bool (*filter_event)   (CimIcHandle ic, const CimEvent* event);
  void (*set_cursor_pos) (CimIcHandle ic, const CimRect*  area);
  /**
   * @brief Get current preedit text.
   *
   * The returned CimPreedit object and its fields are owned by the IME.
   * The IM client must not free them.
   */
  const CimPreedit*   (*get_preedit)   (CimIcHandle ic);
  /**
   * @brief Get current candidate data.
   *
   * The returned CimCandidate object and its fields are owned by the IME.
   * The IM client must not free them.
   */
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
/**
 * @brief Plugin metadata.
 *
 * This structure provides basic information describing an input method
 * plugin.
 *
 * All fields are owned by the plugin implementation. Implementations are
 * expected to provide static storage for the CimInfo object itself as well
 * as for any referenced strings.
 *
 * The caller must not modify or free the structure or any of the fields
 * referenced by it.
 *
 * The structure and all referenced fields are valid only while the plugin
 * remains loaded.
 */
struct _CimInfo {
  const char* name;
  const char* desc;
  void* reserved[6];
};

typedef struct _CimPlugin  CimPlugin;

/**
 * @brief Root object of the Cim plugin ABI.
 *
 * Every plugin must export an externally visible global #CimPlugin object
 * named `cim_plugin`. The object and its vtable must remain valid while the
 * plugin is loaded.
 *
 * The vtable, its create callback, and its destroy callback must not be NULL.
 * Set padding to zero and all reserved entries to NULL.
 */
struct _CimPlugin {
  uint32_t cim_api_major;
  uint32_t cim_api_minor;
  uint32_t cim_api_micro;
  uint32_t padding;
  /**
   * @brief Retrieve plugin metadata.
   *
   * Returns a pointer to a plugin-owned #CimInfo structure describing the
   * plugin.
   *
   * The returned pointer is borrowed and must not be modified or freed.
   * The returned structure remains valid only while the plugin remains
   * loaded.
   *
   * @return Pointer to a constant #CimInfo structure, or NULL if the plugin
   *         does not provide metadata.
   *
   * Example implementation:
   *
   * @code
   * static const CimInfo example_info = {
   *   .name = "example",
   *   .desc = "Example input method plugin",
   *   .reserved = { NULL, NULL, NULL, NULL, NULL, NULL }
   * };
   *
   * static const CimInfo *
   * example_get_info (void)
   * {
   *   return &example_info;
   * }
   * @endcode
   */
  const CimInfo* (*get_info) (void);
/**
 * @brief Initialize plugin-global state.
 *
 * This function is called once when the plugin is loaded by libcim,
 * before any input context is created.
 *
 * @retval 0 Initialization succeeded.
 * @retval nonzero Initialization failed.
 *
 * If this function returns a nonzero value, the plugin is not loaded
 * and #fini is not called.
 *
 * This callback is optional. If NULL, initialization is treated as successful.
 */
  int  (*init) (void);
  void (*fini) (void);
  CimIcVTable* vtable;
  void* reserved[2];
};

#ifdef USE_C23_ENUM
enum _CimError : uint32_t {
#else
enum _CimError {
#endif
  CIM_ERROR_NONE = 0,
  CIM_ERROR_INVALID_ARGUMENT,
  CIM_ERROR_HOME_NOT_SET,
  CIM_ERROR_PLUGIN_PATH_TOO_LONG,
  CIM_ERROR_MUTEX_FAILED,
  CIM_ERROR_DLOPEN_FAILED,
  CIM_ERROR_DLSYM_FAILED,
  CIM_ERROR_NULL_PLUGIN,
  CIM_ERROR_BAD_ABI,
  CIM_ERROR_INVALID_PLUGIN,
  CIM_ERROR_INIT_FAILED,
  CIM_ERROR_CREATE_FAILED,
  CIM_ERROR_ALLOCATION_FAILED,
  CIM_ERROR_DLCLOSE_FAILED
};
#ifdef USE_C23_ENUM
typedef enum _CimError CimError;
#else
typedef uint32_t CimError;
#endif

CimIcHandle cim_ic_create (void);

/**
 * @brief Destroy an input context.
 *
 * @param ic A valid live input context. This value must not be NULL.
 *
 * Passing NULL is a contract violation and terminates the process.
 */
void cim_ic_destroy (CimIcHandle ic);

/**
 * @brief Notify an input context that it gained focus.
 *
 * @param ic A valid live input context. This value must not be NULL.
 *
 * Passing NULL is a contract violation and terminates the process.
 */
void cim_ic_focus_in (CimIcHandle ic);

/**
 * @brief Notify an input context that it lost focus.
 *
 * @param ic A valid live input context. This value must not be NULL.
 *
 * Passing NULL is a contract violation and terminates the process.
 */
void cim_ic_focus_out (CimIcHandle ic);

/**
 * @brief Reset an input context.
 *
 * @param ic A valid live input context. This value must not be NULL.
 *
 * Passing NULL is a contract violation and terminates the process.
 */
void cim_ic_reset (CimIcHandle ic);

/**
 * @brief Filter an input event.
 *
 * This function asks Cim to handle the given event.
 *
 * @param ic A valid live input context. This value must not be NULL.
 * @param event A valid event to be filtered. This value must not be NULL.
 *
 * Passing NULL for either parameter is a contract violation and terminates
 * the process.
 *
 * @retval true  The event was consumed by Cim or the active plugin.
 *               The caller should not process the event further.
 * @retval false The event was not consumed by Cim or the active plugin.
 *               The caller should continue normal event processing.
 *
 * This return value does not indicate success or failure. It only indicates
 * whether the event was consumed.
 */
bool  cim_ic_filter_event   (CimIcHandle ic, const CimEvent* event);

/**
 * @brief Update the cursor rectangle for an input context.
 *
 * @param ic A valid live input context. This value must not be NULL.
 * @param area A valid cursor rectangle. This value must not be NULL.
 *
 * Passing NULL for either parameter is a contract violation and terminates
 * the process.
 */
void cim_ic_set_cursor_pos (CimIcHandle ic, const CimRect* area);

/**
 * @brief Set callbacks for an input context.
 *
 * @param ic A valid live input context. This value must not be NULL.
 * @param callbacks A valid callback table. This value must not be NULL.
 * @param user_data Optional opaque data passed to callbacks. This value may
 *                  be NULL.
 *
 * Passing NULL for either @p ic or @p callbacks is a contract violation and
 * terminates the process.
 */
void cim_ic_set_callbacks (CimIcHandle ic,
                           const CimCallbacks* callbacks,
                           void* user_data);

/**
 * @brief Get the current preedit state.
 *
 * @param ic A valid live input context. This value must not be NULL.
 *
 * @return The current preedit state, or NULL when no preedit state is
 *         available.
 *
 * Passing NULL for @p ic is a contract violation and terminates the process.
 */
const CimPreedit* cim_ic_get_preedit (CimIcHandle ic);

/**
 * @brief Get the current candidate state.
 *
 * @param ic A valid live input context. This value must not be NULL.
 *
 * @return The current candidate state, or NULL when no candidate state is
 *         available.
 *
 * Passing NULL for @p ic is a contract violation and terminates the process.
 */
const CimCandidate* cim_ic_get_candidate (CimIcHandle ic);

/**
 * @brief Activate a candidate item.
 *
 * @param ic A valid live input context. This value must not be NULL.
 * @param row Candidate row index.
 * @param col Candidate column index.
 *
 * Passing NULL for @p ic is a contract violation and terminates the process.
 */
void cim_ic_activate_candidate_item (CimIcHandle ic,
                                     uint32_t row,
                                     uint32_t col);

/**
 * @brief Change the current candidate page.
 *
 * @param ic A valid live input context. This value must not be NULL.
 * @param page_index Candidate page index.
 *
 * Passing NULL for @p ic is a contract violation and terminates the process.
 */
void cim_ic_change_candidate_page (CimIcHandle ic,
                                   uint32_t page_index);

/* utility functions */
char *cim_dup_plugin_path (void);

/**
 * @brief Return the latest Cim error for the current thread.
 *
 * @return The thread-local error code.
 *
 * Each non-diagnostic Cim API call resets this value to #CIM_ERROR_NONE
 * before it runs. A failure may replace it with a more specific error.
 * Calling cim_get_last_error() or cim_strerror() does not change it.
 */
CimError cim_get_last_error (void);

/**
 * @brief Return a static message for a Cim error code.
 *
 * @param error A valid #CimError enumerator.
 *
 * @return Pointer to a static null-terminated string owned by Cim. The caller
 *         must not modify or free the returned string.
 *
 * Passing a value outside the #CimError enumeration is a contract violation
 * and terminates the process.
 */
const char *cim_strerror (CimError error);

#ifdef __cplusplus
}
#endif

#endif /* __CIM_H__ */
