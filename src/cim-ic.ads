-- ============================================================================
-- cim-ic.ads
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Interfaces.C;
with System;

package Cim.IC is

  subtype Handle is CimIcHandle;

  NULL_HANDLE : constant Handle := NULL_IC;

  --! contract: Calls using the same live handle must not overlap, including
  --! reentrant calls. A handle may be used from different tasks or threads
  --! when the caller serializes those calls. Different handles may be used
  --! concurrently. A handle becomes invalid when destroy begins and must not
  --! be used again.

  function create return Handle;
  procedure destroy (self : in out Handle);

  procedure focus_in (self : Handle);
  procedure focus_out (self : Handle);
  procedure reset (self : Handle);

  -- event must point to a valid event and must not be null.
  function filter_event
    (self  : Handle;
     event : CimEvent_Access)
  return Boolean;

  -- area must point to a valid rectangle and must not be null.
  procedure set_cursor_pos
    (self : Handle;
     area : CimRect_Access);

  -- callbacks must point to a valid callback table and must not be null.
  -- user_data may be System.NULL_ADDRESS.
  procedure set_callbacks
    (self      : Handle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address);

  function get_preedit (self : Handle) return CimPreedit_Access;
  function get_candidate (self : Handle) return CimCandidate_Access;

  procedure activate_candidate_item
    (self : Handle;
     row  : Interfaces.C.unsigned;
     col  : Interfaces.C.unsigned);

  procedure change_candidate_page
    (self       : Handle;
     page_index : Interfaces.C.unsigned);

end Cim.IC;
