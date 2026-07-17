-- ============================================================================
-- cim-ic.adb
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Cim.Runtime;

package body Cim.IC is

  function create return Handle is
  begin
    return Cim.Runtime.ic_create;
  end create;

  procedure destroy (self : in out Handle) is
  begin
    Cim.Runtime.ic_destroy (self);
    self := NULL_HANDLE;
  end destroy;

  procedure focus_in (self : Handle) is
  begin
    Cim.Runtime.ic_focus_in (self);
  end focus_in;

  procedure focus_out (self : Handle) is
  begin
    Cim.Runtime.ic_focus_out (self);
  end focus_out;

  procedure reset (self : Handle) is
  begin
    Cim.Runtime.ic_reset (self);
  end reset;

  function filter_event
    (self  : Handle;
     event : CimEvent_Access)
  return Boolean is
  begin
    return Boolean(Cim.Runtime.ic_filter_event (self, event));
  end filter_event;

  procedure set_cursor_pos
    (self : Handle;
     area : CimRect_Access) is
  begin
    Cim.Runtime.ic_set_cursor_pos (self, area);
  end set_cursor_pos;

  procedure set_callbacks
    (self      : Handle;
     callbacks : CimCallbacks_Access;
     user_data : System.Address) is
  begin
    Cim.Runtime.ic_set_callbacks (self, callbacks, user_data);
  end set_callbacks;

  function get_preedit (self : Handle) return CimPreedit_Access is
  begin
    return Cim.Runtime.ic_get_preedit (self);
  end get_preedit;

  function get_candidate (self : Handle) return CimCandidate_Access is
  begin
    return Cim.Runtime.ic_get_candidate (self);
  end get_candidate;

  procedure activate_candidate_item
    (self : Handle;
     row  : Interfaces.C.unsigned;
     col  : Interfaces.C.unsigned) is
  begin
    Cim.Runtime.ic_activate_candidate_item (self, row, col);
  end activate_candidate_item;

  procedure change_candidate_page
    (self       : Handle;
     page_index : Interfaces.C.unsigned) is
  begin
    Cim.Runtime.ic_change_candidate_page (self, page_index);
  end change_candidate_page;

end Cim.IC;
