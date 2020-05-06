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
#include <libcork/ds.h>

#include <clogger/api.h>


/*-----------------------------------------------------------------------
 * Strings
 */

struct clog_string_field {
    struct clog_message_field parent;
};

CORK_INLINE
void
clog_message_add_string_field(struct clog_message* message,
                              struct clog_string_field* field, const char* key,
                              const char* value)
{
    field->parent.key = key;
    field->parent.value = value;
    field->parent.done = NULL;
    cork_dllist_add_to_tail(&message->fields, &field->parent.item);
}


/*-----------------------------------------------------------------------
 * Formatted strings
 */

struct clog_printf_field {
    struct clog_message_field parent;
    struct cork_buffer value;
};

void
clog_printf_field_done(struct clog_message_field* field);

CORK_INLINE
CORK_ATTR_PRINTF(4, 5)
void
clog_message_add_printf_field(struct clog_message* message,
                              struct clog_printf_field* field, const char* key,
                              const char* fmt, ...)
{
    field->parent.key = key;
    field->parent.done = clog_printf_field_done;
    cork_buffer_init(&field->value);
    va_list args;
    va_start(args, fmt);
    cork_buffer_vprintf(&field->value, fmt, args);
    va_end(args);
    field->parent.value = field->value.buf;
    cork_dllist_add_to_tail(&message->fields, &field->parent.item);
}


#endif /* CLOGGER_FIELDS_H */
