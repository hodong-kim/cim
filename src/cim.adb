-- src/cim.adb
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD

with Cim.Runtime;

package body Cim is

  ---------------------------------------------------------------------------
  -- 1. Utilities & Errors
  ---------------------------------------------------------------------------
  function get_plugin_path return String is
    use Interfaces.C.Strings;
    c_str : chars_ptr := Cim.Runtime.dup_plugin_path;
  begin
    if c_str = Null_Ptr then
      return "";
    end if;

    -- Convert the C string to an Ada String, then free the C string memory.
    declare
      result : constant String := Value (c_str);
    begin
      Free (c_str);
      return result;
    end;
  end get_plugin_path;

  function get_last_error return Cim.Error is
  begin
    return Cim.Runtime.get_error;
  end get_last_error;

  function strerror (err : Cim.Error) return String is
  begin
    -- Return a pure Ada String rather than a C-style chars_ptr.
    case err is
      when CIM_ERROR_NONE                 => return "no error";
      when CIM_ERROR_INVALID_ARGUMENT     => return "invalid argument";
      when CIM_ERROR_HOME_NOT_SET         => return "HOME is not set";
      when CIM_ERROR_PLUGIN_PATH_TOO_LONG => return "plugin link path is too long";
      when CIM_ERROR_MUTEX_FAILED         => return "mutex operation failed";
      when CIM_ERROR_DLOPEN_FAILED        => return "failed to load plugin";
      when CIM_ERROR_DLSYM_FAILED         => return "failed to resolve cim_plugin";
      when CIM_ERROR_NULL_PLUGIN          => return "plugin descriptor is NULL";
      when CIM_ERROR_BAD_ABI              => return "incompatible plugin ABI";
      when CIM_ERROR_INVALID_PLUGIN       => return "invalid plugin";
      when CIM_ERROR_INIT_FAILED          => return "plugin initialization failed";
      when CIM_ERROR_CREATE_FAILED        => return "plugin create failed";
      when CIM_ERROR_ALLOCATION_FAILED =>
        return "memory allocation failed";
      when CIM_ERROR_DLCLOSE_FAILED =>
        return "failed to unload plugin";
    end case;
  end strerror;

end Cim;
