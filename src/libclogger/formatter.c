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
#include <string.h>

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/helpers/errors.h>

#include "clogger/api.h"
#include "clogger/error.h"
#include "clogger/formatter.h"


/*-----------------------------------------------------------------------
 * Segment interface
 */

struct segment {
    int
    (*start)(struct segment *segment);

    int
    (*annotation)(struct segment *segment, const char *key, const char *value);

    int
    (*message)(struct segment *segment, struct clog_message *msg);

    int
    (*append)(struct segment *segment, struct cork_buffer *dest);

    void
    (*free)(struct segment *segment);
};

typedef cork_array(struct segment *)  segment_array;


/*-----------------------------------------------------------------------
 * Formatter type
 */

struct clog_formatter {
    segment_array  segments;
};


/*-----------------------------------------------------------------------
 * Raw segment
 */

struct raw_segment {
    struct segment  parent;
    struct cork_buffer  content;
};

static int
raw_segment__start(struct segment *vself)
{
    return 0;
}

static int
raw_segment__annotation(struct segment *vself, const char *key,
                        const char *value)
{
    return 0;
}

static int
raw_segment__message(struct segment *vself, struct clog_message *msg)
{
    return 0;
}

static int
raw_segment__append(struct segment *vself, struct cork_buffer *dest)
{
    struct raw_segment  *self =
        cork_container_of(vself, struct raw_segment, parent);
    cork_buffer_append(dest, self->content.buf, self->content.size);
    return 0;
}

static void
raw_segment__free(struct segment *vself)
{
    struct raw_segment  *self =
        cork_container_of(vself, struct raw_segment, parent);
    cork_buffer_done(&self->content);
    free(self);
}

static int
raw_segment_new(struct clog_formatter *fmt, const char *content, size_t size)
{
    struct raw_segment  *self = cork_new(struct raw_segment);
    self->parent.start = raw_segment__start;
    self->parent.annotation = raw_segment__annotation;
    self->parent.message = raw_segment__message;
    self->parent.append = raw_segment__append;
    self->parent.free = raw_segment__free;
    cork_buffer_init(&self->content);
    cork_buffer_set(&self->content, content, size);
    /*printf("RAW \"%.*s\"\n", (int) size, content);*/
    cork_array_append(&fmt->segments, &self->parent);
    return 0;
}


/*-----------------------------------------------------------------------
 * Message part
 */

enum msg_part {
    MSG_LEVEL,
    MSG_LEVEL_FIXED,
    MSG_CHANNEL,
    MSG_MESSAGE
};

struct msg_segment {
    struct segment  parent;
    enum msg_part  part;
    struct cork_buffer  value;
};

static int
msg_segment__start(struct segment *vself)
{
    return 0;
}

static int
msg_segment__annotation(struct segment *vself, const char *key,
                        const char *value)
{
    return 0;
}

static int
msg_segment__message(struct segment *vself, struct clog_message *msg)
{
    struct msg_segment  *self =
        cork_container_of(vself, struct msg_segment, parent);
    switch (self->part) {
        case MSG_LEVEL:
            cork_buffer_set_string(&self->value, clog_level_name(msg->level));
            break;

        case MSG_LEVEL_FIXED:
            cork_buffer_set_string
                (&self->value, clog_level_name_fixed_width(msg->level));
            break;

        case MSG_CHANNEL:
            cork_buffer_set_string(&self->value, msg->channel);
            break;

        case MSG_MESSAGE:
        {
            va_list  args;
            va_copy(args, msg->args);
            cork_buffer_vprintf(&self->value, msg->format, args);
            va_end(args);
            break;
        }

        default:
            cork_unreachable();
    }
    return 0;
}

static int
msg_segment__append(struct segment *vself, struct cork_buffer *dest)
{
    struct msg_segment  *self =
        cork_container_of(vself, struct msg_segment, parent);
    cork_buffer_append(dest, self->value.buf, self->value.size);
    return 0;
}

static void
msg_segment__free(struct segment *vself)
{
    struct msg_segment  *self =
        cork_container_of(vself, struct msg_segment, parent);
    cork_buffer_done(&self->value);
    free(self);
}

static int
msg_segment_new(struct clog_formatter *fmt, enum msg_part part)
{
    struct msg_segment  *self = cork_new(struct msg_segment);
    self->parent.start = msg_segment__start;
    self->parent.annotation = msg_segment__annotation;
    self->parent.message = msg_segment__message;
    self->parent.append = msg_segment__append;
    self->parent.free = msg_segment__free;
    self->part = part;
    cork_buffer_init(&self->value);
#if 0
    printf("MSG %s\n",
           part == MSG_LEVEL? "level":
           part == MSG_LEVEL_FIXED? "fixed level":
           part == MSG_CHANNEL? "channel":
           part == MSG_MESSAGE? "message": "UNKNOWN");
#endif
    cork_array_append(&fmt->segments, &self->parent);
    return 0;
}


/*-----------------------------------------------------------------------
 * Variable reference
 */

struct var_segment {
    struct segment  parent;
    const char  *name;
    struct cork_buffer  value;
    bool  value_given;
};

static int
var_segment__start(struct segment *vself)
{
    struct var_segment  *self =
        cork_container_of(vself, struct var_segment, parent);
    self->value_given = false;
    return 0;
}

static int
var_segment__annotation(struct segment *vself, const char *key,
                        const char *value)
{
    struct var_segment  *self =
        cork_container_of(vself, struct var_segment, parent);
    if (strcmp(key, self->name) == 0) {
        self->value_given = true;
        cork_buffer_set_string(&self->value, value);
    }
    return 0;
}

static int
var_segment__message(struct segment *vself, struct clog_message *msg)
{
    return 0;
}

static int
var_segment__append(struct segment *vself, struct cork_buffer *dest)
{
    struct var_segment  *self =
        cork_container_of(vself, struct var_segment, parent);
    if (self->value_given) {
        cork_buffer_append(dest, self->value.buf, self->value.size);
    }
    return 0;
}

static void
var_segment__free(struct segment *vself)
{
    struct var_segment  *self =
        cork_container_of(vself, struct var_segment, parent);
    cork_strfree(self->name);
    cork_buffer_done(&self->value);
    free(self);
}

static int
var_segment_new(struct clog_formatter *fmt, const char *name, size_t size)
{
    struct var_segment  *self = cork_new(struct var_segment);
    self->parent.start = var_segment__start;
    self->parent.annotation = var_segment__annotation;
    self->parent.message = var_segment__message;
    self->parent.append = var_segment__append;
    self->parent.free = var_segment__free;
    cork_buffer_init(&self->value);
    cork_buffer_set(&self->value, name, size);
    /*printf("VAR \"%.*s\"\n", (int) size, name);*/
    self->name = cork_strdup(self->value.buf);
    cork_array_append(&fmt->segments, &self->parent);
    return 0;
}


/*-----------------------------------------------------------------------
 * Format string
 */

static int
clog_format_string_parse(struct clog_formatter *self, const char *fmt)
{
    const char  *curr = fmt;

    while (curr[0] != '\0') {
        /* curr is the start of the next segment. */

        if (curr[0] == '#') {
            if (curr[1] == '{') {
                /* We've got a variable reference.  Look for the closing "}" */
                const char  *v_end = curr + 2;
                while (v_end[0] != '}') {
                    if (v_end[0] == '\0') {
                        /* Unterminated variable reference */
                        clog_bad_format("Unterminated variable reference");
                        return -1;
                    }
                    v_end++;
                }
                rii_check(var_segment_new(self, curr + 2, v_end - curr - 2));
                curr = v_end + 1;
            } else if (curr[1] == '#') {
                rii_check(raw_segment_new(self, "#", 1));
                curr += 2;
            } else {
                /* Unknown "#" conversion */
                clog_bad_format("Unknown conversion #%c", curr[1]);
                return -1;
            }
        } else if (curr[0] == '%') {
            if (curr[1] == '%') {
                rii_check(raw_segment_new(self, "%", 1));
                curr += 2;
            } else if (curr[1] == 'l') {
                rii_check(msg_segment_new(self, MSG_LEVEL));
                curr += 2;
            } else if (curr[1] == 'L') {
                rii_check(msg_segment_new(self, MSG_LEVEL_FIXED));
                curr += 2;
            } else if (curr[1] == 'c') {
                rii_check(msg_segment_new(self, MSG_CHANNEL));
                curr += 2;
            } else if (curr[1] == 'm') {
                rii_check(msg_segment_new(self, MSG_MESSAGE));
                curr += 2;
            } else {
                /* Unknown "%" conversion */
                clog_bad_format("Unknown conversion %%%c", curr[1]);
                return -1;
            }
        } else {
            /* We've got a raw segment.  It proceeds until the end of the string
             * or the first occurrence of "#" or "%" */
            const char  *s_end = curr + 1;
            while (s_end[0] != '\0' && s_end[0] != '#' && s_end[0] != '%') {
                s_end++;
            }
            rii_check(raw_segment_new(self, curr, s_end - curr));
            curr = s_end;
        }
    }

    return 0;
}


/*-----------------------------------------------------------------------
 * Formatters
 */

void
clog_formatter_free(struct clog_formatter *self)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment  *segment = cork_array_at(&self->segments, i);
        segment->free(segment);
    }
    cork_array_done(&self->segments);
    free(self);
}

struct clog_formatter *
clog_formatter_new(const char *fmt)
{
    struct clog_formatter  *self = cork_new(struct clog_formatter);
    cork_array_init(&self->segments);
    ei_check(clog_format_string_parse(self, fmt));
    return self;

error:
    clog_formatter_free(self);
    return NULL;
}

int
clog_formatter_start(struct clog_formatter *self)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment  *segment = cork_array_at(&self->segments, i);
        rii_check(segment->start(segment));
    }
    return 0;
}

int
clog_formatter_annotation(struct clog_formatter *self, const char *key,
                          const char *value)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment  *segment = cork_array_at(&self->segments, i);
        rii_check(segment->annotation(segment, key, value));
    }
    return 0;
}

int
clog_formatter_finish(struct clog_formatter *self, struct clog_message *msg,
                      struct cork_buffer *dest)
{
    size_t  i;

    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment  *segment = cork_array_at(&self->segments, i);
        rii_check(segment->message(segment, msg));
    }

    cork_buffer_clear(dest);
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment  *segment = cork_array_at(&self->segments, i);
        rii_check(segment->append(segment, dest));
    }

    return 0;
}
