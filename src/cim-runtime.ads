-- ============================================================================
-- cim-runtime.ads
-- Copyright (c) 2023-2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Interfaces.C;
with Interfaces.C.Strings;
with System;

package Cim.Runtime is

  ---------------------------------------------------------------------------
  -- Thread-local error state
  ---------------------------------------------------------------------------

  procedure set_error (err : Error);
  function get_error return Error;

  ---------------------------------------------------------------------------
  -- Plugin lifecycle and input context operations
  ---------------------------------------------------------------------------

  function ic_create return CimIcHandle;
  procedure ic_destroy (ic : CimIcHandle);

  procedure ic_focus_in (ic : CimIcHandle);
  procedure ic_focus_out (ic : CimIcHandle);
  procedure ic_reset (ic : CimIcHandle);

  function ic_filter_event
    (ic    : CimIcHandle;
     event : CimEvent_Access)
  return Interfaces.C.C_bool;

  procedure ic_set_cursor_pos
    (ic   : CimIcHandle;
     area : CimRect_Access);

  procedure ic_set_callbacks
    (ic        : CimIcHandle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address);

  function ic_get_preedit (ic : CimIcHandle) return CimPreedit_Access;
  function ic_get_candidate (ic : CimIcHandle) return CimCandidate_Access;

  procedure ic_activate_candidate_item
    (ic  : CimIcHandle;
     row : Interfaces.C.unsigned;
     col : Interfaces.C.unsigned);

  procedure ic_change_candidate_page
    (ic         : CimIcHandle;
     page_index : Interfaces.C.unsigned);

  function dup_plugin_path return Interfaces.C.Strings.chars_ptr;

end Cim.Runtime;
