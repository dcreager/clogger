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

void
clog_process_message(struct clog_message* msg)
{
    struct clog_handler* handler = clog_get_stack();
    clog_handler_message(handler, msg);
}

void
clog_annotate_message(struct clog_message* msg, const char* key,
                      const char* value)
{
    struct clog_handler* handler = clog_get_stack();
    clog_handler_annotation(handler, msg, key, value);
}


void
clog_set_minimum_level(enum clog_level level)
{
    clog_minimum_level = level;
}

void
_clog_init_message(struct clog_message* msg, enum clog_level level,
                   const char* channel)
{
    msg->level = level;
    msg->channel = channel;
    msg->format = NULL;
}

void
_clog_finish_message(struct clog_message* msg, const char* format, ...)
{
    msg->format = format;
    va_start(msg->args, format);
    clog_process_message(msg);
    va_end(msg->args);
}

/* Include a linkable (but deprecated) copy of this for any existing code
 * compiled against ≤ v1.0. */
void
_clog_log_channel(enum clog_level level, const char* channel,
                  const char* format, ...)
{
    /* Otherwise create a clog_message object and pass it off to all of the
     * handlers. */
    struct clog_message  msg;
    msg.level = level;
    msg.channel = channel;
    msg.format = format;
    va_start(msg.args, format);
    clog_process_message(&msg);
    va_end(msg.args);
}


/*-----------------------------------------------------------------------
 * Inline declarations
 */

void
clog_handler_annotation(struct clog_handler* handler, struct clog_message* msg,
                        const char* key, const char* value);

void
clog_handler_message(struct clog_handler* handler, struct clog_message* msg);

void
clog_handler_free(struct clog_handler* handler);
