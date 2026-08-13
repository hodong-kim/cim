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
  use type Clair.Status.Code;
  use type Interfaces.C.Strings.chars_ptr;
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

  procedure cim_unhandled_exception_c
    (operation : Interfaces.C.Strings.chars_ptr)
  with import,
       convention    => c,
       external_name => "cim_unhandled_exception_c";

  pragma no_return (cim_unhandled_exception_c);

  function cim_get_plugin_symbol_c return Interfaces.C.Strings.chars_ptr
  with import,
       convention    => c,
       external_name => "cim_get_plugin_symbol_c";

  function cim_get_thread_token_c return System.Address
  with import,
       convention    => c,
       external_name => "cim_get_thread_token_c";

  procedure cim_thread_yield_c
  with import,
       convention    => c,
       external_name => "cim_thread_yield_c";

  procedure contract_violation (message : String);
  pragma no_return (contract_violation);

  procedure unhandled_exception (operation : String);
  pragma no_return (unhandled_exception);

  procedure private_require_idle_for_finalization
  with export,
       convention    => c,
       external_name =>
         "cim_private_runtime_require_idle_for_finalization";

  procedure contract_violation (message : String) is
    c_message : aliased Interfaces.C.char_array :=
      Interfaces.C.To_C (message);
  begin
    cim_contract_violation_c
      (Interfaces.C.Strings.To_Chars_Ptr
         (c_message'Unchecked_Access));
  end contract_violation;

  procedure unhandled_exception (operation : String) is
    c_operation : aliased Interfaces.C.char_array :=
      Interfaces.C.To_C (operation);
  begin
    cim_unhandled_exception_c
      (Interfaces.C.Strings.To_Chars_Ptr
         (c_operation'Unchecked_Access));
  end unhandled_exception;

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

  -- Only transition metadata is protected. Loader operations and plugin
  -- callbacks run outside protected actions.
  package Plugin_State is

    procedure ref
      (plugin_out : out CimPlugin_Access;
       success    : out Boolean);

    procedure unref;

    function require_ready_plugin return CimPlugin_Access;
    function is_idle return Boolean;

  end Plugin_State;

  package body Plugin_State is

    type Plugin_Lifecycle_State is
      (Plugin_Unloaded,
       Plugin_Loading,
       Plugin_Ready,
       Plugin_Closing);

    type Ref_Action is
      (Ref_Ready,
       Ref_Load,
       Ref_Wait,
       Ref_Reentrant,
       Ref_Invalid_State,
       Ref_Count_Overflow);

    type Unref_Action is
      (Unref_Done,
       Unref_Close,
       Unref_Invalid_State);

    type Plugin_Instance is record
      dl_handle   : Clair.DL.Handle := Clair.DL.NULL_HANDLE;
      plugin      : CimPlugin_Access := null;
      initialized : Boolean := False;
    end record;

    EMPTY_PLUGIN_INSTANCE : constant Plugin_Instance
                          := (dl_handle   => Clair.DL.NULL_HANDLE,
                              plugin      => null,
                              initialized => False);

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

    -- Keep lifecycle ownership independent from Ada task registration.
    function current_owner return System.Address is
    begin
      return cim_get_thread_token_c;
    end current_owner;

    protected State is

      procedure begin_ref
        (owner      : System.Address;
         action     : out Ref_Action;
         plugin_out : out CimPlugin_Access);

      procedure complete_load
        (owner    : System.Address;
         loaded   : Plugin_Instance;
         accepted : out Boolean);

      procedure cancel_load
        (owner    : System.Address;
         accepted : out Boolean);

      procedure begin_unref
        (owner    : System.Address;
         action   : out Unref_Action;
         to_close : out Plugin_Instance);

      procedure complete_close
        (owner    : System.Address;
         accepted : out Boolean);

      function get_ready_plugin return CimPlugin_Access;
      function is_idle return Boolean;

    private

      lifecycle_state  : Plugin_Lifecycle_State := Plugin_Unloaded;
      instance         : Plugin_Instance := EMPTY_PLUGIN_INSTANCE;
      transition_owner : System.Address := System.NULL_ADDRESS;
      -- Counts live or in-progress input contexts, not API calls.
      ref_count : Natural := 0;

    end State;

    protected body State is

      procedure begin_ref
        (owner      : System.Address;
         action     : out Ref_Action;
         plugin_out : out CimPlugin_Access)
      is
      begin
        plugin_out := null;

        case lifecycle_state is
          when Plugin_Unloaded =>
            if instance /= EMPTY_PLUGIN_INSTANCE or else
               transition_owner /= System.NULL_ADDRESS or else
               ref_count /= 0
            then
              action := Ref_Invalid_State;
              return;
            end if;

            lifecycle_state  := Plugin_Loading;
            transition_owner := owner;
            action           := Ref_Load;

          when Plugin_Ready =>
            if instance.dl_handle = Clair.DL.NULL_HANDLE or else
               instance.plugin = null or else
               not instance.initialized or else
               transition_owner /= System.NULL_ADDRESS or else
               ref_count = 0
            then
              action := Ref_Invalid_State;
              return;
            end if;

            if ref_count = Natural'last then
              action := Ref_Count_Overflow;
              return;
            end if;

            ref_count  := ref_count + 1;
            plugin_out := instance.plugin;
            action     := Ref_Ready;

          when Plugin_Loading | Plugin_Closing =>
            if transition_owner = owner then
              action := Ref_Reentrant;
            else
              action := Ref_Wait;
            end if;
        end case;
      end begin_ref;

      procedure complete_load
        (owner    : System.Address;
         loaded   : Plugin_Instance;
         accepted : out Boolean)
      is
      begin
        accepted := False;

        if lifecycle_state /= Plugin_Loading or else
           transition_owner /= owner or else
           instance /= EMPTY_PLUGIN_INSTANCE or else
           ref_count /= 0 or else
           loaded.dl_handle = Clair.DL.NULL_HANDLE or else
           loaded.plugin = null or else
           not loaded.initialized
        then
          return;
        end if;

        instance         := loaded;
        ref_count        := 1;
        transition_owner := System.NULL_ADDRESS;
        lifecycle_state  := Plugin_Ready;
        accepted         := True;
      end complete_load;

      procedure cancel_load
        (owner    : System.Address;
         accepted : out Boolean)
      is
      begin
        accepted := False;

        if lifecycle_state /= Plugin_Loading or else
           transition_owner /= owner or else
           instance /= EMPTY_PLUGIN_INSTANCE or else
           ref_count /= 0
        then
          return;
        end if;

        instance         := EMPTY_PLUGIN_INSTANCE;
        ref_count        := 0;
        transition_owner := System.NULL_ADDRESS;
        lifecycle_state  := Plugin_Unloaded;
        accepted         := True;
      end cancel_load;

      procedure begin_unref
        (owner    : System.Address;
         action   : out Unref_Action;
         to_close : out Plugin_Instance)
      is
      begin
        to_close := EMPTY_PLUGIN_INSTANCE;

        if lifecycle_state /= Plugin_Ready or else
           instance.dl_handle = Clair.DL.NULL_HANDLE or else
           instance.plugin = null or else
           not instance.initialized or else
           transition_owner /= System.NULL_ADDRESS or else
           ref_count = 0
        then
          action := Unref_Invalid_State;
          return;
        end if;

        ref_count := ref_count - 1;

        if ref_count /= 0 then
          action := Unref_Done;
          return;
        end if;

        to_close         := instance;
        instance         := EMPTY_PLUGIN_INSTANCE;
        transition_owner := owner;
        lifecycle_state  := Plugin_Closing;
        action           := Unref_Close;
      end begin_unref;

      procedure complete_close
        (owner    : System.Address;
         accepted : out Boolean)
      is
      begin
        accepted := False;

        if lifecycle_state /= Plugin_Closing or else
           transition_owner /= owner or else
           instance /= EMPTY_PLUGIN_INSTANCE or else
           ref_count /= 0
        then
          return;
        end if;

        transition_owner := System.NULL_ADDRESS;
        lifecycle_state  := Plugin_Unloaded;
        accepted         := True;
      end complete_close;

      function get_ready_plugin return CimPlugin_Access is
      begin
        if lifecycle_state /= Plugin_Ready or else
           instance.dl_handle = Clair.DL.NULL_HANDLE or else
           instance.plugin = null or else
           not instance.initialized or else
           transition_owner /= System.NULL_ADDRESS or else
           ref_count = 0
        then
          return null;
        end if;

        return instance.plugin;
      end get_ready_plugin;

      function is_idle return Boolean is
      begin
        return lifecycle_state = Plugin_Unloaded and then
               instance = EMPTY_PLUGIN_INSTANCE and then
               transition_owner = System.NULL_ADDRESS and then
               ref_count = 0;
      end is_idle;

    end State;

    procedure close_instance (item : in out Plugin_Instance) is
      retval : Clair.Status.Code;
    begin
      if item.initialized and then
         item.plugin /= null and then
         item.plugin.fini /= null
      then
        item.plugin.fini.all;
      end if;

      item.initialized := False;

      if item.dl_handle /= Clair.DL.NULL_HANDLE then
        retval := Clair.DL.close (item.dl_handle);
        item.dl_handle := Clair.DL.NULL_HANDLE;
        item.plugin := null;

        if retval = Clair.Status.LIBRARY_CLOSE_ERROR then
          if get_error = CIM_ERROR_NONE then
            set_error (CIM_ERROR_DLCLOSE_FAILED);
          end if;
        elsif retval /= Clair.Status.OK then
          contract_violation ("Clair.DL.close returned unexpected status");
        end if;
      else
        item.plugin := null;
      end if;
    end close_instance;

    procedure load_instance
      (item    : out Plugin_Instance;
       success : out Boolean)
    is
      c_path         : Interfaces.C.Strings.chars_ptr
                     := Interfaces.C.Strings.NULL_PTR;
      path_error     : Error;
      plugin_address : System.Address;
      retval         : Clair.Status.Code;
    begin
      item    := EMPTY_PLUGIN_INSTANCE;
      success := False;

      path_error := cim_get_plugin_path_c (c_path);

      if path_error /= CIM_ERROR_NONE then
        set_error (path_error);
        return;
      end if;

      retval := Clair.DL.open
        (path   => c_path,
         result => item.dl_handle);

      cim_free_plugin_path_c (c_path);
      c_path := Interfaces.C.Strings.NULL_PTR;

      if retval = Clair.Status.LIBRARY_LOAD_ERROR then
        if item.dl_handle /= Clair.DL.NULL_HANDLE then
          contract_violation
            ("Clair.DL.open returned a handle after load failure");
        end if;

        set_error (CIM_ERROR_DLOPEN_FAILED);
        return;
      elsif retval /= Clair.Status.OK then
        contract_violation ("Clair.DL.open returned unexpected status");
      end if;

      retval := Clair.DL.find_symbol
        (lib      => item.dl_handle,
         sym_name => cim_get_plugin_symbol_c,
         result   => plugin_address);

      if retval = Clair.Status.SYMBOL_LOOKUP_ERROR then
        set_error (CIM_ERROR_DLSYM_FAILED);
        close_instance (item);
        return;
      elsif retval /= Clair.Status.OK then
        contract_violation
          ("Clair.DL.find_symbol returned unexpected status");
      end if;

      item.plugin := to_cim_plugin_access (plugin_address);

      if item.plugin = null then
        set_error (CIM_ERROR_NULL_PLUGIN);
        close_instance (item);
        return;
      end if;

      if item.plugin.cim_api_major /= CIM_MAJOR_VERSION then
        set_error (CIM_ERROR_BAD_ABI);
        close_instance (item);
        return;
      end if;

      if item.plugin.vtable = null or else
         item.plugin.vtable.create = null or else
         item.plugin.vtable.destroy = null
      then
        set_error (CIM_ERROR_INVALID_PLUGIN);
        close_instance (item);
        return;
      end if;

      if item.plugin.init /= null and then item.plugin.init.all /= 0 then
        set_error (CIM_ERROR_INIT_FAILED);
        close_instance (item);
        return;
      end if;

      item.initialized := True;
      set_error (CIM_ERROR_NONE);
      success := True;
    exception
      when others =>
        if c_path /= Interfaces.C.Strings.NULL_PTR then
          cim_free_plugin_path_c (c_path);
        end if;

        if item.dl_handle /= Clair.DL.NULL_HANDLE then
          close_instance (item);
        end if;

        raise;
    end load_instance;

    procedure ref
      (plugin_out : out CimPlugin_Access;
       success    : out Boolean)
    is
      owner          : constant System.Address := current_owner;
      action         : Ref_Action;
      accepted       : Boolean;
      loaded         : Plugin_Instance := EMPTY_PLUGIN_INSTANCE;
      loaded_success : Boolean;
    begin
      plugin_out := null;
      success    := False;

      loop
        State.begin_ref (owner, action, plugin_out);

        case action is
          when Ref_Ready =>
            success := True;
            return;

          when Ref_Load =>
            begin
              load_instance (loaded, loaded_success);
            exception
              when others =>
                State.cancel_load (owner, accepted);

                if not accepted then
                  contract_violation
                    ("Plugin_State.ref: failed to cancel loading");
                end if;

                raise;
            end;

            if not loaded_success then
              State.cancel_load (owner, accepted);

              if not accepted then
                contract_violation
                  ("Plugin_State.ref: failed to cancel loading");
              end if;

              return;
            end if;

            State.complete_load (owner, loaded, accepted);

            if not accepted then
              close_instance (loaded);
              contract_violation
                ("Plugin_State.ref: failed to publish loaded plugin");
            end if;

            plugin_out := loaded.plugin;
            success    := True;
            return;

          when Ref_Wait =>
            -- Do not enqueue foreign C threads on an Ada protected entry.
            -- Lifecycle transitions are rare, so yield and retry instead.
            cim_thread_yield_c;

          when Ref_Reentrant =>
            contract_violation
              ("Cim plugin lifecycle callback re-entered Cim");

          when Ref_Invalid_State =>
            contract_violation
              ("Plugin_State.ref: invalid lifecycle state");

          when Ref_Count_Overflow =>
            contract_violation
              ("Plugin_State.ref: ref_count overflow");
        end case;
      end loop;
    end ref;

    procedure unref is
      owner    : constant System.Address := current_owner;
      action   : Unref_Action;
      accepted : Boolean;
      to_close : Plugin_Instance;
    begin
      State.begin_unref (owner, action, to_close);

      case action is
        when Unref_Done =>
          null;

        when Unref_Close =>
          begin
            close_instance (to_close);
          exception
            when others =>
              State.complete_close (owner, accepted);

              if not accepted then
                contract_violation
                  ("Plugin_State.unref: failed to complete closing");
              end if;

              raise;
          end;

          State.complete_close (owner, accepted);

          if not accepted then
            contract_violation
              ("Plugin_State.unref: failed to complete closing");
          end if;

        when Unref_Invalid_State =>
          contract_violation
            ("Plugin_State.unref: invalid lifecycle state");
      end case;
    end unref;

    function require_ready_plugin return CimPlugin_Access is
      plugin : constant CimPlugin_Access := State.get_ready_plugin;
    begin
      if plugin = null then
        contract_violation
          ("Cim input context operation requires a ready plugin");
      end if;

      return plugin;
    end require_ready_plugin;

    function is_idle return Boolean is
    begin
      return State.is_idle;
    end is_idle;

  end Plugin_State;

  procedure private_require_idle_for_finalization is
  begin
    if not Plugin_State.is_idle then
      contract_violation
        ("Cim finalization requested while plugin state is active");
    end if;
  exception
    when others =>
      unhandled_exception
        ("cim_private_runtime_require_idle_for_finalization");
  end private_require_idle_for_finalization;

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
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_destroy: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.destroy /= null then
      plugin.vtable.destroy (ic);
    end if;

    Plugin_State.unref;
  end ic_destroy;

  procedure ic_focus_in (ic : CimIcHandle) is
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_focus_in: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.focus_in /= null then
      plugin.vtable.focus_in (ic);
    end if;
  end ic_focus_in;

  procedure ic_focus_out (ic : CimIcHandle) is
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_focus_out: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.focus_out /= null then
      plugin.vtable.focus_out (ic);
    end if;
  end ic_focus_out;

  procedure ic_reset (ic : CimIcHandle) is
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_reset: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.reset /= null then
      plugin.vtable.reset (ic);
    end if;
  end ic_reset;

  function ic_filter_event
    (ic    : CimIcHandle;
     event : CimEvent_Access)
  return Interfaces.C.C_bool is
    plugin : CimPlugin_Access;
    retval : Interfaces.C.C_bool := Interfaces.C.C_bool (False);
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_filter_event: ic is NULL");
    elsif event = null then
      contract_violation ("cim_ic_filter_event: event is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.filter_event /= null then
      retval := plugin.vtable.filter_event (ic, event);
    end if;

    return retval;
  end ic_filter_event;

  procedure ic_set_cursor_pos
    (ic   : CimIcHandle;
     area : CimRect_Access) is
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_set_cursor_pos: ic is NULL");
    elsif area = null then
      contract_violation ("cim_ic_set_cursor_pos: area is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.set_cursor_pos /= null then
      plugin.vtable.set_cursor_pos (ic, area);
    end if;
  end ic_set_cursor_pos;

  procedure ic_set_callbacks
    (ic        : CimIcHandle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address) is
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_set_callbacks: ic is NULL");
    elsif callbacks = null then
      contract_violation ("cim_ic_set_callbacks: callbacks is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.set_callbacks /= null then
      plugin.vtable.set_callbacks (ic, callbacks, user_data);
    end if;
  end ic_set_callbacks;

  function ic_get_preedit (ic : CimIcHandle) return CimPreedit_Access is
    plugin : CimPlugin_Access;
    retval : CimPreedit_Access := null;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_get_preedit: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.get_preedit /= null then
      retval := plugin.vtable.get_preedit (ic);
    end if;

    return retval;
  end ic_get_preedit;

  function ic_get_candidate (ic : CimIcHandle) return CimCandidate_Access is
    plugin : CimPlugin_Access;
    retval : CimCandidate_Access := null;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_get_candidate: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then plugin.vtable.get_candidate /= null then
      retval := plugin.vtable.get_candidate (ic);
    end if;

    return retval;
  end ic_get_candidate;

  procedure ic_activate_candidate_item
    (ic  : CimIcHandle;
     row : Interfaces.C.unsigned;
     col : Interfaces.C.unsigned) is
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_activate_candidate_item: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then
      plugin.vtable.activate_candidate_item /= null
    then
      plugin.vtable.activate_candidate_item (ic, row, col);
    end if;
  end ic_activate_candidate_item;

  procedure ic_change_candidate_page
    (ic         : CimIcHandle;
     page_index : Interfaces.C.unsigned) is
    plugin : CimPlugin_Access;
  begin
    if ic = NULL_IC then
      contract_violation ("cim_ic_change_candidate_page: ic is NULL");
    end if;

    set_error (CIM_ERROR_NONE);
    plugin := Plugin_State.require_ready_plugin;

    if plugin.vtable /= null and then
      plugin.vtable.change_candidate_page /= null
    then
      plugin.vtable.change_candidate_page (ic, page_index);
    end if;
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
