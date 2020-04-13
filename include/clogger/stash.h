/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef CLOGGER_STASH_H
#define CLOGGER_STASH_H

#include <libcork/core.h>

#include <clogger/api.h>

struct clog_stash;

struct clog_stash*
clog_stash_new(void);

void
clog_stash_free(struct clog_stash* stash);

CORK_ATTR_SENTINEL
bool
clog_stash_contains_event(const struct clog_stash* stash, ...);

struct clog_handler*
clog_stashing_handler_new(struct clog_stash* stash);

#endif /* CLOGGER_STASH_H */
