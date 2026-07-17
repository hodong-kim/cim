-- src/cim-c.ads
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD

with Interfaces.C;
with Interfaces.C.Strings;
with System;

package Cim.C is

  ---------------------------------------------------------------------------
  -- Input Context Lifecycle
  ---------------------------------------------------------------------------

  function ic_create return CimIcHandle
  with export,
       convention    => c,
       external_name => "cim_ic_create";

  procedure ic_destroy (ic : CimIcHandle)
  with export,
       convention    => c,
       external_name => "cim_ic_destroy";

  ---------------------------------------------------------------------------
  -- Input Context Operations
  ---------------------------------------------------------------------------

  procedure ic_focus_in (ic : CimIcHandle)
  with export,
       convention    => c,
       external_name => "cim_ic_focus_in";

  procedure ic_focus_out (ic : CimIcHandle)
  with export,
       convention    => c,
       external_name => "cim_ic_focus_out";

  procedure ic_reset (ic : CimIcHandle)
  with export,
       convention    => c,
       external_name => "cim_ic_reset";

  function ic_filter_event
    (ic    : CimIcHandle;
     event : CimEvent_Access)
  return Interfaces.C.C_bool
  with export,
       convention    => c,
       external_name => "cim_ic_filter_event";

  procedure ic_set_cursor_pos
    (ic   : CimIcHandle;
     area : CimRect_Access)
  with export,
       convention    => c,
       external_name => "cim_ic_set_cursor_pos";

  procedure ic_set_callbacks
    (ic        : CimIcHandle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address)
  with export,
       convention    => c,
       external_name => "cim_ic_set_callbacks";

  function ic_get_preedit
    (ic : CimIcHandle)
  return CimPreedit_Access
  with export,
       convention    => c,
       external_name => "cim_ic_get_preedit";

  function ic_get_candidate
    (ic : CimIcHandle)
  return CimCandidate_Access
  with export,
       convention    => c,
       external_name => "cim_ic_get_candidate";

  procedure ic_activate_candidate_item
    (ic  : CimIcHandle;
     row : Interfaces.C.unsigned;
     col : Interfaces.C.unsigned)
  with export,
       convention    => c,
       external_name => "cim_ic_activate_candidate_item";

  procedure ic_change_candidate_page
    (ic         : CimIcHandle;
     page_index : Interfaces.C.unsigned)
  with export,
       convention    => c,
       external_name => "cim_ic_change_candidate_page";

  ---------------------------------------------------------------------------
  -- Utilities & Errors
  ---------------------------------------------------------------------------

  function dup_plugin_path
  return Interfaces.C.Strings.chars_ptr
  with export,
       convention    => c,
       external_name => "cim_dup_plugin_path";

  function get_last_error
  return Error
  with export,
       convention    => c,
       external_name => "cim_get_last_error";

  function strerror
    (err : Interfaces.C.unsigned)
  return Interfaces.C.Strings.chars_ptr
  with export,
       convention    => c,
       external_name => "cim_strerror";

end Cim.C;
