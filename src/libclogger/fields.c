/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>

#include <libcork/core.h>
#include <libcork/ds.h>

#include "clogger/api.h"
#include "clogger/fields.h"

void
clog_add_string_field(struct clog_message* msg, const char* key,
                      const char* value)
{
    clog_annotate_message_field(msg, key, value);
}

void
clog_add_printf_field(struct clog_message* msg, const char* key,
                      const char* fmt, ...)
{
    struct cork_buffer buf = CORK_BUFFER_INIT();
    va_list args;
    va_start(args, fmt);
    cork_buffer_vprintf(&buf, fmt, args);
    va_end(args);
    clog_annotate_message_field(msg, key, buf.buf);
    cork_buffer_done(&buf);
}
