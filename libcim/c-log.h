/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-log.h
 * This file is part of Clair.
 *
 * Copyright (C) 2020-2024 Hodong Kim <hodong@nimfsoft.art>
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
#ifndef __C_LOG_H__
#define __C_LOG_H__

#ifdef __linux__
  #ifndef _DEFAULT_SOURCE
    #define _DEFAULT_SOURCE
  #endif
#endif
#include <syslog.h>
#include "c-macros.h"
#include <stdarg.h>
#include <stdlib.h>
#include "c-str.h"

C_BEGIN_DECLS

#define c_log_critical(format, ...) \
  c_log (LOG_CRIT, __FILE__ ":%d:%s: " format, \
         __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__)

#define c_log_warning(format, ...) \
  c_log (LOG_WARNING, __FILE__ ":%d:%s: " format, \
         __LINE__, __PRETTY_FUNCTION__ __VA_OPT__(,) __VA_ARGS__)

#define c_log_info(format, ...) \
  c_log (LOG_INFO, format __VA_OPT__(,) __VA_ARGS__)

#ifdef DEBUG
  #define c_log_debug(format, ...) \
    c_log (LOG_DEBUG, __FILE__ ":%d: " format, __LINE__ \
           __VA_OPT__(,) __VA_ARGS__)
#else
  #define c_log_debug(format, ...)  ((void) 0)
#endif

void c_log (int priority, const char* format, ...);

C_END_DECLS

#endif /* __C_LOG_H__ */
