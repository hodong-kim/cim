#include "test-common.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
test_set_plugin_env (const char *target)
{
  const char *plugin_dir;
  const char *relative_target;
  char cwd[PATH_MAX];
  char abs_target[PATH_MAX];

  if (target == NULL || target[0] == '\0')
  {
    fprintf (stderr, "test_set_plugin_env: invalid target\n");
    return 1;
  }

  plugin_dir = getenv ("CIM_TEST_PLUGIN_DIR");
  relative_target = target;

  if (plugin_dir != NULL &&
      plugin_dir[0] != '\0' &&
      strncmp (relative_target, "lib/", 4) == 0)
    relative_target += 4;

  if (target[0] == '/')
  {
    if (snprintf (abs_target, sizeof (abs_target), "%s", target) >=
        (int) sizeof (abs_target))
      return 1;
  }
  else if (plugin_dir != NULL && plugin_dir[0] != '\0')
  {
    if (relative_target[0] == '\0')
    {
      fprintf (stderr, "test_set_plugin_env: invalid relative target\n");
      return 1;
    }

    if (snprintf (abs_target,
                  sizeof (abs_target),
                  "%s/%s",
                  plugin_dir,
                  relative_target) >= (int) sizeof (abs_target))
      return 1;
  }
  else
  {
    if (getcwd (cwd, sizeof (cwd)) == NULL)
    {
      perror ("getcwd");
      return 1;
    }

    if (snprintf (abs_target, sizeof (abs_target), "%s/%s", cwd, target) >=
        (int) sizeof (abs_target))
      return 1;
  }

  if (setenv ("CIM_PLUGIN", abs_target, 1) != 0)
  {
    perror ("setenv");
    return 1;
  }

  return 0;
}

int
test_set_verbose_env (bool enabled)
{
  if (enabled)
  {
    if (setenv ("TEST_VERBOSE", "1", 1) != 0)
    {
      perror ("setenv");
      return 1;
    }
  }
  else
  {
    if (unsetenv ("TEST_VERBOSE") != 0)
    {
      perror ("unsetenv");
      return 1;
    }
  }

  return 0;
}

void
test_unset_plugin_env (void)
{
  (void) unsetenv ("CIM_PLUGIN");
}
