/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <libcork/core.h>
#include <libcork/ds.h>

#include "clogger/api.h"
#include "clogger/fields.h"

void
clog_message_add_string_field(struct clog_message_fields* fields,
                              struct clog_string_field* field, const char* key,
                              const char* value);

void
clog_printf_field_done(struct clog_message_field* vfield)
{
    struct clog_printf_field* field =
            cork_container_of(vfield, struct clog_printf_field, parent);
    cork_buffer_done(&field->value);
}

void
clog_message_add_printf_field(struct clog_message_fields* fields,
                              struct clog_printf_field* field, const char* key,
                              const char* fmt, ...);
