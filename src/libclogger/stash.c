/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>

#include <libcork/ds.h>
#include <libcork/helpers/errors.h>

#include "clogger/api.h"
#include "clogger/stash.h"

struct clog_stashed_event {
    struct cork_hash_table* fields;
};

static void
cork_strfree_callback(void* vstr)
{
    const char* str = (const char*) vstr;
    cork_strfree(str);
}

static struct clog_stashed_event*
clog_stashed_event_new(void)
{
    struct clog_stashed_event* event = cork_new(struct clog_stashed_event);
    event->fields = cork_string_hash_table_new(0, 0);
    cork_hash_table_set_free_key(event->fields, cork_strfree_callback);
    cork_hash_table_set_free_value(event->fields, cork_strfree_callback);
    return event;
}

static void
clog_stashed_event_free(struct clog_stashed_event* event)
{
    cork_hash_table_free(event->fields);
    cork_delete(struct clog_stashed_event, event);
}

static bool
clog_stashed_event_matches(const struct clog_stashed_event* event, va_list args)
{
    const char* key;
    const char* expected;

    while (true) {
        key = va_arg(args, const char*);
        if (key == NULL) {
            return true;
        }

        expected = va_arg(args, const char*);
        if (expected == NULL) {
            // False because the caller didn't provide the right number of
            // parameters.
            return false;
        }

        const char *actual = cork_hash_table_get(event->fields, key);
        if (actual == NULL || strcmp(expected, actual) != 0) {
            return false;
        }
    }
}

static void
clog_stashed_event_add(struct clog_stashed_event* event, const char* key,
                       const char* value)
{
    const char* key_copy = cork_strdup(key);
    const char* value_copy = cork_strdup(value);
    bool is_new;
    void* old_key;
    void* old_value;
    cork_hash_table_put(event->fields, (void*) key_copy, (void*) value_copy,
                        &is_new, &old_key, &old_value);
    if (!is_new) {
        cork_strfree((const char*) old_key);
        cork_strfree((const char*) old_value);
    }
}

static void
clog_stashed_event_free_callback(void* ud, void* vevent)
{
    struct clog_stashed_event** event = vevent;
    clog_stashed_event_free(*event);
}

struct clog_stash {
    cork_array(struct clog_stashed_event*) events;
};

struct clog_stash*
clog_stash_new(void)
{
    struct clog_stash* stash = cork_new(struct clog_stash);
    cork_array_init(&stash->events);
    cork_array_set_remove(&stash->events, clog_stashed_event_free_callback);
    cork_array_set_done(&stash->events, clog_stashed_event_free_callback);
    return stash;
}

void
clog_stash_free(struct clog_stash* stash)
{
    cork_array_done(&stash->events);
    cork_delete(struct clog_stash, stash);
}

bool
clog_stash_contains_event(const struct clog_stash* stash, ...)
{
    for (size_t i = 0; i < cork_array_size(&stash->events); i++) {
        va_list args;
        va_start(args, stash);
        bool matches = clog_stashed_event_matches(
                cork_array_at(&stash->events, i), args);
        va_end(args);
        if (matches) {
            return true;
        }
    }
    return false;
}

struct clog_stashing_handler {
    struct clog_handler parent;
    struct clog_stash* stash;
    struct clog_stashed_event* pending_event;
    struct cork_buffer buf;
};

static void
clog_stashing_handler_ensure_pending_event(struct clog_stashing_handler* self)
{
    if (self->pending_event == NULL) {
        self->pending_event = clog_stashed_event_new();
    }
}

static void
clog_stashing_handler_annotation(struct clog_handler* handler,
                                 struct clog_message* msg, const char* key,
                                 const char* value)
{
    struct clog_stashing_handler* self =
            cork_container_of(handler, struct clog_stashing_handler, parent);
    clog_stashing_handler_ensure_pending_event(self);
    clog_stashed_event_add(self->pending_event, key, value);
    clog_handler_annotation(handler->next, msg, key, value);
}

static void
clog_stashing_handler_message(struct clog_handler* handler,
                              struct clog_message* msg)
{
    struct clog_stashing_handler* self =
            cork_container_of(handler, struct clog_stashing_handler, parent);
    va_list args;
    va_copy(args, msg->args);
    cork_buffer_vprintf(&self->buf, msg->format, args);
    va_end(args);
    clog_stashed_event_add(self->pending_event, "__message", self->buf.buf);
    cork_array_append(&self->stash->events, self->pending_event);
    self->pending_event = NULL;
    clog_handler_message(handler->next, msg);
}

static void
clog_stashing_handler_free(struct clog_handler *vself)
{
    struct clog_stashing_handler* self =
            cork_container_of(vself, struct clog_stashing_handler, parent);
    if (self->pending_event != NULL) {
        clog_stashed_event_free(self->pending_event);
    }
    cork_buffer_done(&self->buf);
    cork_delete(struct clog_stashing_handler, self);
}

struct clog_handler*
clog_stashing_handler_new(struct clog_stash* stash)
{
    struct clog_stashing_handler* self =
            cork_new(struct clog_stashing_handler);
    self->parent.annotation = clog_stashing_handler_annotation;
    self->parent.message = clog_stashing_handler_message;
    self->parent.free = clog_stashing_handler_free;
    self->stash = stash;
    self->pending_event = NULL;
    cork_buffer_init(&self->buf);
    return &self->parent;
}
