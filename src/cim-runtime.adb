-- ============================================================================
-- cim-runtime.adb
-- Copyright (c) 2023-2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Ada.Unchecked_Conversion;
with Clair.DL;
with Clair.Status;

package body Cim.Runtime is

  use type Clair.DL.Handle;
  use type Clair.DL.Open_Flags;
  use type Clair.Status.Code;
  use type System.Address;
  use type Interfaces.C.unsigned;
  use type Interfaces.C.int;

  type CimPlugin_Access is access all CimPlugin;

  last_error : Error := CIM_ERROR_NONE;
  pragma thread_local_storage (last_error);

  procedure cim_contract_violation_c
    (message : Interfaces.C.Strings.chars_ptr)
  with import,
       convention    => c,
       external_name => "cim_contract_violation_c";

  pragma no_return (cim_contract_violation_c);

  function cim_get_plugin_symbol_c return Interfaces.C.Strings.chars_ptr
  with import,
       convention    => c,
       external_name => "cim_get_plugin_symbol_c";

  procedure contract_violation (message : String);
  pragma no_return (contract_violation);

  procedure contract_violation (message : String) is
    c_message : aliased Interfaces.C.char_array :=
      Interfaces.C.To_C (message);
  begin
    cim_contract_violation_c
      (Interfaces.C.Strings.To_Chars_Ptr
         (c_message'Unchecked_Access));
  end contract_violation;

  procedure set_error (err : Error) is
  begin
    last_error := err;
  end set_error;

  function get_error return Error is
  begin
    return last_error;
  end get_error;

  function to_cim_plugin_access is new Ada.Unchecked_Conversion
    (source => System.Address,
     target => CimPlugin_Access);

  ---------------------------------------------------------------------------
  -- Plugin state
  ---------------------------------------------------------------------------

  protected Plugin_State is

    procedure ref
      (plugin_out : out CimPlugin_Access;
       success    : out Boolean);

    procedure unref;

  private

    dl_handle          : Clair.DL.Handle := Clair.DL.NULL_HANDLE;
    plugin             : CimPlugin_Access := null;
    plugin_initialized : Boolean := False;
    ref_count          : Natural := 0;

    procedure open_plugin_locked (success : out Boolean);
    procedure close_plugin_locked;

  end Plugin_State;

  protected body Plugin_State is

    procedure open_plugin_locked (success : out Boolean) is
      function cim_get_plugin_path_c
        (path_out : out Interfaces.C.Strings.chars_ptr)
      return Error
      with import,
           convention    => c,
           external_name => "cim_get_plugin_path_c";

      procedure cim_free_plugin_path_c
        (path : Interfaces.C.Strings.chars_ptr)
      with import,
           convention    => c,
           external_name => "cim_free_plugin_path_c";

      c_path         : Interfaces.C.Strings.chars_ptr
                     := Interfaces.C.Strings.NULL_PTR;
      path_error     : Error;
      plugin_address : System.Address;
      retval         : Clair.Status.Code;
    begin
      success := False;

      if dl_handle /= Clair.DL.NULL_HANDLE and then plugin /= null then
        success := True;
        return;
      end if;

      path_error := cim_get_plugin_path_c (c_path);

      if path_error /= CIM_ERROR_NONE then
        set_error (path_error);
        return;
      end if;

      retval := Clair.DL.open
        (path   => c_path,
         mode   => Clair.DL.RTLD_LAZY or Clair.DL.RTLD_LOCAL,
         result => dl_handle);
      cim_free_plugin_path_c (c_path);
      c_path := Interfaces.C.Strings.NULL_PTR;

      if retval = Clair.Status.LIBRARY_LOAD_ERROR then
        set_error (CIM_ERROR_DLOPEN_FAILED);
        return;
      elsif retval /= Clair.Status.OK then
        contract_violation ("Clair.DL.open returned unexpected status");
      end if;

      retval := Clair.DL.find_symbol
        (lib      => dl_handle,
         sym_name => cim_get_plugin_symbol_c,
         result   => plugin_address);

      if retval = Clair.Status.SYMBOL_LOOKUP_ERROR then
        set_error (CIM_ERROR_DLSYM_FAILED);
        close_plugin_locked;
        return;
      elsif retval /= Clair.Status.OK then
        contract_violation
          ("Clair.DL.find_symbol returned unexpected status");
      end if;

      plugin := to_cim_plugin_access (plugin_address);

      if plugin = null then
        set_error (CIM_ERROR_NULL_PLUGIN);
        close_plugin_locked;
        return;
      end if;

      if plugin.cim_api_major /= CIM_MAJOR_VERSION then
        set_error (CIM_ERROR_BAD_ABI);
        close_plugin_locked;
        return;
      end if;

      if plugin.vtable = null or else
         plugin.vtable.create = null or else
         plugin.vtable.destroy = null
      then
        set_error (CIM_ERROR_INVALID_PLUGIN);
        close_plugin_locked;
        return;
      end if;

      if plugin.init /= null then
        if plugin.init.all /= 0 then
          set_error (CIM_ERROR_INIT_FAILED);
          close_plugin_locked;
          return;
        end if;
      end if;

      plugin_initialized := True;
      set_error (CIM_ERROR_NONE);
      success := True;
    end open_plugin_locked;

    procedure close_plugin_locked is
      handle_to_close : Clair.DL.Handle := dl_handle;
      plugin_to_close : constant CimPlugin_Access := plugin;
      was_initialized : constant Boolean := plugin_initialized;
      retval          : Clair.Status.Code;
    begin
      -- Invalidate the protected state before calling external teardown code.
      -- A finalized plugin must never be reused after teardown starts.
      dl_handle          := Clair.DL.NULL_HANDLE;
      plugin             := null;
      plugin_initialized := False;

      if was_initialized and then
         plugin_to_close /= null and then
         plugin_to_close.fini /= null
      then
        plugin_to_close.fini.all;
      end if;

      if handle_to_close /= Clair.DL.NULL_HANDLE then
        retval := Clair.DL.close (handle_to_close);

        if retval = Clair.Status.LIBRARY_CLOSE_ERROR then
          -- Preserve a more specific error that initiated teardown.
          if get_error = CIM_ERROR_NONE then
            set_error (CIM_ERROR_DLCLOSE_FAILED);
          end if;
        elsif retval /= Clair.Status.OK then
          contract_violation ("Clair.DL.close returned unexpected status");
        end if;
      end if;
    end close_plugin_locked;

    procedure ref
      (plugin_out : out CimPlugin_Access;
       success    : out Boolean) is
    begin
      plugin_out := null;
      success := False;

      if plugin = null then
        open_plugin_locked (success);

        if not success then
          return;
        end if;
      end if;

      ref_count := ref_count + 1;
      plugin_out := plugin;
      success := True;
    end ref;

    procedure unref is
    begin
      if ref_count = 0 then
        contract_violation ("Plugin_State.unref: ref_count is zero");
      end if;

      ref_count := ref_count - 1;

      if ref_count = 0 and then plugin /= null then
        close_plugin_locked;
      end if;
    end unref;

  end Plugin_State;

  ---------------------------------------------------------------------------
  -- Public API implementation
  ---------------------------------------------------------------------------

  function ic_create return CimIcHandle is
    plugin  : CimPlugin_Access;
    success : Boolean;
    ic      : CimIcHandle;
  begin
    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return NULL_IC;
    end if;

    ic := plugin.vtable.create.all;

    if ic = NULL_IC then
      set_error (CIM_ERROR_CREATE_FAILED);
      Plugin_State.unref;
      return NULL_IC;
    end if;

    return ic;
  end ic_create;

  procedure ic_destroy (ic : CimIcHandle) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_destroy: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);

    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then plugin.vtable.destroy /= null then
      plugin.vtable.destroy (ic);
    end if;

    Plugin_State.unref;
    Plugin_State.unref;
  end ic_destroy;

  procedure ic_focus_in (ic : CimIcHandle) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_focus_in: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then plugin.vtable.focus_in /= null then
      plugin.vtable.focus_in (ic);
    end if;

    Plugin_State.unref;
  end ic_focus_in;

  procedure ic_focus_out (ic : CimIcHandle) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_focus_out: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then plugin.vtable.focus_out /= null then
      plugin.vtable.focus_out (ic);
    end if;

    Plugin_State.unref;
  end ic_focus_out;

  procedure ic_reset (ic : CimIcHandle) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_reset: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then plugin.vtable.reset /= null then
      plugin.vtable.reset (ic);
    end if;

    Plugin_State.unref;
  end ic_reset;

  function ic_filter_event
    (ic    : CimIcHandle;
     event : CimEvent_Access)
  return Interfaces.C.C_bool is
    plugin  : CimPlugin_Access;
    success : Boolean;
    retval  : Interfaces.C.C_bool := Interfaces.C.C_bool (False);
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_filter_event: ic is NULL");
    elsif event = null then
      contract_violation ("cim_ic_filter_event: event is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return Interfaces.C.C_bool (False);
    end if;

    if plugin.vtable /= null and then plugin.vtable.filter_event /= null then
      retval := plugin.vtable.filter_event (ic, event);
    end if;

    Plugin_State.unref;
    return retval;
  end ic_filter_event;

  procedure ic_set_cursor_pos
    (ic   : CimIcHandle;
     area : CimRect_Access) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_set_cursor_pos: ic is NULL");
    elsif area = null then
      contract_violation ("cim_ic_set_cursor_pos: area is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then plugin.vtable.set_cursor_pos /= null then
      plugin.vtable.set_cursor_pos (ic, area);
    end if;

    Plugin_State.unref;
  end ic_set_cursor_pos;

  procedure ic_set_callbacks
    (ic        : CimIcHandle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_set_callbacks: ic is NULL");
    elsif callbacks = null then
      contract_violation ("cim_ic_set_callbacks: callbacks is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then plugin.vtable.set_callbacks /= null then
      plugin.vtable.set_callbacks (ic, callbacks, user_data);
    end if;

    Plugin_State.unref;
  end ic_set_callbacks;

  function ic_get_preedit (ic : CimIcHandle) return CimPreedit_Access is
    plugin  : CimPlugin_Access;
    success : Boolean;
    retval  : CimPreedit_Access := null;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_get_preedit: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return null;
    end if;

    if plugin.vtable /= null and then plugin.vtable.get_preedit /= null then
      retval := plugin.vtable.get_preedit (ic);
    end if;

    Plugin_State.unref;
    return retval;
  end ic_get_preedit;

  function ic_get_candidate (ic : CimIcHandle) return CimCandidate_Access is
    plugin  : CimPlugin_Access;
    success : Boolean;
    retval  : CimCandidate_Access := null;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_get_candidate: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return null;
    end if;

    if plugin.vtable /= null and then plugin.vtable.get_candidate /= null then
      retval := plugin.vtable.get_candidate (ic);
    end if;

    Plugin_State.unref;
    return retval;
  end ic_get_candidate;

  procedure ic_activate_candidate_item
    (ic  : CimIcHandle;
     row : Interfaces.C.unsigned;
     col : Interfaces.C.unsigned) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_activate_candidate_item: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then
      plugin.vtable.activate_candidate_item /= null
    then
      plugin.vtable.activate_candidate_item (ic, row, col);
    end if;

    Plugin_State.unref;
  end ic_activate_candidate_item;

  procedure ic_change_candidate_page
    (ic         : CimIcHandle;
     page_index : Interfaces.C.unsigned) is
    plugin  : CimPlugin_Access;
    success : Boolean;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_change_candidate_page: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    Plugin_State.ref (plugin, success);

    if not success then
      return;
    end if;

    if plugin.vtable /= null and then
      plugin.vtable.change_candidate_page /= null
    then
      plugin.vtable.change_candidate_page (ic, page_index);
    end if;

    Plugin_State.unref;
  end ic_change_candidate_page;

  function dup_plugin_path return Interfaces.C.Strings.chars_ptr is
    use Interfaces.C.Strings;

    function cim_get_plugin_path_c (path_out : out chars_ptr)
    return Error
    with import,
         convention    => c,
         external_name => "cim_get_plugin_path_c";

    c_path     : chars_ptr;
    path_error : Error;
  begin
    path_error := cim_get_plugin_path_c (c_path);
    set_error (path_error);
    return c_path;
  end dup_plugin_path;

end Cim.Runtime;
