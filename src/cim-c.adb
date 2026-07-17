-- src/cim-c.adb
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD

with Cim.Runtime;

package body Cim.C is

  package C_Strings renames Interfaces.C.Strings;

  procedure cim_contract_violation_c
    (message : C_Strings.chars_ptr)
  with import,
       convention    => c,
       external_name => "cim_contract_violation_c";

  pragma no_return (cim_contract_violation_c);

  procedure cim_unhandled_exception_c
    (operation : C_Strings.chars_ptr)
  with import,
       convention    => c,
       external_name => "cim_unhandled_exception_c";

  pragma no_return (cim_unhandled_exception_c);

  procedure contract_violation (message : String);
  pragma no_return (contract_violation);

  procedure unhandled_exception (operation : String);
  pragma no_return (unhandled_exception);

  procedure contract_violation (message : String) is
    c_message : aliased Interfaces.C.char_array
              := Interfaces.C.To_C (message);
  begin
    cim_contract_violation_c
      (C_Strings.To_Chars_Ptr
         (c_message'Unchecked_Access));
  end contract_violation;

  procedure unhandled_exception (operation : String) is
    c_operation : aliased Interfaces.C.char_array :=
      Interfaces.C.To_C (operation);
  begin
    cim_unhandled_exception_c
      (C_Strings.To_Chars_Ptr
         (c_operation'Unchecked_Access));
  end unhandled_exception;

  -- No Ada exception may cross an exported C ABI boundary. Expected external
  -- failures must be represented by the documented return value and Cim.Error.
  -- Any remaining exception indicates an unexpected internal failure.

  ---------------------------------------------------------------------------
  -- Static C strings returned by cim_strerror
  ---------------------------------------------------------------------------

  ERROR_NONE_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("no error");

  ERROR_INVALID_ARGUMENT_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("invalid argument");

  ERROR_HOME_NOT_SET_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("HOME is not set");

  ERROR_PLUGIN_PATH_TOO_LONG_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("plugin link path is too long");

  ERROR_MUTEX_FAILED_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("mutex operation failed");

  ERROR_DLOPEN_FAILED_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("failed to load plugin");

  ERROR_DLCLOSE_FAILED_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("failed to unload plugin");

  ERROR_DLSYM_FAILED_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("failed to resolve cim_plugin");

  ERROR_NULL_PLUGIN_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("plugin descriptor is NULL");

  ERROR_BAD_ABI_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("incompatible plugin ABI");

  ERROR_INVALID_PLUGIN_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("invalid plugin");

  ERROR_INIT_FAILED_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("plugin initialization failed");

  ERROR_CREATE_FAILED_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("plugin create failed");

  ERROR_ALLOCATION_FAILED_STRING : constant C_Strings.chars_ptr :=
    C_Strings.New_String ("memory allocation failed");

  ---------------------------------------------------------------------------
  -- Exported C API entry points
  ---------------------------------------------------------------------------

  function ic_create return CimIcHandle is
  begin
    return Cim.Runtime.ic_create;
  exception
    when others =>
      unhandled_exception ("cim_ic_create");
  end ic_create;

  procedure ic_destroy (ic : CimIcHandle) is
  begin
    Cim.Runtime.ic_destroy (ic);
  exception
    when others =>
      unhandled_exception ("cim_ic_destroy");
  end ic_destroy;

  procedure ic_focus_in (ic : CimIcHandle) is
  begin
    Cim.Runtime.ic_focus_in (ic);
  exception
    when others =>
      unhandled_exception ("cim_ic_focus_in");
  end ic_focus_in;

  procedure ic_focus_out (ic : CimIcHandle) is
  begin
    Cim.Runtime.ic_focus_out (ic);
  exception
    when others =>
      unhandled_exception ("cim_ic_focus_out");
  end ic_focus_out;

  procedure ic_reset (ic : CimIcHandle) is
  begin
    Cim.Runtime.ic_reset (ic);
  exception
    when others =>
      unhandled_exception ("cim_ic_reset");
  end ic_reset;

  function ic_filter_event
    (ic    : CimIcHandle;
     event : CimEvent_Access)
  return Interfaces.C.C_bool is
  begin
    return Cim.Runtime.ic_filter_event (ic, event);
  exception
    when others =>
      unhandled_exception ("cim_ic_filter_event");
  end ic_filter_event;

  procedure ic_set_cursor_pos
    (ic   : CimIcHandle;
     area : CimRect_Access) is
  begin
    Cim.Runtime.ic_set_cursor_pos (ic, area);
  exception
    when others =>
      unhandled_exception ("cim_ic_set_cursor_pos");
  end ic_set_cursor_pos;

  procedure ic_set_callbacks
    (ic        : CimIcHandle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address) is
  begin
    Cim.Runtime.ic_set_callbacks (ic, callbacks, user_data);
  exception
    when others =>
      unhandled_exception ("cim_ic_set_callbacks");
  end ic_set_callbacks;

  function ic_get_preedit (ic : CimIcHandle) return CimPreedit_Access is
  begin
    return Cim.Runtime.ic_get_preedit (ic);
  exception
    when others =>
      unhandled_exception ("cim_ic_get_preedit");
  end ic_get_preedit;

  function ic_get_candidate
    (ic : CimIcHandle)
  return CimCandidate_Access is
  begin
    return Cim.Runtime.ic_get_candidate (ic);
  exception
    when others =>
      unhandled_exception ("cim_ic_get_candidate");
  end ic_get_candidate;

  procedure ic_activate_candidate_item
    (ic  : CimIcHandle;
     row : Interfaces.C.unsigned;
     col : Interfaces.C.unsigned) is
  begin
    Cim.Runtime.ic_activate_candidate_item (ic, row, col);
  exception
    when others =>
      unhandled_exception ("cim_ic_activate_candidate_item");
  end ic_activate_candidate_item;

  procedure ic_change_candidate_page
    (ic         : CimIcHandle;
     page_index : Interfaces.C.unsigned) is
  begin
    Cim.Runtime.ic_change_candidate_page (ic, page_index);
  exception
    when others =>
      unhandled_exception ("cim_ic_change_candidate_page");
  end ic_change_candidate_page;

  function dup_plugin_path return C_Strings.chars_ptr is
  begin
    Cim.Runtime.set_error (CIM_ERROR_NONE);
    return Cim.Runtime.dup_plugin_path;
  exception
    when others =>
      unhandled_exception ("cim_dup_plugin_path");
  end dup_plugin_path;

  function get_last_error return Error is
  begin
    return Cim.Runtime.get_error;
  exception
    when others =>
      unhandled_exception ("cim_get_last_error");
  end get_last_error;

  function strerror
    (err : Interfaces.C.unsigned)
  return C_Strings.chars_ptr
  is
    function to_error (value : Interfaces.C.unsigned) return Error is
    begin
      return Error'Enum_Val (Integer(value));
    exception
      when Constraint_Error =>
        contract_violation ("cim_strerror: err is out of range");
    end to_error;

    error_value : constant Error := to_error (err);
  begin
    case error_value is
      when CIM_ERROR_NONE =>
        return ERROR_NONE_STRING;
      when CIM_ERROR_INVALID_ARGUMENT =>
        return ERROR_INVALID_ARGUMENT_STRING;
      when CIM_ERROR_HOME_NOT_SET =>
        return ERROR_HOME_NOT_SET_STRING;
      when CIM_ERROR_PLUGIN_PATH_TOO_LONG =>
        return ERROR_PLUGIN_PATH_TOO_LONG_STRING;
      when CIM_ERROR_MUTEX_FAILED =>
        return ERROR_MUTEX_FAILED_STRING;
      when CIM_ERROR_DLOPEN_FAILED =>
        return ERROR_DLOPEN_FAILED_STRING;
      when CIM_ERROR_DLSYM_FAILED =>
        return ERROR_DLSYM_FAILED_STRING;
      when CIM_ERROR_NULL_PLUGIN =>
        return ERROR_NULL_PLUGIN_STRING;
      when CIM_ERROR_BAD_ABI =>
        return ERROR_BAD_ABI_STRING;
      when CIM_ERROR_INVALID_PLUGIN =>
        return ERROR_INVALID_PLUGIN_STRING;
      when CIM_ERROR_INIT_FAILED =>
        return ERROR_INIT_FAILED_STRING;
      when CIM_ERROR_CREATE_FAILED =>
        return ERROR_CREATE_FAILED_STRING;
      when CIM_ERROR_ALLOCATION_FAILED =>
        return ERROR_ALLOCATION_FAILED_STRING;
      when CIM_ERROR_DLCLOSE_FAILED =>
        return ERROR_DLCLOSE_FAILED_STRING;
    end case;
  exception
    when others =>
      unhandled_exception ("cim_strerror");
  end strerror;

end Cim.C;
