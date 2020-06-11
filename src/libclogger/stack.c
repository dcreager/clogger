/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/helpers/errors.h>
#include <libcork/threads.h>

#include "clogger/api.h"
#include "clogger/error.h"


enum clog_level clog_minimum_level = CLOG_LEVEL_WARNING;

static bool can_push_process_handlers = true;
static struct clog_handler* process_stack = NULL;
cork_tls(struct clog_handler*, thread_stack);


void
clog_handler_push_process(struct clog_handler* handler)
{
    assert(can_push_process_handlers);
    handler->next = process_stack;
    process_stack = handler;
}

int
clog_handler_pop_process(struct clog_handler* handler)
{
    struct clog_handler** thread_stack = thread_stack_get();
    assert(*thread_stack == NULL);
    if (CORK_UNLIKELY(process_stack != handler)) {
        clog_bad_stack("Unexpected handler when popping from process stack");
        return -1;
    }

    process_stack = handler->next;
    handler->next = NULL;
    return 0;
}

void
clog_handler_push_thread(struct clog_handler* handler)
{
    struct clog_handler** thread_stack = thread_stack_get();
    can_push_process_handlers = false;
    if (*thread_stack == NULL) {
        handler->next = process_stack;
    } else {
        handler->next = *thread_stack;
    }
    *thread_stack = handler;
}

int
clog_handler_pop_thread(struct clog_handler* handler)
{
    struct clog_handler** thread_stack = thread_stack_get();
    if (CORK_UNLIKELY(*thread_stack != handler)) {
        clog_bad_stack("Unexpected handler when popping from thread stack");
        return -1;
    }

    if (handler->next == process_stack) {
        *thread_stack = NULL;
        handler->next = NULL;
        return 0;
    }

    *thread_stack = handler->next;
    handler->next = NULL;
    return 0;
}

static struct clog_handler*
clog_get_stack(void)
{
    struct clog_handler** thread_stack = thread_stack_get();
    if (*thread_stack == NULL) {
        return process_stack;
    } else {
        return *thread_stack;
    }
}

const char*
clog_message_message(struct clog_message* message)
{
    if (message->message.buf == NULL) {
        cork_buffer_vprintf(&message->message, message->fmt, message->args);
    }
    return message->message.buf;
}

void
_clog_process_message(struct clog_message* message, const char* fmt, ...)
{
    struct clog_handler* handler = clog_get_stack();
    if (handler == NULL) {
        clog_message_done(message);
        return;
    }
    message->fmt = fmt;
    va_start(message->args, fmt);
    handler->handle(handler, message);
    va_end(message->args);
    clog_message_done(message);
}


void
clog_set_minimum_level(enum clog_level level)
{
    clog_minimum_level = level;
}


/*-----------------------------------------------------------------------
 * Inline declarations
 */

void
clog_message_field_done(struct clog_message_field* field);

void
clog_message_fields_init(struct clog_message_fields* fields);

void
clog_message_fields_push(struct clog_message_fields* fields,
                         struct clog_message_field* field);

void
clog_message_fields_pop(struct clog_message_fields* fields,
                        struct clog_message_field* field);

void
clog_message_fields_done(struct clog_message_fields* fields);

void
clog_message_init(struct clog_message* message, enum clog_level level,
                  const char* channel);

void
clog_message_pop_field(struct clog_message* message,
                       struct clog_message_field* field);

void
clog_message_done(struct clog_message* message);

void
clog_handler_handle(struct clog_handler* handler, struct clog_message* message);

void
clog_handler_free(struct clog_handler* handler);
