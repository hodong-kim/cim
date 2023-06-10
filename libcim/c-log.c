/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-log.c
 * This file is part of Clair.
 *
 * Copyright (C) 2020-2023 Hodong Kim <hodong@nimfsoft.art>
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
#include "c-log.h"
#include "c-progname.h"
#include <errno.h>

void c_log (int priority, const char* format, ...)
{
  const char* prefix;
  char*       new_format;
  va_list     args;

  switch (priority)
  {
    case LOG_EMERG:
      prefix = "EMERGENCY";
      break;
    case LOG_ALERT:
      prefix = "ALERT";
      break;
    case LOG_CRIT:
      prefix = "CRITICAL";
      break;
    case LOG_ERR:
      prefix = "ERROR";
      break;
    case LOG_WARNING:
      prefix = "WARNING";
      break;
    case LOG_NOTICE:
      prefix = "NOTICE";
      break;
    case LOG_INFO:
      prefix = "INFO";
      break;
    case LOG_DEBUG:
      prefix = "DEBUG";
      break;
    default:
      prefix = "NOTICE";
      break;
  }

#ifdef __linux__
  char* progname;
  progname = c_get_progname ();
  openlog (progname ? progname : "(null)", LOG_PID | LOG_PERROR, LOG_DAEMON);
#else
  openlog (getprogname(), LOG_PID | LOG_PERROR, LOG_DAEMON);
#endif
  new_format = c_str_join (prefix, ": ", format, nullptr);

  /*
   * 2022.12.18
   * https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=268436
   * The freebsd libc syslog, vsyslog does not preserve the errno at the time of
   * the call, so errno may change after the syslog call.
   */
  int saved_errno = errno;
  va_start (args, format);
  vsyslog  (priority, new_format, args);
  va_end   (args);
  errno = saved_errno;

  free (new_format);
  closelog ();
#ifdef __linux__
  free (progname);
#endif
}
