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

  function create return Handle;

  -- self must identify a live context and must not be NULL_HANDLE.
  procedure destroy (self : in out Handle);

  -- self must identify a live context and must not be NULL_HANDLE.
  procedure focus_in (self : Handle);

  -- self must identify a live context and must not be NULL_HANDLE.
  procedure focus_out (self : Handle);

  -- self must identify a live context and must not be NULL_HANDLE.
  procedure reset (self : Handle);

  -- self must identify a live context and must not be NULL_HANDLE.
  -- event must point to a valid event and must not be null.
  function filter_event
    (self  : Handle;
     event : CimEvent_Access)
  return Boolean;

  -- self must identify a live context and must not be NULL_HANDLE.
  -- area must point to a valid rectangle and must not be null.
  procedure set_cursor_pos
    (self : Handle;
     area : CimRect_Access);

  -- self must identify a live context and must not be NULL_HANDLE.
  -- callbacks must point to a valid callback table and must not be null.
  -- user_data may be System.NULL_ADDRESS.
  procedure set_callbacks
    (self      : Handle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address);

  -- self must identify a live context and must not be NULL_HANDLE.
  function get_preedit (self : Handle) return CimPreedit_Access;

  -- self must identify a live context and must not be NULL_HANDLE.
  function get_candidate (self : Handle) return CimCandidate_Access;

  -- self must identify a live context and must not be NULL_HANDLE.
  procedure activate_candidate_item
    (self : Handle;
     row  : Interfaces.C.unsigned;
     col  : Interfaces.C.unsigned);

  -- self must identify a live context and must not be NULL_HANDLE.
  procedure change_candidate_page
    (self       : Handle;
     page_index : Interfaces.C.unsigned);

end Cim.IC;
