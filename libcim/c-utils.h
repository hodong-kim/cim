/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * c-utils.h
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
#ifndef __C_UTILS_H__
#define __C_UTILS_H__

#include <stdbool.h>
#include <sys/stat.h>
#include "c-macros.h"
#include <stdio.h>
#include <sys/types.h>

C_BEGIN_DECLS

uid_t       c_get_loginuid (void);
const char *c_get_user_home_dir   (void);
char       *c_get_user_config_dir (void);
bool        c_mkdir_p (const char *pathname, mode_t mode);

C_END_DECLS

#endif /* __C_UTILS_H__ */
