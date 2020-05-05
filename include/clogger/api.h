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

#include <stdarg.h>
#include <stdbool.h>

#include <libcork/core.h>


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


struct clog_message {
    enum clog_level  level;
    const char  *channel;
    const char  *format;
    va_list  args;
};


struct clog_handler {
    int (*annotation)(struct clog_handler* handler, struct clog_message* msg,
                      const char* key, const char* value);

    int (*message)(struct clog_handler* handler, struct clog_message* msg);

    void (*free)(struct clog_handler* handler);

    struct clog_handler* next;
};

CORK_INLINE
int
clog_handler_annotation(struct clog_handler* handler, struct clog_message* msg,
                        const char* key, const char* value)
{
    if (handler == NULL) {
        return 0;
    }
    return handler->annotation(handler, msg, key, value);
}

CORK_INLINE
int
clog_handler_message(struct clog_handler* handler, struct clog_message* msg)
{
    if (handler == NULL) {
        return 0;
    }
    return handler->message(handler, msg);
}

CORK_INLINE
void
clog_handler_free(struct clog_handler* handler)
{
    handler->free(handler);
}


void
clog_handler_push_process(struct clog_handler *handler);

int
clog_handler_pop_process(struct clog_handler *handler);

void
clog_handler_push_thread(struct clog_handler *handler);

int
clog_handler_pop_thread(struct clog_handler *handler);


int
clog_process_message(struct clog_message* msg);

int
clog_annotate_message(struct clog_message* msg, const char* key,
                      const char* value);

void
_clog_init_message(struct clog_message* msg, enum clog_level level,
                   const char* channel);

void
_clog_finish_message(struct clog_message* msg, const char* format, ...)
        CORK_ATTR_PRINTF(2, 3);

#define clog_log_channel(level, channel, ...)                                  \
    clog_event_channel((level), (channel), __VA_ARGS__) {}

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
#define clog_event_channel(level, channel, ...)                                \
    for (bool __continue = true; __continue; )                                 \
    for (enum clog_level __level = (level); __continue; )                      \
    for (__continue = (__level <= clog_minimum_level); __continue; )           \
    for (const char* __channel = (channel); __continue; )                      \
    for (struct clog_message __msg; __continue; )                              \
    for (_clog_init_message(&__msg, __level, __channel); __continue; )         \
    for (int __i = 0; __i < 2; __i++)                                          \
    if (__i == 1) {                                                            \
        _clog_finish_message(&__msg, __VA_ARGS__);                             \
        __continue = false;                                                    \
    } else
/* clang-format on */

#define clog_pending_event() (&__msg)

#define clog_event(level, ...)                                                 \
    clog_event_channel((level), CLOG_CHANNEL, __VA_ARGS__)

#define cloge_critical(...) clog_event(CLOG_LEVEL_CRITICAL, __VA_ARGS__)
#define cloge_error(...) clog_event(CLOG_LEVEL_ERROR, __VA_ARGS__)
#define cloge_warning(...) clog_event(CLOG_LEVEL_WARNING, __VA_ARGS__)
#define cloge_notice(...) clog_event(CLOG_LEVEL_NOTICE, __VA_ARGS__)
#define cloge_info(...) clog_event(CLOG_LEVEL_INFO, __VA_ARGS__)
#define cloge_debug(...) clog_event(CLOG_LEVEL_DEBUG, __VA_ARGS__)
#define cloge_trace(...) clog_event(CLOG_LEVEL_TRACE, __VA_ARGS__)

extern enum clog_level  clog_minimum_level;

void
clog_set_minimum_level(enum clog_level level);


#endif /* CLOGGER_API_H */
