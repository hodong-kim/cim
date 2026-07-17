-- ============================================================================
-- cim_runtime_tests.adb
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Clair.Test.Assertions;
with Cim;
with Cim.Runtime;

package body Cim_Runtime_Tests is

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
  begin
    Cim.Runtime.set_error (Cim.CIM_ERROR_DLSYM_FAILED);

    assert_error_equal
      (reporter => reporter,
       actual   => Cim.Runtime.get_error,
       expected => Cim.CIM_ERROR_DLSYM_FAILED,
       message  => "runtime error state did not preserve the assigned value");

    assert_error_equal
      (reporter => reporter,
       actual   => Cim.get_last_error,
       expected => Cim.CIM_ERROR_DLSYM_FAILED,
       message  => "public error accessor disagrees with runtime state");

    Cim.Runtime.set_error (Cim.CIM_ERROR_NONE);
  exception
    when others =>
      Cim.Runtime.set_error (Cim.CIM_ERROR_NONE);
      raise;
  end error_state_round_trips;

  procedure error_state_is_task_local
    (reporter : in out Clair.Test.Reporter.Context)
  is
    task Worker is
      entry read_error (error : out Cim.Error);
    end Worker;

    task body Worker is
    begin
      Cim.Runtime.set_error (Cim.CIM_ERROR_DLOPEN_FAILED);

      accept read_error (error : out Cim.Error) do
        error := Cim.Runtime.get_error;
      end read_error;
    end Worker;

    worker_error : Cim.Error := Cim.CIM_ERROR_NONE;
  begin
    Cim.Runtime.set_error (Cim.CIM_ERROR_INVALID_ARGUMENT);
    Worker.read_error (worker_error);

    assert_error_equal
      (reporter => reporter,
       actual   => worker_error,
       expected => Cim.CIM_ERROR_DLOPEN_FAILED,
       message  => "worker task did not retain its own error state");

    assert_error_equal
      (reporter => reporter,
       actual   => Cim.Runtime.get_error,
       expected => Cim.CIM_ERROR_INVALID_ARGUMENT,
       message  => "worker task overwrote the caller error state");

    assert_error_equal
      (reporter => reporter,
       actual   => Cim.get_last_error,
       expected => Cim.CIM_ERROR_INVALID_ARGUMENT,
       message  => "public accessor lost the caller task error state");

    Cim.Runtime.set_error (Cim.CIM_ERROR_NONE);
  exception
    when others =>
      Cim.Runtime.set_error (Cim.CIM_ERROR_NONE);
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
