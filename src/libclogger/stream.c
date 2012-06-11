/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/threads.h>
#include <libcork/helpers/errors.h>

#include "clogger/api.h"
#include "clogger/handlers.h"


struct clog_stream_handler {
    struct clog_handler  parent;
    struct cork_stream_consumer  *consumer;
    volatile cork_thread_id  active_thread;
    struct cork_buffer  annotations_buf;
    struct cork_buffer  msg_buf;
    bool  first_chunk;
};


/*-----------------------------------------------------------------------
 * Simple spin-lock
 */

/* Returns true if we've just claimed the lock; false if we already had it. */
static bool
clog_stream_handler_claim(struct clog_stream_handler *self)
{
    cork_thread_id  tid = cork_thread_get_id();
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
clog_stream_handler_release(struct clog_stream_handler *self)
{
    /* Assume that we already have the lock */
    self->active_thread = CORK_THREAD_NONE;
}


/*-----------------------------------------------------------------------
 * Stream handler
 */

static int
clog_stream_handler__annotation(struct clog_handler *vself,
                                struct clog_message *msg,
                                const char *key, const char *value)
{
    struct clog_stream_handler  *self =
        cork_container_of(vself, struct clog_stream_handler, parent);

    if (clog_stream_handler_claim(self)) {
        /* Just claimed the lock; clear the buffer */
        cork_buffer_printf(&self->annotations_buf, "%s=%s", key, value);
    } else {
        cork_buffer_append_printf(&self->annotations_buf, " %s=%s", key, value);
    }

    return CLOG_CONTINUE;
}

static int
clog_stream_handler__message(struct clog_handler *vself,
                             struct clog_message *msg)
{
    struct clog_stream_handler  *self =
        cork_container_of(vself, struct clog_stream_handler, parent);
    char  *annotations;

    clog_stream_handler_claim(self);
    annotations = self->annotations_buf.buf;
    if (annotations == NULL) {
        annotations = "";
    }
    cork_buffer_printf(&self->msg_buf, "[%s] %s:%s ",
                       clog_level_name_fixed_width(msg->level),
                       msg->channel, annotations);
    cork_buffer_append_vprintf(&self->msg_buf, msg->format, msg->args);
    cork_buffer_append(&self->msg_buf, "\n", 1);

    rii_check(cork_stream_consumer_data
              (self->consumer, self->msg_buf.buf, self->msg_buf.size,
               self->first_chunk));
    self->first_chunk = false;
    clog_stream_handler_release(self);
    return CLOG_CONTINUE;
}

static void
clog_stream_handler__free(struct clog_handler *vself);


/*-----------------------------------------------------------------------
 * Stream consumer for FILE objects
 */

struct stream_consumer {
    struct cork_stream_consumer  parent;
    FILE  *fp;
    bool  should_close;
};

static int
stream_consumer__data(struct cork_stream_consumer *vself,
                      const void *buf, size_t size, bool is_first)
{
    struct stream_consumer  *self =
        cork_container_of(vself, struct stream_consumer, parent);
    size_t  bytes_written = fwrite(buf, 1, size, self->fp);
    /* If there was an error writing to the file, then signal this to
     * the producer */
    if (bytes_written == size) {
        return 0;
    } else {
        cork_system_error_set();
        return -1;
    }
}

static int
stream_consumer__eof(struct cork_stream_consumer *vself)
{
    return 0;
}

static void
stream_consumer__free(struct cork_stream_consumer *vself)
{
    struct stream_consumer  *self =
        cork_container_of(vself, struct stream_consumer, parent);
    if (self->should_close) {
        fclose(self->fp);
    }
    free(self);
}

struct cork_stream_consumer *
stream_consumer_new(FILE *fp, bool should_close)
{
    struct stream_consumer  *self = cork_new(struct stream_consumer);
    self->parent.data = stream_consumer__data;
    self->parent.eof = stream_consumer__eof;
    self->parent.free = stream_consumer__free;
    self->fp = fp;
    self->should_close = should_close;
    return &self->parent;
}


/*-----------------------------------------------------------------------
 * Constructors and destructors
 */

struct clog_handler *
clog_stream_handler_new_consumer(struct cork_stream_consumer *consumer)
{
    struct clog_stream_handler  *self = cork_new(struct clog_stream_handler);
    self->parent.annotation = clog_stream_handler__annotation;
    self->parent.message = clog_stream_handler__message;
    self->parent.free = clog_stream_handler__free;
    self->consumer = consumer;
    self->active_thread = CORK_THREAD_NONE;
    cork_buffer_init(&self->annotations_buf);
    cork_buffer_init(&self->msg_buf);
    self->first_chunk = true;
    return &self->parent;
}

struct clog_handler *
clog_stream_handler_new_fp(FILE *fp, bool should_close)
{
    struct cork_stream_consumer  *consumer =
        stream_consumer_new(fp, should_close);
    return clog_stream_handler_new_consumer(consumer);
}

struct clog_handler *
clog_stderr_handler_new(void)
{
    return clog_stream_handler_new_fp(stderr, false);
}

static void
clog_stream_handler__free(struct clog_handler *vself)
{
    struct clog_stream_handler  *self =
        cork_container_of(vself, struct clog_stream_handler, parent);
    cork_stream_consumer_free(self->consumer);
    cork_buffer_done(&self->annotations_buf);
    cork_buffer_done(&self->msg_buf);
    free(self);
}
