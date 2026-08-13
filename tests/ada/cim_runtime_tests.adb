-- ============================================================================
-- cim_runtime_tests.adb
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Ada.Environment_Variables;
with Ada.Strings.Unbounded;
with Clair.Test.Assertions;
with Cim;
with Cim.IC;

package body Cim_Runtime_Tests is

  use type Cim.IC.Handle;

  CIM_PLUGIN_ENV      : constant String := "CIM_PLUGIN";
  TEST_PLUGIN_DIR_ENV : constant String := "CIM_TEST_PLUGIN_DIR";

  type Plugin_Environment_State is record
    exists : Boolean := False;
    value  : Ada.Strings.Unbounded.Unbounded_String;
  end record;

  function capture_plugin_environment return Plugin_Environment_State is
    state : Plugin_Environment_State;
  begin
    state.exists := Ada.Environment_Variables.Exists (CIM_PLUGIN_ENV);

    if state.exists then
      state.value := Ada.Strings.Unbounded.To_Unbounded_String
        (Ada.Environment_Variables.Value (CIM_PLUGIN_ENV));
    end if;

    return state;
  end capture_plugin_environment;

  procedure restore_plugin_environment
    (state : Plugin_Environment_State)
  is
  begin
    if state.exists then
      Ada.Environment_Variables.Set
        (CIM_PLUGIN_ENV,
         Ada.Strings.Unbounded.To_String (state.value));
    else
      Ada.Environment_Variables.Clear (CIM_PLUGIN_ENV);
    end if;
  end restore_plugin_environment;

  procedure set_test_plugin (name : String) is
  begin
    if not Ada.Environment_Variables.Exists (TEST_PLUGIN_DIR_ENV) then
      raise Program_Error with "CIM_TEST_PLUGIN_DIR is not configured";
    end if;

    declare
      directory : constant String :=
        Ada.Environment_Variables.Value (TEST_PLUGIN_DIR_ENV);
    begin
      if directory'length = 0 then
        raise Program_Error with "CIM_TEST_PLUGIN_DIR is empty";
      end if;

      if directory(directory'last) = '/' then
        Ada.Environment_Variables.Set
          (CIM_PLUGIN_ENV, directory & name);
      else
        Ada.Environment_Variables.Set
          (CIM_PLUGIN_ENV, directory & "/" & name);
      end if;
    end;
  end set_test_plugin;

  procedure create_expected_failure
    (plugin_name : String;
     operation   : String)
  is
    ic : Cim.IC.Handle := Cim.IC.NULL_HANDLE;
  begin
    set_test_plugin (plugin_name);
    ic := Cim.IC.create;

    if ic /= Cim.IC.NULL_HANDLE then
      Cim.IC.destroy (ic);
      raise Program_Error with operation & " unexpectedly succeeded";
    end if;
  end create_expected_failure;

  procedure assert_error_equal
    (reporter : in out Clair.Test.Reporter.Context;
     actual   : Cim.Error;
     expected : Cim.Error;
     message  : String)
  is
  begin
    Clair.Test.Assertions.assert_equal_integer
      (reporter => reporter,
       actual   => Integer(Cim.Error'enum_rep (actual)),
       expected => Integer(Cim.Error'enum_rep (expected)),
       message  => message);
  end assert_error_equal;

  procedure error_state_round_trips
    (reporter : in out Clair.Test.Reporter.Context)
  is
    environment : constant Plugin_Environment_State :=
      capture_plugin_environment;
  begin
    create_expected_failure
      (plugin_name => "libim-no-symbol.so",
       operation   => "Cim.IC.create");

    assert_error_equal
      (reporter => reporter,
       actual   => Cim.get_last_error,
       expected => Cim.CIM_ERROR_DLSYM_FAILED,
       message  => "Ada error accessor lost the operation failure");

    restore_plugin_environment (environment);
  exception
    when others =>
      restore_plugin_environment (environment);
      raise;
  end error_state_round_trips;

  procedure error_state_is_task_local
    (reporter : in out Clair.Test.Reporter.Context)
  is
    task Worker is
      entry run (error : out Cim.Error);
    end Worker;

    task body Worker is
      ic : Cim.IC.Handle := Cim.IC.NULL_HANDLE;
    begin
      accept run (error : out Cim.Error) do
        ic := Cim.IC.create;

        if ic /= Cim.IC.NULL_HANDLE then
          Cim.IC.destroy (ic);
          raise Program_Error with
            "worker Cim.IC.create unexpectedly succeeded";
        end if;

        error := Cim.get_last_error;
      end run;
    end Worker;

    environment  : constant Plugin_Environment_State :=
      capture_plugin_environment;
    worker_error : Cim.Error := Cim.CIM_ERROR_NONE;
  begin
    create_expected_failure
      (plugin_name => "libim-bad-version.so",
       operation   => "main Cim.IC.create");

    set_test_plugin ("libim-does-not-exist.so");
    Worker.run (worker_error);

    assert_error_equal
      (reporter => reporter,
       actual   => worker_error,
       expected => Cim.CIM_ERROR_DLOPEN_FAILED,
       message  => "worker task did not retain its own error state");

    assert_error_equal
      (reporter => reporter,
       actual   => Cim.get_last_error,
       expected => Cim.CIM_ERROR_BAD_ABI,
       message  => "worker task overwrote the caller Ada error state");

    restore_plugin_environment (environment);
  exception
    when others =>
      restore_plugin_environment (environment);
      raise;
  end error_state_is_task_local;

  procedure run_error_state_suite
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_scenario
      (reporter => reporter,
       name     => "error state round-trips through public accessors",
       runner   => error_state_round_trips'access);
  end run_error_state_suite;

  procedure run_task_local_suite
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_scenario
      (reporter => reporter,
       name     => "error state is isolated between Ada tasks",
       runner   => error_state_is_task_local'access);
  end run_task_local_suite;

  procedure run
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_suite
      (reporter => reporter,
       name     => "Cim runtime error state",
       runner   => run_error_state_suite'access);

    Clair.Test.Reporter.run_suite
      (reporter => reporter,
       name     => "Cim runtime task isolation",
       runner   => run_task_local_suite'access);
  end run;

end Cim_Runtime_Tests;
