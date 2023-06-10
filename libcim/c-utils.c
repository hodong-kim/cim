/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-utils.c
 * This file is part of Clair.
 *
 * Copyright (C) 2019-2023 Hodong Kim <hodong@nimfsoft.art>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "c-utils.h"
#include "c-str.h"
#include <errno.h>
#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>

uid_t c_get_loginuid ()
{
  uid_t loginuid = -1;

#ifdef __linux__
  FILE* file;

  file = fopen ("/proc/self/loginuid", "rt");

  if (file)
  {
    uid_t uid;

    if (fscanf (file, "%d", &uid) > 0)
      loginuid = uid;

    fclose (file);
  }
#else
  const char* name;

  if ((name = getlogin ()))
  {
    const struct passwd* info;

    if ((info = getpwnam (name)))
      loginuid = info->pw_uid;
  }
#endif

  if (loginuid == (uid_t) -1)
    loginuid = getuid ();

  return loginuid;
}

/*
 * Returns the home directory on success, or NULL on failure.
 * Do not free it.
 */
const char* c_get_user_home_dir ()
{
  const char* home = getpwuid (c_get_loginuid ())->pw_dir;
  if (!home)
    home = getenv ("HOME");

  return home;
}

/*
 * Returns the newly allocated string on success, or NULL on failure.
 * Free it with free().
 */
char* c_get_user_config_dir ()
{
  const char* s1;
  const char* s2 = "/.config";

  s1 = c_get_user_home_dir ();

  if (!s1)
    return NULL;

  return c_str_join (s1, s2, nullptr);
}

/* returns true on success, false on failure */
bool c_mkdir_p (const char *pathname, mode_t mode)
{
  char *path;
  char *p;
  char  c = 0;
  struct stat status;
  bool retval = false;

  if (!pathname || *pathname == '\0')
    return retval;

  path = c_strdup (pathname);
  p = path;

  while (true)
  {
    if (*p == '/')
    {
      c = *(p + 1);
      *(p + 1) = '\0';
    }

    if (*p == '/' || *p == '\0')
    {
      if (stat(path, &status) || !S_ISDIR(status.st_mode))
      {
        errno = 0;

        if (mkdir (path, mode))
        {
          retval = false;
          break;
        }
      }

      retval = true;
    }

    if (*p == '/')
      *(p + 1) = c;

    if (*p == '\0')
      break;

    p++;
  }

  free (path);

  return retval;
}
