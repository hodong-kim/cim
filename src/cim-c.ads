-- ============================================================================
-- cim-c.ads
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Interfaces.C;
with Interfaces.C.Strings;
with System;

package Cim.C is

  ---------------------------------------------------------------------------
  -- Input Context Lifecycle
  ---------------------------------------------------------------------------

  function ic_create return CimIcHandle
  with convention => c;

  procedure ic_destroy (ic : CimIcHandle)
  with convention => c;

  ---------------------------------------------------------------------------
  -- Input Context Operations
  ---------------------------------------------------------------------------

  procedure ic_focus_in (ic : CimIcHandle)
  with convention => c;

  procedure ic_focus_out (ic : CimIcHandle)
  with convention => c;

  procedure ic_reset (ic : CimIcHandle)
  with convention => c;

  function ic_filter_event
    (ic    : CimIcHandle;
     event : CimEvent_Access)
  return Interfaces.C.C_bool
  with convention => c;

  procedure ic_set_cursor_pos
    (ic   : CimIcHandle;
     area : CimRect_Access)
  with convention => c;

  procedure ic_set_callbacks
    (ic        : CimIcHandle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address)
  with convention => c;

  function ic_get_preedit
    (ic : CimIcHandle)
  return CimPreedit_Access
  with convention => c;

  function ic_get_candidate
    (ic : CimIcHandle)
  return CimCandidate_Access
  with convention => c;

  procedure ic_activate_candidate_item
    (ic  : CimIcHandle;
     row : Interfaces.C.unsigned;
     col : Interfaces.C.unsigned)
  with convention => c;

  procedure ic_change_candidate_page
    (ic         : CimIcHandle;
     page_index : Interfaces.C.unsigned)
  with convention => c;

  ---------------------------------------------------------------------------
  -- Utilities & Errors
  ---------------------------------------------------------------------------

  function dup_plugin_path
  return Interfaces.C.Strings.chars_ptr
  with convention => c;

  function get_last_error
  return Error
  with convention => c;

  function strerror
    (err : Interfaces.C.unsigned)
  return Interfaces.C.Strings.chars_ptr
  with convention => c;

end Cim.C;
