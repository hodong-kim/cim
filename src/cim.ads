-- ============================================================================
-- cim.ads
-- Copyright (c) 2023-2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Interfaces.C;
with Interfaces.C.Strings;
with System;

package Cim is

  ---------------------------------------------------------------------------
  -- Core Types & Constants
  ---------------------------------------------------------------------------

  subtype CimIcHandle is System.Address;

  NULL_IC : constant CimIcHandle := System.NULL_ADDRESS;

  CIM_MAJOR_VERSION : constant Interfaces.C.unsigned := 2;
  CIM_MINOR_VERSION : constant Interfaces.C.unsigned := 1;
  CIM_MICRO_VERSION : constant Interfaces.C.unsigned := 0;

  ---------------------------------------------------------------------------
  -- Enums
  ---------------------------------------------------------------------------

  type CimEventType is
    (CIM_EVENT_KEY_PRESS,
     CIM_EVENT_KEY_RELEASE)
  with convention => c;

  for CimEventType use
    (CIM_EVENT_KEY_PRESS   => 0,
     CIM_EVENT_KEY_RELEASE => 1);

  type CimTextAttrType is
    (CIM_TEXT_ATTR_UNDERLINE,
     CIM_TEXT_ATTR_HIGHLIGHT)
  with convention => c;

  for CimTextAttrType use
    (CIM_TEXT_ATTR_UNDERLINE => 0,
     CIM_TEXT_ATTR_HIGHLIGHT => 1);

  type CimItemType is
    (CIM_ITEM_STRING,
     CIM_ITEM_N_TYPES)
  with convention => c;

  for CimItemType use
    (CIM_ITEM_STRING  => 0,
     CIM_ITEM_N_TYPES => 1);

  type CimNotificationType is
    (CIM_NOTIFICATION_COMPOSE_CANCELLED)
  with convention => c;

  for CimNotificationType use
    (CIM_NOTIFICATION_COMPOSE_CANCELLED => 0);

  type Error is
    (CIM_ERROR_NONE,
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
     CIM_ERROR_DLCLOSE_FAILED)
  with convention => c;

  for Error use
    (CIM_ERROR_NONE                 => 0,
     CIM_ERROR_INVALID_ARGUMENT     => 1,
     CIM_ERROR_HOME_NOT_SET         => 2,
     CIM_ERROR_PLUGIN_PATH_TOO_LONG => 3,
     CIM_ERROR_MUTEX_FAILED         => 4,
     CIM_ERROR_DLOPEN_FAILED        => 5,
     CIM_ERROR_DLSYM_FAILED         => 6,
     CIM_ERROR_NULL_PLUGIN          => 7,
     CIM_ERROR_BAD_ABI              => 8,
     CIM_ERROR_INVALID_PLUGIN       => 9,
     CIM_ERROR_INIT_FAILED          => 10,
     CIM_ERROR_CREATE_FAILED        => 11,
     CIM_ERROR_ALLOCATION_FAILED    => 12,
     CIM_ERROR_DLCLOSE_FAILED       => 13);

  ---------------------------------------------------------------------------
  -- Structs & Named Access Types
  ---------------------------------------------------------------------------

  type CimEvent is record
    event_type : CimEventType;
    state      : Interfaces.C.unsigned;
    keyval     : Interfaces.C.unsigned;
    keycode    : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimEvent_Access is access constant CimEvent
  with convention => c;

  type CimTextAttr is record
    attr_type : CimTextAttrType;
    pos       : Interfaces.C.unsigned;
    n_chars   : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimTextAttr_Access is access all CimTextAttr
  with convention => c;

  type CimPreedit is record
    text       : Interfaces.C.Strings.chars_ptr;
    attrs      : CimTextAttr_Access;
    attrs_len  : Interfaces.C.unsigned;
    cursor_pos : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimPreedit_Access is access constant CimPreedit
  with convention => c;

  type CimSurround is record
    text       : Interfaces.C.Strings.chars_ptr;
    len        : Interfaces.C.unsigned;
    cursor_pos : Interfaces.C.unsigned;
    anchor_pos : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimSurround_Access is access constant CimSurround
  with convention => c;

  type CimRect is record
    x      : Interfaces.C.int;
    y      : Interfaces.C.int;
    width  : Interfaces.C.unsigned;
    height : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimRect_Access is access constant CimRect
  with convention => c;

  type CimItem is record
    data      : System.Address;
    item_type : CimItemType;
    padding   : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimItem_Access is access all CimItem
  with convention => c;

  type CimCandidate is record
    page_index     : Interfaces.C.unsigned;
    n_pages        : Interfaces.C.unsigned;
    table          : CimItem_Access;
    n_rows         : Interfaces.C.unsigned;
    n_cols         : Interfaces.C.unsigned;
    aux_text       : Interfaces.C.Strings.chars_ptr;
    aux_cursor_pos : Interfaces.C.unsigned;
    padding        : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimCandidate_Access is access constant CimCandidate
  with convention => c;

  type CimSelection is record
    start_row : Interfaces.C.unsigned;
    start_col : Interfaces.C.unsigned;
    end_row   : Interfaces.C.unsigned;
    end_col   : Interfaces.C.unsigned;
  end record
  with convention => c;

  type CimSelection_Access is access constant CimSelection
  with convention => c;

  ---------------------------------------------------------------------------
  -- Callbacks & VTable
  ---------------------------------------------------------------------------

  type Reserved_Array_5 is array (0 .. 4) of System.Address
  with convention => c;

  type Reserved_Array_4 is array (0 .. 3) of System.Address
  with convention => c;

  type Reserved_Array_6 is array (0 .. 5) of System.Address
  with convention => c;

  type Reserved_Array_2 is array (0 .. 1) of System.Address
  with convention => c;

  type CimCallbacks is record
    preedit_start : access procedure
      (ic        : CimIcHandle;
       user_data : System.Address)
    with convention => c;

    preedit_end : access procedure
      (ic        : CimIcHandle;
       user_data : System.Address)
    with convention => c;

    preedit_changed : access procedure
      (ic        : CimIcHandle;
       preedit   : CimPreedit_Access;
       user_data : System.Address)
    with convention => c;

    commit : access procedure
      (ic        : CimIcHandle;
       text      : Interfaces.C.Strings.chars_ptr;
       user_data : System.Address)
    with convention => c;

    get_surround : access function
      (ic        : CimIcHandle;
       user_data : System.Address)
    return CimSurround_Access
    with convention => c;

    delete_surround : access function
      (ic        : CimIcHandle;
       offset    : Interfaces.C.int;
       n_chars   : Interfaces.C.unsigned;
       user_data : System.Address)
    return Interfaces.C.C_bool
    with convention => c;

    candidate_show : access procedure
      (ic        : CimIcHandle;
       n_rows    : Interfaces.C.unsigned;
       n_cols    : Interfaces.C.unsigned;
       show_aux  : Interfaces.C.C_bool;
       user_data : System.Address)
    with convention => c;

    candidate_hide : access procedure
      (ic        : CimIcHandle;
       user_data : System.Address)
    with convention => c;

    candidate_changed : access procedure
      (ic        : CimIcHandle;
       candidate : CimCandidate_Access;
       user_data : System.Address)
    with convention => c;

    candidate_selected : access procedure
      (ic        : CimIcHandle;
       selection : CimSelection_Access;
       user_data : System.Address)
    with convention => c;

    notify : access procedure
      (ic         : CimIcHandle;
       notif_type : CimNotificationType;
       user_data  : System.Address)
    with convention => c;

    reserved : Reserved_Array_5;
  end record
  with convention => c;

  type CimCallbacks_Access is access constant CimCallbacks
  with convention => c;

  type CimIcVTable is record
    create : access function
    return CimIcHandle
    with convention => c;

    destroy : access procedure
      (ic : CimIcHandle)
    with convention => c;

    focus_in : access procedure
      (ic : CimIcHandle)
    with convention => c;

    focus_out : access procedure
      (ic : CimIcHandle)
    with convention => c;

    reset : access procedure
      (ic : CimIcHandle)
    with convention => c;

    filter_event : access function
      (ic    : CimIcHandle;
       event : CimEvent_Access)
    return Interfaces.C.C_bool
    with convention => c;

    set_cursor_pos : access procedure
      (ic   : CimIcHandle;
       area : CimRect_Access)
    with convention => c;

    get_preedit : access function
      (ic : CimIcHandle)
    return CimPreedit_Access
    with convention => c;

    get_candidate : access function
      (ic : CimIcHandle)
    return CimCandidate_Access
    with convention => c;

    set_callbacks : access procedure
      (ic        : CimIcHandle;
       callbacks : CimCallbacks_Access;
       user_data : System.Address)
    with convention => c;

    activate_candidate_item : access procedure
      (ic  : CimIcHandle;
       row : Interfaces.C.unsigned;
       col : Interfaces.C.unsigned)
    with convention => c;

    change_candidate_page : access procedure
      (ic         : CimIcHandle;
       page_index : Interfaces.C.unsigned)
    with convention => c;

    reserved : Reserved_Array_4;
  end record
  with convention => c;

  type CimInfo is record
    name     : Interfaces.C.Strings.chars_ptr;
    desc     : Interfaces.C.Strings.chars_ptr;
    reserved : Reserved_Array_6;
  end record
  with convention => c;

  type CimPlugin is record
    cim_api_major : Interfaces.C.unsigned;
    cim_api_minor : Interfaces.C.unsigned;
    cim_api_micro : Interfaces.C.unsigned;
    padding       : Interfaces.C.unsigned;

    get_info : access function
    return access constant CimInfo
    with convention => c;

    init : access function
    return Interfaces.C.int
    with convention => c;

    fini : access procedure
    with convention => c;

    vtable   : access CimIcVTable;
    reserved : Reserved_Array_2;
  end record
  with convention => c;

  ---------------------------------------------------------------------------
  -- Utilities & Errors
  ---------------------------------------------------------------------------

  function get_plugin_path return String;

  --! summary: Return the latest Cim error for the current thread.
  --! notes: Each non-diagnostic Cim operation resets the stored value to
  --! `CIM_ERROR_NONE` before it runs. A failure may replace it. Calling
  --! `get_last_error` or `strerror` does not change the stored value.
  function get_last_error return Error;
  function strerror (err : Error) return String;

end Cim;
