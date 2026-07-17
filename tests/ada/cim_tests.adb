-- ============================================================================
-- cim_tests.adb
-- Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
-- SPDX-License-Identifier: 0BSD
-- ============================================================================
with Clair.Test.Assertions;
with Cim;

package body Cim_Tests is

  procedure assert_error_representation
    (reporter : in out Clair.Test.Reporter.Context;
     error    : Cim.Error;
     expected : Integer;
     name     : String)
  is
  begin
    Clair.Test.Assertions.assert_equal_integer
      (reporter => reporter,
       actual   => Integer(Cim.Error'enum_rep (error)),
       expected => expected,
       message  => name & " ABI representation changed");
  end assert_error_representation;

  procedure assert_error_message
    (reporter : in out Clair.Test.Reporter.Context;
     error    : Cim.Error;
     expected : String;
     name     : String)
  is
  begin
    Clair.Test.Assertions.assert_equal_string
      (reporter => reporter,
       actual   => Cim.strerror (error),
       expected => expected,
       message  => name & " error message changed");
  end assert_error_message;

  procedure version_constants_match_api
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Assertions.assert_equal_natural
      (reporter => reporter,
       actual   => Natural(Cim.CIM_MAJOR_VERSION),
       expected => 2,
       message  => "CIM major version changed");

    Clair.Test.Assertions.assert_equal_natural
      (reporter => reporter,
       actual   => Natural(Cim.CIM_MINOR_VERSION),
       expected => 1,
       message  => "CIM minor version changed");

    Clair.Test.Assertions.assert_equal_natural
      (reporter => reporter,
       actual   => Natural(Cim.CIM_MICRO_VERSION),
       expected => 0,
       message  => "CIM micro version changed");
  end version_constants_match_api;

  procedure error_representations_match_abi
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    assert_error_representation
      (reporter, Cim.CIM_ERROR_NONE, 0, "CIM_ERROR_NONE");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_INVALID_ARGUMENT, 1,
       "CIM_ERROR_INVALID_ARGUMENT");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_HOME_NOT_SET, 2,
       "CIM_ERROR_HOME_NOT_SET");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_PLUGIN_PATH_TOO_LONG, 3,
       "CIM_ERROR_PLUGIN_PATH_TOO_LONG");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_MUTEX_FAILED, 4,
       "CIM_ERROR_MUTEX_FAILED");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_DLOPEN_FAILED, 5,
       "CIM_ERROR_DLOPEN_FAILED");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_DLSYM_FAILED, 6,
       "CIM_ERROR_DLSYM_FAILED");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_NULL_PLUGIN, 7,
       "CIM_ERROR_NULL_PLUGIN");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_BAD_ABI, 8,
       "CIM_ERROR_BAD_ABI");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_INVALID_PLUGIN, 9,
       "CIM_ERROR_INVALID_PLUGIN");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_INIT_FAILED, 10,
       "CIM_ERROR_INIT_FAILED");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_CREATE_FAILED, 11,
       "CIM_ERROR_CREATE_FAILED");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_ALLOCATION_FAILED, 12,
       "CIM_ERROR_ALLOCATION_FAILED");
    assert_error_representation
      (reporter, Cim.CIM_ERROR_DLCLOSE_FAILED, 13,
       "CIM_ERROR_DLCLOSE_FAILED");
  end error_representations_match_abi;

  procedure error_messages_match_api
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    assert_error_message
      (reporter, Cim.CIM_ERROR_NONE, "no error", "CIM_ERROR_NONE");
    assert_error_message
      (reporter, Cim.CIM_ERROR_INVALID_ARGUMENT, "invalid argument",
       "CIM_ERROR_INVALID_ARGUMENT");
    assert_error_message
      (reporter, Cim.CIM_ERROR_HOME_NOT_SET, "HOME is not set",
       "CIM_ERROR_HOME_NOT_SET");
    assert_error_message
      (reporter, Cim.CIM_ERROR_PLUGIN_PATH_TOO_LONG,
       "plugin link path is too long", "CIM_ERROR_PLUGIN_PATH_TOO_LONG");
    assert_error_message
      (reporter, Cim.CIM_ERROR_MUTEX_FAILED, "mutex operation failed",
       "CIM_ERROR_MUTEX_FAILED");
    assert_error_message
      (reporter, Cim.CIM_ERROR_DLOPEN_FAILED, "failed to load plugin",
       "CIM_ERROR_DLOPEN_FAILED");
    assert_error_message
      (reporter, Cim.CIM_ERROR_DLSYM_FAILED,
       "failed to resolve cim_plugin", "CIM_ERROR_DLSYM_FAILED");
    assert_error_message
      (reporter, Cim.CIM_ERROR_NULL_PLUGIN, "plugin descriptor is NULL",
       "CIM_ERROR_NULL_PLUGIN");
    assert_error_message
      (reporter, Cim.CIM_ERROR_BAD_ABI, "incompatible plugin ABI",
       "CIM_ERROR_BAD_ABI");
    assert_error_message
      (reporter, Cim.CIM_ERROR_INVALID_PLUGIN, "invalid plugin",
       "CIM_ERROR_INVALID_PLUGIN");
    assert_error_message
      (reporter, Cim.CIM_ERROR_INIT_FAILED,
       "plugin initialization failed", "CIM_ERROR_INIT_FAILED");
    assert_error_message
      (reporter, Cim.CIM_ERROR_CREATE_FAILED, "plugin create failed",
       "CIM_ERROR_CREATE_FAILED");
    assert_error_message
      (reporter, Cim.CIM_ERROR_ALLOCATION_FAILED,
       "memory allocation failed", "CIM_ERROR_ALLOCATION_FAILED");
    assert_error_message
      (reporter, Cim.CIM_ERROR_DLCLOSE_FAILED,
       "failed to unload plugin", "CIM_ERROR_DLCLOSE_FAILED");
  end error_messages_match_api;

  procedure run_version_suite
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_scenario
      (reporter => reporter,
       name     => "version constants match the public API",
       runner   => version_constants_match_api'access);
  end run_version_suite;

  procedure run_error_abi_suite
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_scenario
      (reporter => reporter,
       name     => "error representations match the public ABI",
       runner   => error_representations_match_abi'access);
  end run_error_abi_suite;

  procedure run_error_message_suite
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_scenario
      (reporter => reporter,
       name     => "error messages match the public API",
       runner   => error_messages_match_api'access);
  end run_error_message_suite;

  procedure run
    (reporter : in out Clair.Test.Reporter.Context)
  is
  begin
    Clair.Test.Reporter.run_suite
      (reporter => reporter,
       name     => "Cim version",
       runner   => run_version_suite'access);

    Clair.Test.Reporter.run_suite
      (reporter => reporter,
       name     => "Cim error ABI",
       runner   => run_error_abi_suite'access);

    Clair.Test.Reporter.run_suite
      (reporter => reporter,
       name     => "Cim error messages",
       runner   => run_error_message_suite'access);
  end run;

end Cim_Tests;
