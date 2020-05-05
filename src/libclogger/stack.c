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

static int
clog_process_message_(struct clog_message *msg, va_list args)
{
    /* First send the message through the handlers in the thread stack, and
     * then the handlers in the process stack.  If any of them return
     * CLOG_SKIP_MESSAGE, immediately abort the processing of the message. */

    struct clog_handler* handler;
    for (handler = clog_get_stack(); handler != NULL; handler = handler->next) {
        int rc;
        va_copy(msg->args, args);
        rc = clog_handler_message(handler, msg);
        va_end(msg->args);
        if (rc == CLOG_SKIP) {
            return 0;
        } else if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

int
clog_process_message(struct clog_message *msg)
{
    va_list args;
    va_copy(args, msg->args);
    int rc = clog_process_message_(msg, args);
    va_end(args);
    return rc;
}

int
clog_annotate_message(struct clog_handler *self, struct clog_message *msg,
                      const char *key, const char *value)
{
    /* First send the annotation through the handlers in the thread stack, and
     * then the handlers in the process stack.  If any of them return
     * CLOG_SKIP_MESSAGE, immediately abort the processing of the message. */

    bool found_handler = false;
    struct clog_handler* handler;

    /* We're not supposed to send the annotation to any handlers "above" the
     * current one in the stack. */

    for (handler = clog_get_stack(); handler != NULL; handler = handler->next) {
        if (self == handler) {
            found_handler = true;
        }

        if (found_handler) {
            int rc = clog_handler_annotation(handler, msg, key, value);
            if (rc == CLOG_SKIP) {
                return 0;
            } else if (rc != 0) {
                return rc;
            }
        }
    }

    return 0;
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
clog_annotate_message_field(struct clog_message* msg, const char* key,
                            const char* value)
{
    /* Just like clog_annotate_message, but we always send the annotation
     * through all registered handlers. */
    struct clog_handler* handler;
    for (handler = clog_get_stack(); handler != NULL; handler = handler->next) {
        clog_handler_annotation(handler, msg, key, value);
    }
}

void
_clog_finish_message(struct clog_message* msg, const char* format, ...)
{
    va_list args;
    msg->format = format;
    va_start(args, format);
    clog_process_message_(msg, args);
    va_end(args);
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
    va_list args;
    msg.level = level;
    msg.channel = channel;
    msg.format = format;
    va_start(args, format);
    clog_process_message_(&msg, args);
    va_end(args);
}
