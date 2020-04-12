/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef CLOGGER_FIELDS_H
#define CLOGGER_FIELDS_H

#include <libcork/core.h>

#include <clogger/api.h>

void
clog_add_string_field(struct clog_message* msg, const char* key,
                      const char* value);

void
clog_add_printf_field(struct clog_message* msg, const char* key,
                      const char* fmt, ...) CORK_ATTR_PRINTF(3, 4);

#endif /* CLOGGER_FIELDS_H */
