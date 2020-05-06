/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef CLOGGER_FORMATTER_H
#define CLOGGER_FORMATTER_H

#include <stdarg.h>

#include <libcork/core.h>
#include <libcork/ds.h>

#include <clogger/api.h>


struct clog_formatter;

struct clog_formatter*
clog_formatter_new(const char* format);

void
clog_formatter_free(struct clog_formatter* fmt);

void
clog_formatter_format_message(struct clog_formatter* fmt,
                              struct cork_buffer* dest,
                              struct clog_message* message);


#endif /* CLOGGER_FORMATTER_H */
