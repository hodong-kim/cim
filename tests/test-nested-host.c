// -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-
/*
 * test-nested-host.c
 * Copyright (c) 2026 Hodong Kim <hodong@nimfsoft.com>
 * SPDX-License-Identifier: 0BSD
 */

#include "cim.h"

#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INNER_PLUGIN_NAME "libim-nested-counter.so"
#define FAIL_PLUGIN_NAME  "libim-init-fail.so"
#define OUTER_A_NAME      "outer-cim-a.so"
#define OUTER_B_NAME      "outer-cim-b.so"

typedef int (*Operation) (void);
typedef int (*Counter_Read) (void);
typedef CimIcHandle (*Cim_Create) (void);
typedef void (*Cim_Destroy) (CimIcHandle ic);


struct Counter_Api
{
  Operation reset;
  Counter_Read init_calls;
  Counter_Read fini_calls;
  Counter_Read live_attachments;
  Counter_Read create_calls;
  Counter_Read destroy_calls;
  Counter_Read live_contexts;
};

struct Shared_Cim_Api
{
  Cim_Create create;
  Cim_Destroy destroy;
};

struct Worker
{
  Operation operation;
  int result;
};

static int
make_path (char *result,
           size_t result_size,
           const char *directory,
           const char *name)
{
  int length;

  length = snprintf (result, result_size, "%s/%s", directory, name);
  if (length < 0 || (size_t) length >= result_size)
  {
    fprintf (stderr, "path is too long: %s/%s\n", directory, name);
    return 1;
  }

  return 0;
}

static void *
open_module_with_flags (const char *path, int flags)
{
  void *handle = dlopen (path, flags);

  if (handle == NULL)
    fprintf (stderr, "dlopen(%s) failed: %s\n", path, dlerror ());

  return handle;
}

static void *
open_module (const char *path)
{
  return open_module_with_flags (path, RTLD_NOW | RTLD_LOCAL);
}

static int
resolve_function (void *handle,
                  const char *name,
                  void *function_out,
                  size_t function_size)
{
  const char *error;
  void *symbol;

  if (function_size != sizeof (symbol))
  {
    fprintf (stderr, "unsupported function pointer size for %s\n", name);
    return 1;
  }

  dlerror ();
  symbol = dlsym (handle, name);
  error = dlerror ();
  if (error != NULL)
  {
    fprintf (stderr, "dlsym(%s) failed: %s\n", name, error);
    return 1;
  }

  memcpy (function_out, &symbol, function_size);
  return 0;
}

static int
expect_visible_symbol (void *handle, const char *name)
{
  const char *error;

  dlerror ();
  (void) dlsym (handle, name);
  error = dlerror ();
  if (error == NULL)
    return 0;

  fprintf (stderr, "expected symbol is not visible: %s: %s\n", name, error);
  return 1;
}

static int
expect_hidden_symbol (void *handle, const char *name)
{
  const char *error;

  dlerror ();
  (void) dlsym (handle, name);
  error = dlerror ();
  if (error != NULL)
    return 0;

  fprintf (stderr, "private symbol is visible: %s\n", name);
  return 1;
}

static int
load_counter_api (void *handle, struct Counter_Api *api)
{
#define RESOLVE_COUNTER(member, symbol_name)                              \
  do                                                                     \
  {                                                                      \
    if (resolve_function                                                 \
          (handle, symbol_name, &api->member, sizeof (api->member)) != 0) \
      return 1;                                                          \
  } while (0)

  RESOLVE_COUNTER (reset, "nested_counter_reset");
  RESOLVE_COUNTER (init_calls, "nested_counter_init_calls");
  RESOLVE_COUNTER (fini_calls, "nested_counter_fini_calls");
  RESOLVE_COUNTER (live_attachments, "nested_counter_live_attachments");
  RESOLVE_COUNTER (create_calls, "nested_counter_create_calls");
  RESOLVE_COUNTER (destroy_calls, "nested_counter_destroy_calls");
  RESOLVE_COUNTER (live_contexts, "nested_counter_live_contexts");

#undef RESOLVE_COUNTER
  return 0;
}

static int
load_shared_cim_api (void *handle, struct Shared_Cim_Api *api)
{
  if (resolve_function
        (handle, "cim_ic_create", &api->create, sizeof (api->create)) != 0 ||
      resolve_function
        (handle, "cim_ic_destroy", &api->destroy,
         sizeof (api->destroy)) != 0)
    return 1;

  return 0;
}

static int
expect_value (const char *name, int actual, int expected)
{
  if (actual == expected)
    return 0;

  fprintf (stderr,
           "%s: expected %d, got %d\n",
           name,
           expected,
           actual);
  return 1;
}

static int
check_counters (const struct Counter_Api *api,
                int init_count,
                int fini_count,
                int attachment_count,
                int create_count,
                int destroy_count,
                int context_count)
{
  int retval = 0;

  retval |= expect_value ("init calls", api->init_calls (), init_count);
  retval |= expect_value ("fini calls", api->fini_calls (), fini_count);
  retval |= expect_value
    ("live attachments", api->live_attachments (), attachment_count);
  retval |= expect_value ("create calls", api->create_calls (), create_count);
  retval |= expect_value
    ("destroy calls", api->destroy_calls (), destroy_count);
  retval |= expect_value
    ("live contexts", api->live_contexts (), context_count);
  return retval;
}

static int
run_sequential_cycle (const struct Counter_Api *api,
                      Operation open_a,
                      Operation close_a,
                      Operation open_b,
                      Operation close_b,
                      bool close_a_first)
{
  int retval = 1;
  bool a_open = false;
  bool b_open = false;

  if (api->reset () != 0)
  {
    fprintf (stderr, "counter reset rejected an idle state\n");
    return 1;
  }

  if (open_a () != 0)
  {
    fprintf (stderr, "outer A failed to create a context\n");
    goto cleanup;
  }
  a_open = true;

  if (open_b () != 0)
  {
    fprintf (stderr, "outer B failed to create a context\n");
    goto cleanup;
  }
  b_open = true;

  if (check_counters (api, 2, 0, 2, 2, 0, 2) != 0)
    goto cleanup;

  if (close_a_first)
  {
    if (close_a () != 0)
      goto cleanup;
    a_open = false;
  }
  else
  {
    if (close_b () != 0)
      goto cleanup;
    b_open = false;
  }

  if (check_counters (api, 2, 1, 1, 2, 1, 1) != 0)
    goto cleanup;

  if (close_a_first)
  {
    if (close_b () != 0)
      goto cleanup;
    b_open = false;
  }
  else
  {
    if (close_a () != 0)
      goto cleanup;
    a_open = false;
  }

  if (check_counters (api, 2, 2, 0, 2, 2, 0) != 0)
    goto cleanup;

  retval = 0;

cleanup:
  if (a_open)
    (void) close_a ();
  if (b_open)
    (void) close_b ();
  return retval;
}

static int
run_failure_isolation (const struct Counter_Api *api,
                       const char *failure_path,
                       const char *inner_path,
                       Operation failing_open,
                       Operation failing_close,
                       Operation other_open,
                       Operation other_close)
{
  int failure_result;
  bool failing_opened = false;
  bool other_opened = false;
  int retval = 1;

  if (api->reset () != 0)
  {
    fprintf (stderr, "counter reset rejected an idle state\n");
    return 1;
  }

  if (setenv ("CIM_PLUGIN", failure_path, 1) != 0)
  {
    perror ("setenv(CIM_PLUGIN failure)");
    return 1;
  }

  failure_result = failing_open ();
  if (failure_result == 0)
  {
    failing_opened = true;
    fprintf (stderr, "failing host unexpectedly created a context\n");
    goto cleanup;
  }

  if (failure_result != CIM_ERROR_INIT_FAILED)
  {
    fprintf (stderr,
             "failing host returned %d instead of CIM_ERROR_INIT_FAILED\n",
             failure_result);
    goto cleanup;
  }

  if (setenv ("CIM_PLUGIN", inner_path, 1) != 0)
  {
    perror ("setenv(CIM_PLUGIN inner)");
    goto cleanup;
  }

  if (other_open () != 0)
  {
    fprintf (stderr, "independent host failed after peer load failure\n");
    goto cleanup;
  }
  other_opened = true;

  if (failing_open () != 0)
  {
    fprintf (stderr, "failed host could not retry after load failure\n");
    goto cleanup;
  }
  failing_opened = true;

  if (check_counters (api, 2, 0, 2, 2, 0, 2) != 0)
    goto cleanup;

  if (other_close () != 0)
    goto cleanup;
  other_opened = false;

  if (check_counters (api, 2, 1, 1, 2, 1, 1) != 0)
    goto cleanup;

  if (failing_close () != 0)
    goto cleanup;
  failing_opened = false;

  if (check_counters (api, 2, 2, 0, 2, 2, 0) != 0)
    goto cleanup;

  retval = 0;

cleanup:
  if (setenv ("CIM_PLUGIN", inner_path, 1) != 0)
  {
    perror ("setenv(CIM_PLUGIN cleanup)");
    retval = 1;
  }

  if (failing_opened)
    (void) failing_close ();
  if (other_opened)
    (void) other_close ();
  return retval;
}

static void *
run_worker (void *data)
{
  struct Worker *worker = data;

  worker->result = worker->operation ();
  return NULL;
}

static int
run_operation_pair (Operation first, Operation second)
{
  struct Worker workers[2] = {
    { .operation = first, .result = -1 },
    { .operation = second, .result = -1 }
  };
  pthread_t threads[2];

  if (pthread_create (&threads[0], NULL, run_worker, &workers[0]) != 0)
  {
    fprintf (stderr, "pthread_create(first) failed\n");
    return 1;
  }

  if (pthread_create (&threads[1], NULL, run_worker, &workers[1]) != 0)
  {
    fprintf (stderr, "pthread_create(second) failed\n");
    (void) pthread_join (threads[0], NULL);
    return 1;
  }

  if (pthread_join (threads[0], NULL) != 0 ||
      pthread_join (threads[1], NULL) != 0)
  {
    fprintf (stderr, "pthread_join failed\n");
    return 1;
  }

  if (workers[0].result != 0 || workers[1].result != 0)
  {
    fprintf (stderr,
             "parallel operations failed: first=%d second=%d\n",
             workers[0].result,
             workers[1].result);
    return 1;
  }

  return 0;
}

static int
run_parallel_cycle (const struct Counter_Api *api,
                    Operation open_a,
                    Operation close_a,
                    Operation open_b,
                    Operation close_b)
{
  if (api->reset () != 0)
  {
    fprintf (stderr, "counter reset rejected an idle state\n");
    return 1;
  }

  if (run_operation_pair (open_a, open_b) != 0)
  {
    (void) close_a ();
    (void) close_b ();
    return 1;
  }

  if (check_counters (api, 2, 0, 2, 2, 0, 2) != 0)
  {
    (void) close_a ();
    (void) close_b ();
    return 1;
  }

  if (run_operation_pair (close_a, close_b) != 0)
    return 1;

  return check_counters (api, 2, 2, 0, 2, 2, 0);
}

static int
run_shared_coexistence_cycle (const struct Counter_Api *api,
                              const struct Shared_Cim_Api *shared,
                              Operation open_a,
                              Operation close_a,
                              Operation open_b,
                              Operation close_b,
                              bool close_shared_first)
{
  CimIcHandle shared_ic = NULL;
  bool a_open = false;
  bool b_open = false;
  int retval = 1;

  if (api->reset () != 0)
  {
    fprintf (stderr, "counter reset rejected an idle state\n");
    return 1;
  }

  shared_ic = shared->create ();
  if (shared_ic == NULL)
  {
    fprintf (stderr, "shared libcim.so failed to create a context\n");
    goto cleanup;
  }

  if (open_a () != 0)
  {
    fprintf (stderr, "outer A failed to create a context\n");
    goto cleanup;
  }
  a_open = true;

  if (open_b () != 0)
  {
    fprintf (stderr, "outer B failed to create a context\n");
    goto cleanup;
  }
  b_open = true;

  if (check_counters (api, 3, 0, 3, 3, 0, 3) != 0)
    goto cleanup;

  if (close_shared_first)
  {
    shared->destroy (shared_ic);
    shared_ic = NULL;

    if (check_counters (api, 3, 1, 2, 3, 1, 2) != 0)
      goto cleanup;

    if (close_a () != 0)
      goto cleanup;
    a_open = false;

    if (check_counters (api, 3, 2, 1, 3, 2, 1) != 0)
      goto cleanup;

    if (close_b () != 0)
      goto cleanup;
    b_open = false;
  }
  else
  {
    if (close_b () != 0)
      goto cleanup;
    b_open = false;

    if (check_counters (api, 3, 1, 2, 3, 1, 2) != 0)
      goto cleanup;

    if (close_a () != 0)
      goto cleanup;
    a_open = false;

    if (check_counters (api, 3, 2, 1, 3, 2, 1) != 0)
      goto cleanup;

    shared->destroy (shared_ic);
    shared_ic = NULL;
  }

  if (check_counters (api, 3, 3, 0, 3, 3, 0) != 0)
    goto cleanup;

  retval = 0;

cleanup:
  if (shared_ic != NULL)
    shared->destroy (shared_ic);
  if (a_open)
    (void) close_a ();
  if (b_open)
    (void) close_b ();
  return retval;
}

int
main (void)
{
  const char *plugin_directory = getenv ("CIM_TEST_PLUGIN_DIR");
  const char *shared_cim_path = getenv ("CIM_TEST_SHARED_CIM");
  char inner_path[PATH_MAX];
  char failure_path[PATH_MAX];
  char outer_a_path[PATH_MAX];
  char outer_b_path[PATH_MAX];
  struct Counter_Api counters;
  struct Shared_Cim_Api shared;
  Operation open_a = NULL;
  Operation close_a = NULL;
  Operation open_b = NULL;
  Operation close_b = NULL;
  void *inner_handle = NULL;
  void *outer_a_handle = NULL;
  void *outer_b_handle = NULL;
  void *shared_cim_handle = NULL;
  int retval = EXIT_FAILURE;

  memset (&counters, 0, sizeof (counters));
  memset (&shared, 0, sizeof (shared));

  if (plugin_directory == NULL || plugin_directory[0] == '\0')
  {
    fprintf (stderr, "CIM_TEST_PLUGIN_DIR is not configured\n");
    goto cleanup;
  }

  if (shared_cim_path == NULL || shared_cim_path[0] == '\0')
  {
    fprintf (stderr, "CIM_TEST_SHARED_CIM is not configured\n");
    goto cleanup;
  }

  if (make_path
        (inner_path, sizeof (inner_path), plugin_directory,
         INNER_PLUGIN_NAME) != 0 ||
      make_path
        (failure_path, sizeof (failure_path), plugin_directory,
         FAIL_PLUGIN_NAME) != 0 ||
      make_path
        (outer_a_path, sizeof (outer_a_path), plugin_directory,
         OUTER_A_NAME) != 0 ||
      make_path
        (outer_b_path, sizeof (outer_b_path), plugin_directory,
         OUTER_B_NAME) != 0)
    goto cleanup;

  if (setenv ("CIM_PLUGIN", inner_path, 1) != 0)
  {
    perror ("setenv(CIM_PLUGIN)");
    goto cleanup;
  }

  inner_handle = open_module (inner_path);
  if (inner_handle == NULL || load_counter_api (inner_handle, &counters) != 0)
    goto cleanup;

  outer_a_handle = open_module (outer_a_path);
  outer_b_handle = open_module (outer_b_path);
  if (outer_a_handle == NULL || outer_b_handle == NULL)
    goto cleanup;

  if (resolve_function
        (outer_a_handle, "outer_cim_a_open", &open_a, sizeof (open_a)) != 0 ||
      resolve_function
        (outer_a_handle, "outer_cim_a_close", &close_a,
         sizeof (close_a)) != 0 ||
      resolve_function
        (outer_b_handle, "outer_cim_b_open", &open_b, sizeof (open_b)) != 0 ||
      resolve_function
        (outer_b_handle, "outer_cim_b_close", &close_b,
         sizeof (close_b)) != 0)
    goto cleanup;

  if (expect_hidden_symbol (outer_a_handle, "cim_ic_create") != 0 ||
      expect_hidden_symbol (outer_a_handle, "ciminit") != 0 ||
      expect_hidden_symbol
        (outer_a_handle, "cim_private_ic_create") != 0 ||
      expect_hidden_symbol (outer_b_handle, "cim_ic_create") != 0 ||
      expect_hidden_symbol (RTLD_DEFAULT, "cim_ic_create") != 0)
    goto cleanup;

  if (run_sequential_cycle
        (&counters, open_a, close_a, open_b, close_b, true) != 0 ||
      run_sequential_cycle
        (&counters, open_a, close_a, open_b, close_b, false) != 0 ||
      run_parallel_cycle
        (&counters, open_a, close_a, open_b, close_b) != 0)
    goto cleanup;

  if (run_failure_isolation
        (&counters, failure_path, inner_path,
         open_a, close_a, open_b, close_b) != 0 ||
      run_failure_isolation
        (&counters, failure_path, inner_path,
         open_b, close_b, open_a, close_a) != 0)
    goto cleanup;

  if (dlclose (outer_b_handle) != 0)
  {
    fprintf (stderr, "dlclose(%s) failed: %s\n", outer_b_path, dlerror ());
    goto cleanup;
  }
  outer_b_handle = NULL;
  open_b = NULL;
  close_b = NULL;

  if (dlclose (outer_a_handle) != 0)
  {
    fprintf (stderr, "dlclose(%s) failed: %s\n", outer_a_path, dlerror ());
    goto cleanup;
  }
  outer_a_handle = NULL;
  open_a = NULL;
  close_a = NULL;

  shared_cim_handle =
    open_module_with_flags (shared_cim_path, RTLD_NOW | RTLD_GLOBAL);
  if (shared_cim_handle == NULL ||
      load_shared_cim_api (shared_cim_handle, &shared) != 0)
    goto cleanup;

  if (expect_visible_symbol (RTLD_DEFAULT, "cim_ic_create") != 0 ||
      expect_hidden_symbol (shared_cim_handle, "ciminit") != 0 ||
      expect_hidden_symbol (shared_cim_handle, "cimfinal") != 0 ||
      expect_hidden_symbol
        (shared_cim_handle, "cim_private_ic_create") != 0 ||
      expect_hidden_symbol
        (RTLD_DEFAULT, "cim_private_ic_create") != 0)
    goto cleanup;

  outer_a_handle = open_module (outer_a_path);
  outer_b_handle = open_module (outer_b_path);
  if (outer_a_handle == NULL || outer_b_handle == NULL)
    goto cleanup;

  if (resolve_function
        (outer_a_handle, "outer_cim_a_open", &open_a, sizeof (open_a)) != 0 ||
      resolve_function
        (outer_a_handle, "outer_cim_a_close", &close_a,
         sizeof (close_a)) != 0 ||
      resolve_function
        (outer_b_handle, "outer_cim_b_open", &open_b, sizeof (open_b)) != 0 ||
      resolve_function
        (outer_b_handle, "outer_cim_b_close", &close_b,
         sizeof (close_b)) != 0)
    goto cleanup;

  if (run_shared_coexistence_cycle
        (&counters, &shared, open_a, close_a, open_b, close_b, true) != 0 ||
      run_shared_coexistence_cycle
        (&counters, &shared, open_a, close_a, open_b, close_b, false) != 0)
    goto cleanup;

  retval = EXIT_SUCCESS;
  printf ("test-nested-host: OK\n");

cleanup:
  if (close_a != NULL)
    (void) close_a ();
  if (close_b != NULL)
    (void) close_b ();

  if (outer_b_handle != NULL && dlclose (outer_b_handle) != 0)
    retval = EXIT_FAILURE;
  if (outer_a_handle != NULL && dlclose (outer_a_handle) != 0)
    retval = EXIT_FAILURE;
  if (shared_cim_handle != NULL && dlclose (shared_cim_handle) != 0)
    retval = EXIT_FAILURE;
  if (inner_handle != NULL && dlclose (inner_handle) != 0)
    retval = EXIT_FAILURE;

  (void) unsetenv ("CIM_PLUGIN");
  return retval;
}
