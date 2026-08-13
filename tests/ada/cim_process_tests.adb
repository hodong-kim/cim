-- ============================================================================
-- cim_process_tests.adb
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Ada.Environment_Variables;
with Ada.Real_Time;

with Clair.Process.Execution;
with Clair.Status;

package body Cim_Process_Tests is

  use type Clair.Status.Code;

  NESTED_HOST_EXECUTABLE_KEY   : constant String
                               := "CIM_TEST_NESTED_HOST_EXECUTABLE";
  GTK_PREEDIT_EXECUTABLE_KEY   : constant String
                               := "CIM_TEST_GTK_PREEDIT_EXECUTABLE";
  GTK_CANDIDATE_EXECUTABLE_KEY : constant String
                               := "CIM_TEST_GTK_CANDIDATE_EXECUTABLE";
  GTK_CALLBACK_EXECUTABLE_KEY  : constant String
                               := "CIM_TEST_GTK_CALLBACK_EXECUTABLE";
  QT6_CALLBACK_EXECUTABLE_KEY  : constant String
                               := "CIM_TEST_QT6_CALLBACK_EXECUTABLE";
  PROCESS_CAPTURE_BYTES : constant Natural := 1_024 * 1_024;
  PROCESS_TIMEOUT : constant Ada.Real_Time.Time_Span
                  := Ada.Real_Time.Seconds (120);

  function required_environment_value (name : String) return String is
  begin
    if not Ada.Environment_Variables.exists (name) then
      raise Program_Error with name & " is not configured";
    end if;

    declare
      environment_value : constant String
                        := Ada.Environment_Variables.value (name);
    begin
      if environment_value'length = 0 then
        raise Program_Error with name & " is empty";
      end if;

      return environment_value;
    end;
  end required_environment_value;

  procedure require_ok
    (retval    : Clair.Status.Code;
     operation : String)
  is
  begin
    if retval /= Clair.Status.OK then
      raise Program_Error with
        operation & " failed with " & Clair.Status.Code'image (retval);
    end if;
  end require_ok;

  procedure configure_command
    (command        : in out Clair.Process.Execution.Command;
     executable_key : String)
  is
    executable : constant String
               := required_environment_value (executable_key);
    plugin_directory : constant String
                     := required_environment_value ("CIM_TEST_PLUGIN_DIR");
    limits : Clair.Process.Execution.Resource_Limits
           := Clair.Process.Execution.default_resource_limits;
    policy : constant Clair.Process.Execution.Timeout_Policy
           := (mode            => Clair.Process.Execution.Timeout_Enabled,
               interval        => PROCESS_TIMEOUT,
               graceful_period => Ada.Real_Time.Seconds (1),
               scope           => Clair.Process.Execution.Process_Tree);
    retval : Clair.Status.Code;
  begin
    limits.stdout_capture_bytes := PROCESS_CAPTURE_BYTES;
    limits.stderr_capture_bytes := PROCESS_CAPTURE_BYTES;

    retval := Clair.Process.Execution.set_resource_limits (command, limits);
    require_ok (retval, "set process case resource limits");

    retval := Clair.Process.Execution.set_executable (command, executable);
    require_ok (retval, "set process case executable");

    retval := Clair.Process.Execution.set_environment_value
      (command, "CIM_TEST_PLUGIN_DIR", plugin_directory);
    require_ok (retval, "set process case plugin directory");

    retval := Clair.Process.Execution.set_timeout_policy (command, policy);
    require_ok (retval, "set process case timeout");
  end configure_command;

  procedure run_process_smoke_suite
    (reporter : in out Clair.Test.Reporter.Context)
  is
    command : Clair.Process.Execution.Command
            := Clair.Process.Execution.empty_command;
  begin
    configure_command (command, "CIM_TEST_CIM_EXECUTABLE");
    Clair.Test.Reporter.run_process_case
      (reporter, "test-cim", command);

    Clair.Process.Execution.reset (command);
    configure_command (command, "CIM_TEST_LIBHANGUL_EXECUTABLE");
    Clair.Test.Reporter.run_process_case
      (reporter, "test-im-libhangul", command);

    if Ada.Environment_Variables.exists (GTK_PREEDIT_EXECUTABLE_KEY) then
      Clair.Process.Execution.reset (command);
      configure_command (command, GTK_PREEDIT_EXECUTABLE_KEY);
      Clair.Test.Reporter.run_process_case
        (reporter, "test-gtk-preedit", command);
    end if;

    if Ada.Environment_Variables.exists (GTK_CANDIDATE_EXECUTABLE_KEY) then
      Clair.Process.Execution.reset (command);
      configure_command (command, GTK_CANDIDATE_EXECUTABLE_KEY);
      Clair.Test.Reporter.run_process_case
        (reporter, "test-gtk-candidate", command);
    end if;

    if Ada.Environment_Variables.exists (GTK_CALLBACK_EXECUTABLE_KEY) then
      Clair.Process.Execution.reset (command);
      configure_command (command, GTK_CALLBACK_EXECUTABLE_KEY);
      Clair.Test.Reporter.run_process_case
        (reporter, "test-gtk-callback-delivery", command);
    end if;

    if Ada.Environment_Variables.exists (QT6_CALLBACK_EXECUTABLE_KEY) then
      Clair.Process.Execution.reset (command);
      configure_command (command, QT6_CALLBACK_EXECUTABLE_KEY);
      Clair.Test.Reporter.run_process_case
        (reporter, "test-qt6-callback-delivery", command);
    end if;

    if Ada.Environment_Variables.exists (NESTED_HOST_EXECUTABLE_KEY) then
      Clair.Process.Execution.reset (command);
      configure_command (command, NESTED_HOST_EXECUTABLE_KEY);
      Clair.Test.Reporter.run_process_case
        (reporter, "test-nested-host", command);
    end if;
  end run_process_smoke_suite;

  procedure run
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_suite
      (reporter => reporter,
       name     => "Cim process smoke tests",
       runner   => run_process_smoke_suite'access);
  end run;

end Cim_Process_Tests;
