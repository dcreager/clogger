/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/threads.h>
#include <libcork/helpers/errors.h>

#include "clogger/api.h"
#include "clogger/error.h"


static enum clog_level  maximum_level = CLOG_LEVEL_WARNING;
static struct clog_handler  *process_stack = NULL;
cork_tls(struct clog_handler *, thread_stack);


void
clog_handler_push_process(struct clog_handler *handler)
{
    handler->next = process_stack;
    process_stack = handler;
}

int
clog_handler_pop_process(struct clog_handler *handler)
{
    if (CORK_LIKELY(process_stack == handler)) {
        process_stack = handler->next;
        handler->next = NULL;
        return 0;
    } else {
        clog_bad_stack("Unexpected handler when popping from process stack");
        return -1;
    }
}

void
clog_handler_push_thread(struct clog_handler *handler)
{
    struct clog_handler  **thread_stack = thread_stack_get();
    handler->next = *thread_stack;
    *thread_stack = handler;
}

int
clog_handler_pop_thread(struct clog_handler *handler)
{
    struct clog_handler  **thread_stack = thread_stack_get();

    if (CORK_LIKELY(*thread_stack == handler)) {
        *thread_stack = handler->next;
        handler->next = NULL;
        return 0;
    } else {
        clog_bad_stack("Unexpected handler when popping from thread stack");
        return -1;
    }
}


int
clog_process_message(struct clog_message *msg)
{
    /* First send the message through the handlers in the process stack, and
     * then the handlers in the thread stack.  If any of them return
     * CLOG_SKIP_MESSAGE, immediately abort the processing of the message. */

    struct clog_handler  **thread_stack = thread_stack_get();
    struct clog_handler  *handler;

    for (handler = process_stack; handler != NULL; handler = handler->next) {
        rii_check(clog_handler_message(handler, msg));
    }

    for (handler = *thread_stack; handler != NULL; handler = handler->next) {
        rii_check(clog_handler_message(handler, msg));
    }

    return 0;
}

void
clog_set_maximum_level(enum clog_level level)
{
    maximum_level = level;
}

void
clog_log(enum clog_level level, const char *format, ...)
{

    if (CORK_LIKELY(level > maximum_level)) {
        /* Skip the message if it's above the requested maximum level */
    } else {
        /* Otherwise create a clog_message object and pass it off to all of the
         * handlers. */
        struct clog_message  msg;
        msg.level = level;
        msg.format = format;
        va_start(msg.args, format);
        clog_process_message(&msg);
        va_end(msg.args);
    }
}
