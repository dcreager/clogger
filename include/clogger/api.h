/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef CLOGGER_API_H
#define CLOGGER_API_H

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>

#include <libcork/core.h>
#include <libcork/ds.h>


/*-----------------------------------------------------------------------
 * Log levels
 */

enum clog_level {
    CLOG_LEVEL_NONE = 0,
    CLOG_LEVEL_CRITICAL,
    CLOG_LEVEL_ERROR,
    CLOG_LEVEL_WARNING,
    CLOG_LEVEL_NOTICE,
    CLOG_LEVEL_INFO,
    CLOG_LEVEL_DEBUG,
    CLOG_LEVEL_TRACE
};

CORK_ATTR_PURE
const char *
clog_level_name(enum clog_level level);

CORK_ATTR_PURE
const char *
clog_level_name_fixed_width(enum clog_level level);

extern enum clog_level  clog_minimum_level;

void
clog_set_minimum_level(enum clog_level level);


/*-----------------------------------------------------------------------
 * Handler interface
 */

struct clog_message_field {
    const char* key;
    const char* value;
    void (*done)(struct clog_message_field* field);
    struct clog_message_field* next;
};

CORK_INLINE
void
clog_message_field_done(struct clog_message_field* field)
{
    if (field->done != NULL) {
        field->done(field);
    }
}

struct clog_message_fields {
    struct clog_message_field* head;
};

CORK_INLINE
void
clog_message_fields_init(struct clog_message_fields* fields)
{
    fields->head = NULL;
}

CORK_INLINE
void
clog_message_fields_push(struct clog_message_fields* fields,
                         struct clog_message_field* field)
{
    field->next = fields->head;
    fields->head = field;
}

CORK_INLINE
void
clog_message_fields_pop(struct clog_message_fields* fields,
                        struct clog_message_field* field)
{
    assert(fields->head == field);
    fields->head = field->next;
    clog_message_field_done(field);
}

CORK_INLINE
void
clog_message_fields_done(struct clog_message_fields* fields)
{
    struct clog_message_field* current;
    struct clog_message_field* next;
    for (current = fields->head; current != NULL; current = next) {
        next = current->next;
        clog_message_field_done(current);
    }
}


struct clog_message {
    enum clog_level level;
    const char* channel;
    struct clog_message_fields fields;
    struct cork_buffer message;
    const char* fmt;
    va_list args;
};

CORK_INLINE
void
clog_message_init(struct clog_message* message, enum clog_level level,
                  const char* channel)
{
    message->level = level;
    message->channel = channel;
    clog_message_fields_init(&message->fields);
    cork_buffer_init(&message->message);
}

CORK_INLINE
void
clog_message_pop_field(struct clog_message* message,
                       struct clog_message_field* field)
{
    clog_message_fields_pop(&message->fields, field);
}

CORK_INLINE
void
clog_message_done(struct clog_message* message)
{
    clog_message_fields_done(&message->fields);
    cork_buffer_done(&message->message);
}

const char*
clog_message_message(struct clog_message* message);

struct clog_handler {
    void (*handle)(struct clog_handler* handler, struct clog_message* message);
    void (*free)(struct clog_handler* handler);
    struct clog_handler* next;
};

CORK_INLINE
void
clog_handler_handle(struct clog_handler* handler, struct clog_message* message)
{
    handler->handle(handler, message);
}

CORK_INLINE
void
clog_handler_free(struct clog_handler* handler)
{
    handler->free(handler);
}


/*-----------------------------------------------------------------------
 * Handler stacks
 */

void
clog_handler_push_process(struct clog_handler *handler);

int
clog_handler_pop_process(struct clog_handler *handler);

void
clog_handler_push_thread(struct clog_handler *handler);

int
clog_handler_pop_thread(struct clog_handler *handler);


/*-----------------------------------------------------------------------
 * Processing messages
 */

#define clog_log_channel(level, channel, ...)                                  \
    clog_event_channel((level), (channel)) { clog_set_message(__VA_ARGS__); }

#define clog_log(level, ...) \
    clog_log_channel((level), CLOG_CHANNEL, __VA_ARGS__)

#define clog_critical(...)  clog_log(CLOG_LEVEL_CRITICAL, __VA_ARGS__)
#define clog_error(...)     clog_log(CLOG_LEVEL_ERROR, __VA_ARGS__)
#define clog_warning(...)   clog_log(CLOG_LEVEL_WARNING, __VA_ARGS__)
#define clog_notice(...)    clog_log(CLOG_LEVEL_NOTICE, __VA_ARGS__)
#define clog_info(...)      clog_log(CLOG_LEVEL_INFO, __VA_ARGS__)
#define clog_debug(...)     clog_log(CLOG_LEVEL_DEBUG, __VA_ARGS__)
#define clog_trace(...)     clog_log(CLOG_LEVEL_TRACE, __VA_ARGS__)

#define clog_channel_critical(ch, ...) \
    clog_log_channel(CLOG_LEVEL_CRITICAL, (ch), __VA_ARGS__)
#define clog_channel_error(ch, ...) \
    clog_log_channel(CLOG_LEVEL_ERROR, (ch), __VA_ARGS__)
#define clog_channel_warning(ch, ...) \
    clog_log_channel(CLOG_LEVEL_WARNING, (ch), __VA_ARGS__)
#define clog_channel_notice(ch, ...) \
    clog_log_channel(CLOG_LEVEL_NOTICE, (ch), __VA_ARGS__)
#define clog_channel_info(ch, ...) \
    clog_log_channel(CLOG_LEVEL_INFO, (ch), __VA_ARGS__)
#define clog_channel_debug(ch, ...) \
    clog_log_channel(CLOG_LEVEL_DEBUG, (ch), __VA_ARGS__)
#define clog_channel_trace(ch, ...) \
    clog_log_channel(CLOG_LEVEL_TRACE, (ch), __VA_ARGS__)

/* This is pretty gross!  It's based on a technique described in [1].  The goal
 * is to take
 *
 *     clog_event_channel(level, channel, fmt, args) {
 *        fields
 *     }
 *
 * and turn it into something equivalent to
 *
 *     if (level <= clog_minimum_level) {
 *         struct clog_message msg;
 *         _clog_init_message(&msg, level, channel, fmt, args);
 *         fields;
 *         _clog_finish_message(&msg);
 *     }
 *
 * [1] https://stackoverflow.com/questions/866012/is-there-a-way-to-define-variables-of-two-different-types-in-a-for-loop-initiali */
/* clang-format off */
#define clog_event_channel(level, channel)                                     \
    for (bool __continue = true; __continue; )                                 \
    for (enum clog_level __level = (level); __continue; )                      \
    for (__continue = (__level <= clog_minimum_level); __continue; )           \
    for (const char* __channel = (channel); __continue; )                      \
    for (struct clog_message __message; __continue; )                          \
    for (clog_message_init(&__message, __level, __channel); __continue; )      \
    for (CORK_ATTR_UNUSED struct clog_message_fields* __fields =               \
            &__message.fields; __continue;)                                    \
    for (; __continue; __continue = false)                                     \
/* clang-format on */

#define clog_add_field(field_name, field_type, ...)                            \
    struct clog_##field_type##_field __##field_name##_field;                   \
    clog_message_add_##field_type##_field(__fields, &__##field_name##_field,   \
                                          #field_name, __VA_ARGS__);

#define clog_field_value(field_name) (__##field_name##_field.parent.value)

#define clog_set_message(...)                                                  \
    do {                                                                       \
        _clog_process_message(&__message, __VA_ARGS__);                        \
    } while (0)

#define clog_event(level) clog_event_channel((level), CLOG_CHANNEL)
#define cloge_critical clog_event(CLOG_LEVEL_CRITICAL)
#define cloge_error clog_event(CLOG_LEVEL_ERROR)
#define cloge_warning clog_event(CLOG_LEVEL_WARNING)
#define cloge_notice clog_event(CLOG_LEVEL_NOTICE)
#define cloge_info clog_event(CLOG_LEVEL_INFO)
#define cloge_debug clog_event(CLOG_LEVEL_DEBUG)
#define cloge_trace clog_event(CLOG_LEVEL_TRACE)

void
_clog_process_message(struct clog_message* message, const char* fmt, ...)
        CORK_ATTR_PRINTF(2, 3);


/* clang-format off */
#define cloge_message_fields                                                   \
    for (bool __continue = true; __continue;)                                  \
    for (struct clog_message_fields __field_list; __continue;)                 \
    for (clog_message_fields_init(&__field_list); __continue;)                 \
    for (struct clog_message_fields* __fields = &__field_list; __continue;)    \
    for (; __continue; __continue = false)
/* clang-format on */


#endif /* CLOGGER_API_H */
