/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/helpers/errors.h>
#include <libcork/threads.h>

#include "clogger/api.h"
#include "clogger/formatter.h"
#include "clogger/handlers.h"


struct clog_stream_handler {
    struct clog_handler parent;
    struct cork_stream_consumer* consumer;
    volatile cork_thread_id active_thread;
    struct cork_buffer buf;
    struct clog_formatter* fmt;
    bool first_chunk;
};


/*-----------------------------------------------------------------------
 * Simple spin-lock
 */

/* Returns true if we've just claimed the lock; false if we already had it. */
static bool
clog_stream_handler_claim(struct clog_stream_handler* self)
{
    cork_thread_id tid = cork_current_thread_get_id();
    if (self->active_thread == tid) {
        return false;
    }

    while (cork_uint_cas(&self->active_thread, CORK_THREAD_NONE, tid) !=
           CORK_THREAD_NONE) {
        /* Someone else holds the lock.  Spin until it looks like it might be
         * free. */
        while (self->active_thread != CORK_THREAD_NONE) {
            cork_pause();
        }
    }

    return true;
}

static void
clog_stream_handler_release(struct clog_stream_handler* self)
{
    /* Assume that we already have the lock */
    self->active_thread = CORK_THREAD_NONE;
}


/*-----------------------------------------------------------------------
 * Stream handler
 */

static void
clog_stream_handler_handle(struct clog_handler* handler,
                           struct clog_message* message)
{
    struct clog_stream_handler* self =
            cork_container_of(handler, struct clog_stream_handler, parent);

    clog_stream_handler_claim(self);
    clog_formatter_format_message(self->fmt, &self->buf, message);
    cork_buffer_append(&self->buf, "\n", 1);
    cork_stream_consumer_data(self->consumer, self->buf.buf, self->buf.size,
                              self->first_chunk);
    self->first_chunk = false;
    clog_stream_handler_release(self);

    if (handler->next != NULL) {
        clog_handler_handle(handler->next, message);
    }
}

static void
clog_stream_handler_free(struct clog_handler* vself);


/*-----------------------------------------------------------------------
 * Stream consumer for FILE objects
 */

struct stream_consumer {
    struct cork_stream_consumer parent;
    FILE* fp;
    bool should_close;
};

static int
stream_consumer_data(struct cork_stream_consumer* vself, const void* buf,
                     size_t size, bool is_first)
{
    struct stream_consumer* self =
            cork_container_of(vself, struct stream_consumer, parent);
    size_t bytes_written = fwrite(buf, 1, size, self->fp);
    /* If there was an error writing to the file, then signal this to the
     * producer */
    if (bytes_written == size) {
        return 0;
    } else {
        cork_system_error_set();
        return -1;
    }
}

static int
stream_consumer_eof(struct cork_stream_consumer* vself)
{
    return 0;
}

static void
stream_consumer_free(struct cork_stream_consumer* vself)
{
    struct stream_consumer* self =
            cork_container_of(vself, struct stream_consumer, parent);
    if (self->should_close) {
        fclose(self->fp);
    }
    cork_delete(struct stream_consumer, self);
}

struct cork_stream_consumer*
stream_consumer_new(FILE* fp, bool should_close)
{
    struct stream_consumer* self = cork_new(struct stream_consumer);
    self->parent.data = stream_consumer_data;
    self->parent.eof = stream_consumer_eof;
    self->parent.free = stream_consumer_free;
    self->fp = fp;
    self->should_close = should_close;
    return &self->parent;
}


/*-----------------------------------------------------------------------
 * Constructors and destructors
 */

static void
clog_stream_handler_free(struct clog_handler* vself)
{
    struct clog_stream_handler* self =
            cork_container_of(vself, struct clog_stream_handler, parent);
    cork_stream_consumer_free(self->consumer);
    cork_buffer_done(&self->buf);
    if (self->fmt != NULL) {
        clog_formatter_free(self->fmt);
    }
    cork_delete(struct clog_stream_handler, self);
}

struct clog_handler*
clog_stream_handler_new_consumer(struct cork_stream_consumer* consumer,
                                 const char* fmt)
{
    struct clog_stream_handler* self = cork_new(struct clog_stream_handler);
    self->parent.handle = clog_stream_handler_handle;
    self->parent.free = clog_stream_handler_free;
    self->consumer = consumer;
    self->active_thread = CORK_THREAD_NONE;
    cork_buffer_init(&self->buf);
    self->first_chunk = true;
    ep_check(self->fmt = clog_formatter_new(fmt));
    return &self->parent;

error:
    clog_stream_handler_free(&self->parent);
    return NULL;
}

struct clog_handler*
clog_stream_handler_new_fp(FILE* fp, bool should_close, const char* fmt)
{
    struct cork_stream_consumer  *consumer =
        stream_consumer_new(fp, should_close);
    return clog_stream_handler_new_consumer(consumer, fmt);
}

struct clog_handler*
clog_stderr_handler_new(const char* fmt)
{
    return clog_stream_handler_new_fp(stderr, false, fmt);
}
