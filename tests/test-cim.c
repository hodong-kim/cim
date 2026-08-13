/******************************************************************************
 * test-cim.c
 * Copyright (C) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 ******************************************************************************/
#include "cim.h"
#include "test-common.h"
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>

#ifdef CIM_HAS_INTERNAL_TEST_HOOK
extern void cim_unhandled_exception_c (const char *operation);
#endif

#define UNHANDLED_EXCEPTION_MODE          "--unhandled-exception"
#define STALE_DESTROY_MODE                "--stale-destroy"
#ifdef CIM_HAS_PLUGIN_REENTRY_TEST
#define PLUGIN_INIT_REENTRY_MODE           "--plugin-init-reentry"
#define PLUGIN_FINI_REENTRY_MODE           "--plugin-fini-reentry"
#endif
#define STRERROR_INVALID_MODE              "--strerror-invalid"
#define CONTRACT_NULL_MODE                "--contract-null"
#define CONTRACT_DESTROY                  "destroy"
#define CONTRACT_FOCUS_IN                 "focus-in"
#define CONTRACT_FOCUS_OUT                "focus-out"
#define CONTRACT_RESET                    "reset"
#define CONTRACT_FILTER_EVENT_IC          "filter-event-ic"
#define CONTRACT_FILTER_EVENT_EVENT       "filter-event-event"
#define CONTRACT_SET_CURSOR_POS_IC        "set-cursor-pos-ic"
#define CONTRACT_SET_CURSOR_POS_AREA      "set-cursor-pos-area"
#define CONTRACT_SET_CALLBACKS_IC         "set-callbacks-ic"
#define CONTRACT_SET_CALLBACKS_CALLBACKS  "set-callbacks-callbacks"
#define CONTRACT_GET_PREEDIT              "get-preedit"
#define CONTRACT_GET_CANDIDATE            "get-candidate"
#define CONTRACT_ACTIVATE_CANDIDATE_ITEM  "activate-candidate-item"
#define CONTRACT_CHANGE_CANDIDATE_PAGE    "change-candidate-page"

_Static_assert (CIM_ERROR_DLCLOSE_FAILED == 13,
                "CimError ABI value changed");

static void
test_preedit_start (CimIcHandle ic, void *user_data)
{
  (void) ic;
  (void) user_data;
}

static void
test_preedit_end (CimIcHandle ic, void *user_data)
{
  (void) ic;
  (void) user_data;
}

static void
test_preedit_changed (CimIcHandle ic,
                      const CimPreedit *preedit,
                      void *user_data)
{
  (void) ic;
  (void) preedit;
  (void) user_data;
}

static void
test_commit (CimIcHandle ic, const char *text, void *user_data)
{
  (void) ic;
  (void) text;
  (void) user_data;
}

static const CimSurround *
test_get_surround (CimIcHandle ic, void *user_data)
{
  (void) ic;
  (void) user_data;
  return NULL;
}

static bool
test_delete_surround (CimIcHandle ic,
                      int32_t offset,
                      uint32_t n_chars,
                      void *user_data)
{
  (void) ic;
  (void) offset;
  (void) n_chars;
  (void) user_data;
  return false;
}

static void
test_candidate_show (CimIcHandle ic,
                     uint32_t n_rows,
                     uint32_t n_cols,
                     bool show_aux,
                     void *user_data)
{
  (void) ic;
  (void) n_rows;
  (void) n_cols;
  (void) show_aux;
  (void) user_data;
}

static void
test_candidate_hide (CimIcHandle ic, void *user_data)
{
  (void) ic;
  (void) user_data;
}

static void
test_candidate_changed (CimIcHandle ic,
                        const CimCandidate *candidate,
                        void *user_data)
{
  (void) ic;
  (void) candidate;
  (void) user_data;
}

static void
test_candidate_selected (CimIcHandle ic,
                         const CimSelection *selection,
                         void *user_data)
{
  (void) ic;
  (void) selection;
  (void) user_data;
}

static void
test_notify (CimIcHandle ic,
             CimNotificationType type,
             void *user_data)
{
  (void) ic;
  (void) type;
  (void) user_data;
}

static int
test_get_info_smoke (const char *name,
                     const char *plugin_path,
                     const char *expected_name,
                     const char *expected_desc)
{
  void *handle;
  CimPlugin *plugin;
  const CimInfo *info;

  if (test_set_plugin_env (plugin_path) != 0)
    return 1;

  handle = dlopen (getenv ("CIM_PLUGIN"), RTLD_LAZY | RTLD_LOCAL);
  if (handle == NULL)
  {
    fprintf (stderr, "%s: dlopen() failed: %s\n", name, dlerror ());
    return 1;
  }

  dlerror ();
  plugin = (CimPlugin *) dlsym (handle, "cim_plugin");
  if (plugin == NULL)
  {
    fprintf (stderr, "%s: dlsym(cim_plugin) failed: %s\n", name, dlerror ());
    dlclose (handle);
    return 1;
  }

  if (plugin->get_info == NULL)
  {
    fprintf (stderr, "%s: plugin->get_info is NULL\n", name);
    dlclose (handle);
    return 1;
  }

  info = plugin->get_info ();
  if (info == NULL)
  {
    fprintf (stderr, "%s: get_info() returned NULL\n", name);
    dlclose (handle);
    return 1;
  }

  if (info->name == NULL)
  {
    fprintf (stderr, "%s: info->name is NULL\n", name);
    dlclose (handle);
    return 1;
  }

  if (info->desc == NULL)
  {
    fprintf (stderr, "%s: info->desc is NULL\n", name);
    dlclose (handle);
    return 1;
  }

  if (expected_name != NULL && strcmp (info->name, expected_name) != 0)
  {
    fprintf (stderr,
             "%s: unexpected info->name: got \"%s\", expected \"%s\"\n",
             name, info->name, expected_name);
    dlclose (handle);
    return 1;
  }

  if (expected_desc != NULL && strcmp (info->desc, expected_desc) != 0)
  {
    fprintf (stderr,
             "%s: unexpected info->desc: got \"%s\", expected \"%s\"\n",
             name, info->desc, expected_desc);
    dlclose (handle);
    return 1;
  }

  if (dlclose (handle) != 0)
  {
    fprintf (stderr, "%s: dlclose() failed\n", name);
    return 1;
  }

  return 0;
}

static int
expect_error_value (const char *name,
                    CimError actual_error,
                    CimError expected_error)
{
  if (actual_error == expected_error)
    return 0;

  fprintf (stderr,
           "%s: unexpected error: got %d (%s), expected %d (%s)\n",
           name,
           (int) actual_error,
           cim_strerror (actual_error),
           (int) expected_error,
           cim_strerror (expected_error));
  return 1;
}

static int
test_automatic_runtime_initialization (void)
{
  return expect_error_value ("test_automatic_runtime_initialization",
                             cim_get_last_error (),
                             CIM_ERROR_NONE);
}

static int
expect_create_failure (const char *name,
                       const char *plugin,
                       CimError expected_error)
{
  CimIcHandle ic;

  if (test_set_plugin_env (plugin) != 0)
    return 1;

  ic = cim_ic_create ();
  if (ic != NULL)
  {
    fprintf (stderr, "%s: cim_ic_create() unexpectedly succeeded\n", name);
    cim_ic_destroy (ic);
    return 1;
  }

  return expect_error_value (name,
                             cim_get_last_error (),
                             expected_error);
}

static CimIcHandle
create_contract_test_context (void)
{
  CimIcHandle ic;

  if (test_set_plugin_env ("lib/im-dummy.so") != 0)
    return NULL;

  ic = cim_ic_create ();
  if (ic == NULL)
  {
    fprintf (stderr,
             "create_contract_test_context: cim_ic_create() failed\n");
  }

  return ic;
}

static int
disable_core_dumps (const char *context)
{
  const struct rlimit core_limit = { 0, 0 };

  if (setrlimit (RLIMIT_CORE, &core_limit) != 0)
  {
    perror (context);
    return 1;
  }

  return 0;
}

static int
run_strerror_invalid_mode (void)
{
  if (disable_core_dumps ("run_strerror_invalid_mode: setrlimit") != 0)
    return EXIT_FAILURE;

  (void) cim_strerror ((CimError) UINT32_MAX);
  return EXIT_FAILURE;
}

#ifdef CIM_HAS_INTERNAL_TEST_HOOK
static int
run_unhandled_exception_mode (void)
{
  if (disable_core_dumps ("run_unhandled_exception_mode: setrlimit") != 0)
    return EXIT_FAILURE;

  cim_unhandled_exception_c ("test-cim");
  return EXIT_FAILURE;
}
#endif

static int
run_stale_destroy_mode (void)
{
  CimIcHandle ic;

  if (disable_core_dumps ("run_stale_destroy_mode: setrlimit") != 0)
    return EXIT_FAILURE;

  if (test_set_plugin_env ("lib/im-noop-destroy.so") != 0)
    return EXIT_FAILURE;

  ic = cim_ic_create ();
  if (ic == NULL)
  {
    fprintf (stderr, "run_stale_destroy_mode: cim_ic_create() failed\n");
    return EXIT_FAILURE;
  }

  cim_ic_destroy (ic);

  /* The no-op destroy callback avoids freeing heap storage, making the stale
   * pointer value deterministic without dereferencing it.
   */
  cim_ic_destroy (ic);

  return EXIT_FAILURE;
}

#ifdef CIM_HAS_PLUGIN_REENTRY_TEST
static int
run_plugin_init_reentry_mode (void)
{
  if (disable_core_dumps
        ("run_plugin_init_reentry_mode: setrlimit") != 0)
    return EXIT_FAILURE;

  if (test_set_plugin_env ("lib/im-reentrant-init.so") != 0)
    return EXIT_FAILURE;

  (void) cim_ic_create ();
  return EXIT_FAILURE;
}

static int
run_plugin_fini_reentry_mode (void)
{
  CimIcHandle ic;

  if (disable_core_dumps
        ("run_plugin_fini_reentry_mode: setrlimit") != 0)
    return EXIT_FAILURE;

  if (test_set_plugin_env ("lib/im-reentrant-fini.so") != 0)
    return EXIT_FAILURE;

  ic = cim_ic_create ();
  if (ic == NULL)
  {
    fprintf (stderr,
             "run_plugin_fini_reentry_mode: cim_ic_create() failed\n");
    return EXIT_FAILURE;
  }

  cim_ic_destroy (ic);
  return EXIT_FAILURE;
}
#endif

static int
run_null_contract_mode (const char *operation)
{
  if (disable_core_dumps ("run_null_contract_mode: setrlimit") != 0)
    return EXIT_FAILURE;

  if (strcmp (operation, CONTRACT_DESTROY) == 0)
    cim_ic_destroy (NULL);
  else if (strcmp (operation, CONTRACT_FOCUS_IN) == 0)
    cim_ic_focus_in (NULL);
  else if (strcmp (operation, CONTRACT_FOCUS_OUT) == 0)
    cim_ic_focus_out (NULL);
  else if (strcmp (operation, CONTRACT_RESET) == 0)
    cim_ic_reset (NULL);
  else if (strcmp (operation, CONTRACT_FILTER_EVENT_IC) == 0)
  {
    CimEvent event = { 0 };

    (void) cim_ic_filter_event (NULL, &event);
  }
  else if (strcmp (operation, CONTRACT_FILTER_EVENT_EVENT) == 0)
  {
    CimIcHandle ic = create_contract_test_context ();

    if (ic == NULL)
      return EXIT_FAILURE;

    (void) cim_ic_filter_event (ic, NULL);

    /* Reached only when the expected contract violation did not occur. */
    cim_ic_destroy (ic);
  }
  else if (strcmp (operation, CONTRACT_SET_CURSOR_POS_IC) == 0)
  {
    CimRect area = { 0 };

    cim_ic_set_cursor_pos (NULL, &area);
  }
  else if (strcmp (operation, CONTRACT_SET_CURSOR_POS_AREA) == 0)
  {
    CimIcHandle ic = create_contract_test_context ();

    if (ic == NULL)
      return EXIT_FAILURE;

    cim_ic_set_cursor_pos (ic, NULL);

    /* Reached only when the expected contract violation did not occur. */
    cim_ic_destroy (ic);
  }
  else if (strcmp (operation, CONTRACT_SET_CALLBACKS_IC) == 0)
  {
    CimCallbacks callbacks;

    memset (&callbacks, 0, sizeof (callbacks));
    cim_ic_set_callbacks (NULL, &callbacks, NULL);
  }
  else if (strcmp (operation, CONTRACT_SET_CALLBACKS_CALLBACKS) == 0)
  {
    CimIcHandle ic = create_contract_test_context ();

    if (ic == NULL)
      return EXIT_FAILURE;

    cim_ic_set_callbacks (ic, NULL, NULL);

    /* Reached only when the expected contract violation did not occur. */
    cim_ic_destroy (ic);
  }
  else if (strcmp (operation, CONTRACT_GET_PREEDIT) == 0)
    (void) cim_ic_get_preedit (NULL);
  else if (strcmp (operation, CONTRACT_GET_CANDIDATE) == 0)
    (void) cim_ic_get_candidate (NULL);
  else if (strcmp (operation, CONTRACT_ACTIVATE_CANDIDATE_ITEM) == 0)
    cim_ic_activate_candidate_item (NULL, 0, 0);
  else if (strcmp (operation, CONTRACT_CHANGE_CANDIDATE_PAGE) == 0)
    cim_ic_change_candidate_page (NULL, 0);
  else
  {
    fprintf (stderr,
             "run_null_contract_mode: unknown operation: %s\n",
             operation);
    return EXIT_FAILURE;
  }

  return EXIT_FAILURE;
}

static int
expect_sigabrt_status (const char *name, int status)
{
  if (!WIFSIGNALED (status))
  {
    if (WIFEXITED (status))
      fprintf (stderr,
               "test_expected_abort(%s): "
               "expected SIGABRT, child exited with status %d\n",
               name,
               WEXITSTATUS (status));
    else
      fprintf (stderr,
               "test_expected_abort(%s): "
               "expected SIGABRT, unexpected wait status %d\n",
               name,
               status);

    return 1;
  }

  if (WTERMSIG (status) != SIGABRT)
  {
    fprintf (stderr,
             "test_expected_abort(%s): "
             "expected SIGABRT, got signal %d\n",
             name,
             WTERMSIG (status));
    return 1;
  }

  return 0;
}

static int
test_expected_abort (const char *executable_path,
                     const char *mode,
                     const char *operation)
{
  const char *name = operation != NULL ? operation : mode;
  pid_t pid;
  int status;

  pid = fork ();
  if (pid == -1)
  {
    perror ("test_expected_abort: fork");
    return 1;
  }

  if (pid == 0)
  {
    if (operation == NULL)
      execlp (executable_path,
              executable_path,
              mode,
              (char *) NULL);
    else
      execlp (executable_path,
              executable_path,
              mode,
              operation,
              (char *) NULL);

    _exit (127);
  }

  if (waitpid (pid, &status, 0) == -1)
  {
    perror ("test_expected_abort: waitpid");
    return 1;
  }

  return expect_sigabrt_status (name, status);
}

static int
read_child_stderr (int fd, char *output, size_t output_size)
{
  char buffer[256];
  size_t output_length = 0;

  for (;;)
  {
    ssize_t count = read (fd, buffer, sizeof (buffer));

    if (count > 0)
    {
      size_t available = output_size - output_length - 1;
      size_t copy_length = (size_t) count;

      if (copy_length > available)
        copy_length = available;

      if (copy_length > 0)
      {
        memcpy (output + output_length, buffer, copy_length);
        output_length += copy_length;
      }

      continue;
    }

    if (count == 0)
      break;

    if (errno == EINTR)
      continue;

    perror ("read_child_stderr: read");
    output[output_length] = '\0';
    return 1;
  }

  output[output_length] = '\0';
  return 0;
}

static int
test_expected_abort_message (const char *executable_path,
                             const char *mode,
                             const char *expected_message)
{
  char output[512];
  int stderr_pipe[2];
  pid_t pid;
  int read_failed;
  int status;

  if (pipe (stderr_pipe) != 0)
  {
    perror ("test_expected_abort_message: pipe");
    return 1;
  }

  pid = fork ();
  if (pid == -1)
  {
    perror ("test_expected_abort_message: fork");
    close (stderr_pipe[0]);
    close (stderr_pipe[1]);
    return 1;
  }

  if (pid == 0)
  {
    close (stderr_pipe[0]);

    if (dup2 (stderr_pipe[1], STDERR_FILENO) == -1)
      _exit (127);

    close (stderr_pipe[1]);
    execlp (executable_path,
            executable_path,
            mode,
            (char *) NULL);
    _exit (127);
  }

  close (stderr_pipe[1]);
  read_failed = read_child_stderr (stderr_pipe[0], output, sizeof (output));
  close (stderr_pipe[0]);

  if (waitpid (pid, &status, 0) == -1)
  {
    perror ("test_expected_abort_message: waitpid");
    return 1;
  }

  if (read_failed != 0)
    return 1;

  if (expect_sigabrt_status (mode, status) != 0)
    return 1;

  if (strstr (output, expected_message) == NULL)
  {
    fprintf (stderr,
             "test_expected_abort_message(%s): missing diagnostic: %s\n"
             "captured stderr: %s",
             mode,
             expected_message,
             output);
    return 1;
  }

  return 0;
}

static int
test_null_contract_violation (const char *executable_path,
                              const char *operation)
{
  return test_expected_abort (executable_path,
                              CONTRACT_NULL_MODE,
                              operation);
}

#ifdef CIM_HAS_INTERNAL_TEST_HOOK
static int
test_unhandled_exception_abort (const char *executable_path)
{
  return test_expected_abort (executable_path,
                              UNHANDLED_EXCEPTION_MODE,
                              NULL);
}
#endif

static int
test_stale_destroy_abort (const char *executable_path)
{
  return test_expected_abort_message
    (executable_path,
     STALE_DESTROY_MODE,
     "Cim contract violation: "
     "Cim input context operation requires a ready plugin");
}

#ifdef CIM_HAS_PLUGIN_REENTRY_TEST
static int
test_plugin_lifecycle_reentry_abort (const char *executable_path)
{
  static const char diagnostic[] =
    "Cim contract violation: "
    "Cim plugin lifecycle callback re-entered Cim";
  int retval = 0;

  retval |= test_expected_abort_message (executable_path,
                                         PLUGIN_INIT_REENTRY_MODE,
                                         diagnostic);
  retval |= test_expected_abort_message (executable_path,
                                         PLUGIN_FINI_REENTRY_MODE,
                                         diagnostic);
  return retval;
}
#endif

static int
test_strerror_invalid_error_abort (const char *executable_path)
{
  return test_expected_abort_message
    (executable_path,
     STRERROR_INVALID_MODE,
     "Cim contract violation: cim_strerror: err is out of range");
}

static int
test_dlclose_error_string (void)
{
  const char *message = cim_strerror (CIM_ERROR_DLCLOSE_FAILED);

  if (message == NULL || strcmp (message, "failed to unload plugin") != 0)
  {
    fprintf (stderr,
             "test_dlclose_error_string: unexpected error string\n");
    return 1;
  }

  return 0;
}

static int
test_dummy_single_create_destroy (void)
{
  CimIcHandle ic;
  CimEvent event = { 0 };
  CimRect rect = { .x = 10, .y = 20, .width = 30, .height = 40 };
  CimCallbacks callbacks;
  const CimPreedit *preedit;
  const CimCandidate *candidate;

  if (test_set_plugin_env ("lib/im-dummy.so") != 0)
    return 1;

  if (test_set_verbose_env (true) != 0)
    return 1;

  memset (&callbacks, 0, sizeof (callbacks));
  callbacks.preedit_start = test_preedit_start;
  callbacks.preedit_end = test_preedit_end;
  callbacks.preedit_changed = test_preedit_changed;
  callbacks.commit = test_commit;
  callbacks.get_surround = test_get_surround;
  callbacks.delete_surround = test_delete_surround;
  callbacks.candidate_show = test_candidate_show;
  callbacks.candidate_hide = test_candidate_hide;
  callbacks.candidate_changed = test_candidate_changed;
  callbacks.candidate_selected = test_candidate_selected;
  callbacks.notify = test_notify;

  ic = cim_ic_create ();
  if (ic == NULL)
  {
    fprintf (stderr, "test_dummy_single_create_destroy: cim_ic_create() failed\n");
    return 1;
  }

  cim_ic_focus_in (ic);
  cim_ic_reset (ic);
  cim_ic_set_cursor_pos (ic, &rect);
  cim_ic_set_callbacks (ic, &callbacks, NULL);

  preedit = cim_ic_get_preedit (ic);
  if (preedit == NULL)
  {
    fprintf (stderr, "test_dummy_single_create_destroy: cim_ic_get_preedit() failed\n");
    cim_ic_destroy (ic);
    return 1;
  }

  if (preedit->text == NULL)
  {
    fprintf (stderr, "test_dummy_single_create_destroy: preedit->text is NULL\n");
    cim_ic_destroy (ic);
    return 1;
  }

  candidate = cim_ic_get_candidate (ic);
  if (candidate == NULL)
  {
    fprintf (stderr, "test_dummy_single_create_destroy: cim_ic_get_candidate() failed\n");
    cim_ic_destroy (ic);
    return 1;
  }

  cim_ic_activate_candidate_item (ic, 0, 1);
  cim_ic_change_candidate_page (ic, 0);

  if (cim_ic_filter_event (ic, &event) != false)
  {
    fprintf (stderr, "test_dummy_single_create_destroy: cim_ic_filter_event() unexpected result\n");
    cim_ic_destroy (ic);
    return 1;
  }

  cim_ic_focus_out (ic);
  cim_ic_destroy (ic);
  return 0;
}

static int
test_dummy_multiple_create_destroy (void)
{
  CimIcHandle ic1;
  CimIcHandle ic2;
  CimIcHandle ic3;
  CimEvent event = { 0 };
  CimRect rect = { .x = 1, .y = 2, .width = 3, .height = 4 };

  if (test_set_plugin_env ("lib/im-dummy.so") != 0)
    return 1;

  if (test_set_verbose_env (true) != 0)
    return 1;

  ic1 = cim_ic_create ();
  if (ic1 == NULL)
  {
    fprintf (stderr, "test_dummy_multiple_create_destroy: cim_ic_create() #1 failed\n");
    return 1;
  }

  ic2 = cim_ic_create ();
  if (ic2 == NULL)
  {
    fprintf (stderr, "test_dummy_multiple_create_destroy: cim_ic_create() #2 failed\n");
    cim_ic_destroy (ic1);
    return 1;
  }

  ic3 = cim_ic_create ();
  if (ic3 == NULL)
  {
    fprintf (stderr, "test_dummy_multiple_create_destroy: cim_ic_create() #3 failed\n");
    cim_ic_destroy (ic2);
    cim_ic_destroy (ic1);
    return 1;
  }

  cim_ic_focus_in (ic1);
  cim_ic_focus_in (ic2);
  cim_ic_set_cursor_pos (ic2, &rect);

  if (cim_ic_filter_event (ic2, &event) != false)
  {
    fprintf (stderr, "test_dummy_multiple_create_destroy: cim_ic_filter_event() unexpected result\n");
    cim_ic_destroy (ic3);
    cim_ic_destroy (ic2);
    cim_ic_destroy (ic1);
    return 1;
  }

  cim_ic_activate_candidate_item (ic2, 0, 0);
  cim_ic_change_candidate_page (ic2, 0);
  cim_ic_reset (ic2);
  cim_ic_focus_out (ic2);
  cim_ic_focus_out (ic1);

  cim_ic_destroy (ic2);
  cim_ic_destroy (ic1);
  cim_ic_destroy (ic3);

  return 0;
}

static int
test_load_fail (void)
{
  return expect_create_failure ("test_load_fail",
                                "lib/im-does-not-exist.so",
                                CIM_ERROR_DLOPEN_FAILED);
}

static int
test_init_fail (void)
{
  return expect_create_failure ("test_init_fail",
                                "lib/im-init-fail.so",
                                CIM_ERROR_INIT_FAILED);
}

static int
test_create_fail (void)
{
  return expect_create_failure ("test_create_fail",
                                "lib/im-create-fail.so",
                                CIM_ERROR_CREATE_FAILED);
}

static int
test_bad_version (void)
{
  return expect_create_failure ("test_bad_version",
                                "lib/im-bad-version.so",
                                CIM_ERROR_BAD_ABI);
}

static int
test_no_symbol (void)
{
  return expect_create_failure ("test_no_symbol",
                                "lib/im-no-symbol.so",
                                CIM_ERROR_DLSYM_FAILED);
}

static int
test_no_create (void)
{
  return expect_create_failure ("test_no_create",
                                "lib/im-no-create.so",
                                CIM_ERROR_INVALID_PLUGIN);
}

static int
test_no_destroy (void)
{
  return expect_create_failure ("test_no_destroy",
                                "lib/im-no-destroy.so",
                                CIM_ERROR_INVALID_PLUGIN);
}

struct error_isolation_result
{
  CimError initial_error;
  CimError operation_error;
  bool create_succeeded;
};

static void *
error_isolation_worker (void *data)
{
  struct error_isolation_result *result =
    (struct error_isolation_result *) data;
  CimIcHandle ic;

  result->initial_error = cim_get_last_error ();
  ic = cim_ic_create ();
  result->operation_error = cim_get_last_error ();
  result->create_succeeded = ic != NULL;

  if (ic != NULL)
    cim_ic_destroy (ic);

  return NULL;
}

static int
test_foreign_thread_error_isolation (void)
{
  pthread_t thread;
  struct error_isolation_result result = {
    .initial_error = CIM_ERROR_DLCLOSE_FAILED,
    .operation_error = CIM_ERROR_DLCLOSE_FAILED,
    .create_succeeded = false
  };

  if (expect_create_failure ("test_foreign_thread_error_isolation(main)",
                             "lib/im-bad-version.so",
                             CIM_ERROR_BAD_ABI) != 0)
    return 1;

  if (test_set_plugin_env ("lib/im-does-not-exist.so") != 0)
    return 1;

  if (pthread_create (&thread, NULL, error_isolation_worker, &result) != 0)
  {
    fprintf (stderr,
             "test_foreign_thread_error_isolation: pthread_create() failed\n");
    return 1;
  }

  if (pthread_join (thread, NULL) != 0)
  {
    fprintf (stderr,
             "test_foreign_thread_error_isolation: pthread_join() failed\n");
    return 1;
  }

  if (expect_error_value ("test_foreign_thread_error_isolation"
                          "(worker initial)",
                          result.initial_error,
                          CIM_ERROR_NONE) != 0)
    return 1;

  if (result.create_succeeded)
  {
    fprintf (stderr,
             "test_foreign_thread_error_isolation: create succeeded\n");
    return 1;
  }

  if (expect_error_value ("test_foreign_thread_error_isolation(worker op)",
                          result.operation_error,
                          CIM_ERROR_DLOPEN_FAILED) != 0)
    return 1;

  return expect_error_value ("test_foreign_thread_error_isolation(main end)",
                             cim_get_last_error (),
                             CIM_ERROR_BAD_ABI);
}

static int
test_error_reset_after_success (void)
{
  CimIcHandle ic;
  int retval;

  if (expect_create_failure ("test_error_reset_after_success(failure)",
                             "lib/im-bad-version.so",
                             CIM_ERROR_BAD_ABI) != 0)
    return 1;

  if (test_set_plugin_env ("lib/im-dummy.so") != 0)
    return 1;

  ic = cim_ic_create ();
  if (ic == NULL)
  {
    fprintf (stderr,
             "test_error_reset_after_success: create failed: %s\n",
             cim_strerror (cim_get_last_error ()));
    return 1;
  }

  retval = expect_error_value ("test_error_reset_after_success(create)",
                               cim_get_last_error (),
                               CIM_ERROR_NONE);
  cim_ic_destroy (ic);

  if (retval != 0)
    return 1;

  return expect_error_value ("test_error_reset_after_success(destroy)",
                             cim_get_last_error (),
                             CIM_ERROR_NONE);
}

enum error_reset_operation
{
  ERROR_RESET_FOCUS_IN,
  ERROR_RESET_FOCUS_OUT,
  ERROR_RESET_RESET,
  ERROR_RESET_FILTER_EVENT,
  ERROR_RESET_SET_CURSOR_POS,
  ERROR_RESET_SET_CALLBACKS,
  ERROR_RESET_GET_PREEDIT,
  ERROR_RESET_GET_CANDIDATE,
  ERROR_RESET_ACTIVATE_CANDIDATE_ITEM,
  ERROR_RESET_CHANGE_CANDIDATE_PAGE
};

struct error_reset_case
{
  const char *name;
  enum error_reset_operation operation;
};

static int
restore_error_reset_environment (const char *home, bool had_home)
{
  int retval = 0;

  if (had_home)
  {
    if (setenv ("HOME", home, 1) != 0)
    {
      perror ("restore_error_reset_environment: setenv(HOME)");
      retval = 1;
    }
  }
  else if (unsetenv ("HOME") != 0)
  {
    perror ("restore_error_reset_environment: unsetenv(HOME)");
    retval = 1;
  }

  if (test_set_plugin_env ("lib/im-dummy.so") != 0)
    retval = 1;

  return retval;
}

static int
seed_home_not_set_error (const char *name,
                         const char *home,
                         bool had_home)
{
  char *path = NULL;
  int retval = 1;

  if (unsetenv ("CIM_PLUGIN") != 0)
  {
    perror ("seed_home_not_set_error: unsetenv(CIM_PLUGIN)");
    goto cleanup;
  }

  if (unsetenv ("HOME") != 0)
  {
    perror ("seed_home_not_set_error: unsetenv(HOME)");
    goto cleanup;
  }

  path = cim_dup_plugin_path ();
  if (path != NULL)
  {
    fprintf (stderr,
             "%s: cim_dup_plugin_path() unexpectedly succeeded\n",
             name);
    free (path);
    goto cleanup;
  }

  retval = expect_error_value (name,
                               cim_get_last_error (),
                               CIM_ERROR_HOME_NOT_SET);

cleanup:
  if (restore_error_reset_environment (home, had_home) != 0)
    retval = 1;

  return retval;
}

static void
run_error_reset_operation (enum error_reset_operation operation,
                           CimIcHandle ic,
                           CimCallbacks *callbacks,
                           CimEvent *event,
                           CimRect *area)
{
  switch (operation)
  {
    case ERROR_RESET_FOCUS_IN:
      cim_ic_focus_in (ic);
      break;
    case ERROR_RESET_FOCUS_OUT:
      cim_ic_focus_out (ic);
      break;
    case ERROR_RESET_RESET:
      cim_ic_reset (ic);
      break;
    case ERROR_RESET_FILTER_EVENT:
      (void) cim_ic_filter_event (ic, event);
      break;
    case ERROR_RESET_SET_CURSOR_POS:
      cim_ic_set_cursor_pos (ic, area);
      break;
    case ERROR_RESET_SET_CALLBACKS:
      cim_ic_set_callbacks (ic, callbacks, NULL);
      break;
    case ERROR_RESET_GET_PREEDIT:
      (void) cim_ic_get_preedit (ic);
      break;
    case ERROR_RESET_GET_CANDIDATE:
      (void) cim_ic_get_candidate (ic);
      break;
    case ERROR_RESET_ACTIVATE_CANDIDATE_ITEM:
      cim_ic_activate_candidate_item (ic, 0, 0);
      break;
    case ERROR_RESET_CHANGE_CANDIDATE_PAGE:
      cim_ic_change_candidate_page (ic, 0);
      break;
  }
}

static int
test_input_context_error_reset (void)
{
  static const struct error_reset_case cases[] = {
    { "cim_ic_focus_in", ERROR_RESET_FOCUS_IN },
    { "cim_ic_focus_out", ERROR_RESET_FOCUS_OUT },
    { "cim_ic_reset", ERROR_RESET_RESET },
    { "cim_ic_filter_event", ERROR_RESET_FILTER_EVENT },
    { "cim_ic_set_cursor_pos", ERROR_RESET_SET_CURSOR_POS },
    { "cim_ic_set_callbacks", ERROR_RESET_SET_CALLBACKS },
    { "cim_ic_get_preedit", ERROR_RESET_GET_PREEDIT },
    { "cim_ic_get_candidate", ERROR_RESET_GET_CANDIDATE },
    { "cim_ic_activate_candidate_item",
      ERROR_RESET_ACTIVATE_CANDIDATE_ITEM },
    { "cim_ic_change_candidate_page",
      ERROR_RESET_CHANGE_CANDIDATE_PAGE }
  };
  const char *home_env = getenv ("HOME");
  char *home = home_env != NULL ? strdup (home_env) : NULL;
  const bool had_home = home_env != NULL;
  CimCallbacks callbacks;
  CimEvent event = { 0 };
  CimRect area = { 0 };
  CimIcHandle ic = NULL;
  int retval = 1;

  if (had_home && home == NULL)
  {
    perror ("test_input_context_error_reset: strdup(HOME)");
    return 1;
  }

  if (test_set_plugin_env ("lib/im-dummy.so") != 0)
    goto cleanup;

  ic = cim_ic_create ();
  if (ic == NULL)
  {
    fprintf (stderr,
             "test_input_context_error_reset: create failed: %s\n",
             cim_strerror (cim_get_last_error ()));
    goto cleanup;
  }

  memset (&callbacks, 0, sizeof (callbacks));

  if (seed_home_not_set_error
        ("test_input_context_error_reset(diagnostics)", home, had_home) != 0)
    goto cleanup;

  (void) cim_strerror (cim_get_last_error ());

  if (expect_error_value
        ("test_input_context_error_reset(diagnostics preserve)",
         cim_get_last_error (),
         CIM_ERROR_HOME_NOT_SET) != 0)
    goto cleanup;

  for (size_t i = 0; i < sizeof (cases) / sizeof (cases[0]); ++i)
  {
    if (seed_home_not_set_error (cases[i].name, home, had_home) != 0)
      goto cleanup;

    run_error_reset_operation
      (cases[i].operation, ic, &callbacks, &event, &area);

    if (expect_error_value (cases[i].name,
                            cim_get_last_error (),
                            CIM_ERROR_NONE) != 0)
      goto cleanup;
  }

  retval = 0;

cleanup:
  if (ic != NULL)
    cim_ic_destroy (ic);

  if (restore_error_reset_environment (home, had_home) != 0)
    retval = 1;

  free (home);
  return retval;
}

static int
test_dummy_get_info (void)
{
  return test_get_info_smoke ("test_dummy_get_info",
                              "lib/im-dummy.so",
                              "dummy",
                              "Minimal dummy plugin");
}

static int
test_init_fail_get_info (void)
{
  return test_get_info_smoke ("test_init_fail_get_info",
                              "lib/im-init-fail.so",
                              "init-fail",
                              "Plugin whose init() always fails");
}

enum
{
  RACE_THREAD_COUNT = 32,
  RACE_ITERATIONS = 100
};

struct race_result
{
  int failures;
};

static void *
race_worker (void *data)
{
  struct race_result *result = (struct race_result *) data;

  for (int i = 0; i < RACE_ITERATIONS; ++i)
  {
    CimIcHandle ic;
    CimEvent event = { 0 };

    ic = cim_ic_create ();
    if (ic == NULL)
    {
      __sync_fetch_and_add (&result->failures, 1);
      continue;
    }

    cim_ic_focus_in (ic);
    cim_ic_reset (ic);
    (void) cim_ic_filter_event (ic, &event);
    cim_ic_focus_out (ic);
    cim_ic_destroy (ic);
  }

  return NULL;
}

static int
test_dummy_create_race (void)
{
  pthread_t threads[RACE_THREAD_COUNT];
  struct race_result result;

  memset (&result, 0, sizeof (result));

  if (test_set_plugin_env ("lib/im-dummy.so") != 0)
    return 1;

  if (test_set_verbose_env (false) != 0)
    return 1;

  for (int i = 0; i < RACE_THREAD_COUNT; ++i)
  {
    if (pthread_create (&threads[i], NULL, race_worker, &result) != 0)
    {
      fprintf (stderr, "test_dummy_create_race: pthread_create() failed\n");
      return 1;
    }
  }

  for (int i = 0; i < RACE_THREAD_COUNT; ++i)
  {
    if (pthread_join (threads[i], NULL) != 0)
    {
      fprintf (stderr, "test_dummy_create_race: pthread_join() failed\n");
      return 1;
    }
  }

  if (result.failures != 0)
  {
    fprintf (stderr,
             "test_dummy_create_race: %d create operations failed\n",
             result.failures);
    (void) test_set_verbose_env (true);
    return 1;
  }

  if (test_set_verbose_env (true) != 0)
    return 1;

  printf ("test_dummy_create_race: OK\n");

  return 0;
}

enum
{
  FAILURE_RACE_THREAD_COUNT = 16
};

struct failure_race_result
{
  int failures;
};

static void *
init_failure_race_worker (void *data)
{
  struct failure_race_result *result =
    (struct failure_race_result *) data;
  CimIcHandle ic = cim_ic_create ();

  if (ic != NULL)
  {
    cim_ic_destroy (ic);
    __sync_fetch_and_add (&result->failures, 1);
    return NULL;
  }

  if (cim_get_last_error () != CIM_ERROR_INIT_FAILED)
    __sync_fetch_and_add (&result->failures, 1);

  return NULL;
}

static int
test_init_failure_race (void)
{
  pthread_t threads[FAILURE_RACE_THREAD_COUNT];
  struct failure_race_result result;

  memset (&result, 0, sizeof (result));

  if (test_set_plugin_env ("lib/im-init-fail.so") != 0)
    return 1;

  for (int i = 0; i < FAILURE_RACE_THREAD_COUNT; ++i)
  {
    if (pthread_create
          (&threads[i], NULL, init_failure_race_worker, &result) != 0)
    {
      fprintf (stderr,
               "test_init_failure_race: pthread_create() failed\n");
      return 1;
    }
  }

  for (int i = 0; i < FAILURE_RACE_THREAD_COUNT; ++i)
  {
    if (pthread_join (threads[i], NULL) != 0)
    {
      fprintf (stderr,
               "test_init_failure_race: pthread_join() failed\n");
      return 1;
    }
  }

  if (result.failures != 0)
  {
    fprintf (stderr,
             "test_init_failure_race: %d operations failed\n",
             result.failures);
    return 1;
  }

  printf ("test_init_failure_race: OK\n");
  return 0;
}

int main (int argc, char *argv[])
{
  int retval = 0;

#ifdef CIM_HAS_INTERNAL_TEST_HOOK
  if (argc == 2 && strcmp (argv[1], UNHANDLED_EXCEPTION_MODE) == 0)
    return run_unhandled_exception_mode ();
#endif

#ifdef CIM_HAS_PLUGIN_REENTRY_TEST
  if (argc == 2 && strcmp (argv[1], PLUGIN_INIT_REENTRY_MODE) == 0)
    return run_plugin_init_reentry_mode ();

  if (argc == 2 && strcmp (argv[1], PLUGIN_FINI_REENTRY_MODE) == 0)
    return run_plugin_fini_reentry_mode ();
#endif

  if (argc == 2 && strcmp (argv[1], STALE_DESTROY_MODE) == 0)
    return run_stale_destroy_mode ();

  if (argc == 2 && strcmp (argv[1], STRERROR_INVALID_MODE) == 0)
    return run_strerror_invalid_mode ();

  if (argc == 3 && strcmp (argv[1], CONTRACT_NULL_MODE) == 0)
    return run_null_contract_mode (argv[2]);

  retval |= test_automatic_runtime_initialization ();
#ifdef CIM_HAS_INTERNAL_TEST_HOOK
  retval |= test_unhandled_exception_abort (argv[0]);
#endif
  retval |= test_stale_destroy_abort (argv[0]);
#ifdef CIM_HAS_PLUGIN_REENTRY_TEST
  retval |= test_plugin_lifecycle_reentry_abort (argv[0]);
#endif
  retval |= test_strerror_invalid_error_abort (argv[0]);
  retval |= test_dlclose_error_string ();
  retval |= test_null_contract_violation (argv[0], CONTRACT_DESTROY);
  retval |= test_null_contract_violation (argv[0], CONTRACT_FOCUS_IN);
  retval |= test_null_contract_violation (argv[0], CONTRACT_FOCUS_OUT);
  retval |= test_null_contract_violation (argv[0], CONTRACT_RESET);
  retval |= test_null_contract_violation (argv[0],
                                          CONTRACT_FILTER_EVENT_IC);
  retval |= test_null_contract_violation (argv[0],
                                          CONTRACT_FILTER_EVENT_EVENT);
  retval |= test_null_contract_violation (argv[0],
                                          CONTRACT_SET_CURSOR_POS_IC);
  retval |= test_null_contract_violation (argv[0],
                                          CONTRACT_SET_CURSOR_POS_AREA);
  retval |= test_null_contract_violation (argv[0],
                                          CONTRACT_SET_CALLBACKS_IC);
  retval |= test_null_contract_violation (
    argv[0], CONTRACT_SET_CALLBACKS_CALLBACKS);
  retval |= test_null_contract_violation (argv[0],
                                          CONTRACT_GET_PREEDIT);
  retval |= test_null_contract_violation (argv[0],
                                          CONTRACT_GET_CANDIDATE);
  retval |= test_null_contract_violation (
    argv[0], CONTRACT_ACTIVATE_CANDIDATE_ITEM);
  retval |= test_null_contract_violation (
    argv[0], CONTRACT_CHANGE_CANDIDATE_PAGE);
  retval |= test_dummy_single_create_destroy ();
  retval |= test_dummy_multiple_create_destroy ();
  retval |= test_load_fail ();
  retval |= test_init_fail ();
  retval |= test_create_fail ();
  retval |= test_bad_version ();
  retval |= test_no_symbol ();
  retval |= test_no_create ();
  retval |= test_no_destroy ();
  retval |= test_foreign_thread_error_isolation ();
  retval |= test_error_reset_after_success ();
  retval |= test_input_context_error_reset ();
  retval |= test_dummy_get_info ();
  retval |= test_init_fail_get_info ();
  retval |= test_init_failure_race ();
  retval |= test_dummy_create_race ();

  test_unset_plugin_env ();
  (void) test_set_verbose_env (false);

  if (retval == 0)
    printf ("OK\n");

  return retval;
}
