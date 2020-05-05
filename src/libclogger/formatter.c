/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
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
    void (*start)(struct segment* segment);

    void (*annotation)(struct segment* segment, const char* key,
                       const char* value);

    void (*message)(struct segment* segment, struct clog_message* msg);

    void (*append)(struct segment* segment, struct cork_buffer* dest);

    void (*free)(struct segment* segment);
};

typedef cork_array(struct segment*) segment_array;


/*-----------------------------------------------------------------------
 * Annotation segment interface
 */

struct annotation_segment {
    void (*annotation)(struct annotation_segment* segment,
                       struct cork_buffer* dest, const char* key,
                       const char* value);

    void (*free)(struct annotation_segment* segment);
};

typedef cork_array(struct annotation_segment*) annotation_segment_array;


/*-----------------------------------------------------------------------
 * Formatter type
 */

struct clog_formatter {
    segment_array segments;
};


/*-----------------------------------------------------------------------
 * Raw segment
 */

struct raw_segment {
    struct segment parent;
    struct cork_buffer content;
};

static void
raw_segment_start(struct segment* vself)
{
}

static void
raw_segment_annotation(struct segment* vself, const char* key,
                       const char* value)
{
}

static void
raw_segment_message(struct segment* vself, struct clog_message* msg)
{
}

static void
raw_segment_append(struct segment* vself, struct cork_buffer* dest)
{
    struct raw_segment* self =
            cork_container_of(vself, struct raw_segment, parent);
    cork_buffer_append(dest, self->content.buf, self->content.size);
}

static void
raw_segment_free(struct segment* vself)
{
    struct raw_segment* self =
            cork_container_of(vself, struct raw_segment, parent);
    cork_buffer_done(&self->content);
    cork_delete(struct raw_segment, self);
}

static void
raw_segment_new(struct clog_formatter* fmt, const char* content, size_t size)
{
    struct raw_segment* self = cork_new(struct raw_segment);
    self->parent.start = raw_segment_start;
    self->parent.annotation = raw_segment_annotation;
    self->parent.message = raw_segment_message;
    self->parent.append = raw_segment_append;
    self->parent.free = raw_segment_free;
    cork_buffer_init(&self->content);
    cork_buffer_set(&self->content, content, size);
    /*printf("RAW \"%.*s\"\n", (int) size, content);*/
    cork_array_append(&fmt->segments, &self->parent);
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
    struct segment parent;
    enum msg_part part;
    struct cork_buffer value;
};

static void
msg_segment_start(struct segment* vself)
{
}

static void
msg_segment_annotation(struct segment* vself, const char* key,
                       const char* value)
{
}

static void
msg_segment_message(struct segment* vself, struct clog_message* msg)
{
    struct msg_segment* self =
            cork_container_of(vself, struct msg_segment, parent);
    switch (self->part) {
        case MSG_LEVEL:
            cork_buffer_set_string(&self->value, clog_level_name(msg->level));
            break;

        case MSG_LEVEL_FIXED:
            cork_buffer_set_string(&self->value,
                                   clog_level_name_fixed_width(msg->level));
            break;

        case MSG_CHANNEL:
            cork_buffer_set_string(&self->value, msg->channel);
            break;

        case MSG_MESSAGE: {
            va_list args;
            va_copy(args, msg->args);
            cork_buffer_vprintf(&self->value, msg->format, args);
            va_end(args);
            break;
        }

        default:
            cork_unreachable();
    }
}

static void
msg_segment_append(struct segment* vself, struct cork_buffer* dest)
{
    struct msg_segment* self =
            cork_container_of(vself, struct msg_segment, parent);
    cork_buffer_append(dest, self->value.buf, self->value.size);
}

static void
msg_segment_free(struct segment* vself)
{
    struct msg_segment* self =
            cork_container_of(vself, struct msg_segment, parent);
    cork_buffer_done(&self->value);
    cork_delete(struct msg_segment, self);
}

static void
msg_segment_new(struct clog_formatter* fmt, enum msg_part part)
{
    struct msg_segment* self = cork_new(struct msg_segment);
    self->parent.start = msg_segment_start;
    self->parent.annotation = msg_segment_annotation;
    self->parent.message = msg_segment_message;
    self->parent.append = msg_segment_append;
    self->parent.free = msg_segment_free;
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
}


/*-----------------------------------------------------------------------
 * Raw segment
 */

struct raw_annotation_segment {
    struct annotation_segment parent;
    struct cork_buffer content;
};

static void
raw_annotation_segment_annotation(struct annotation_segment* vself,
                                  struct cork_buffer* dest, const char* key,
                                  const char* value)
{
    struct raw_annotation_segment* self =
            cork_container_of(vself, struct raw_annotation_segment, parent);
    cork_buffer_append(dest, self->content.buf, self->content.size);
}

static void
raw_annotation_segment_free(struct annotation_segment* vself)
{
    struct raw_annotation_segment* self =
            cork_container_of(vself, struct raw_annotation_segment, parent);
    cork_buffer_done(&self->content);
    cork_delete(struct raw_annotation_segment, self);
}

static void
raw_annotation_segment_new(annotation_segment_array* arr, const char* content,
                           size_t size)
{
    struct raw_annotation_segment* self =
            cork_new(struct raw_annotation_segment);
    self->parent.annotation = raw_annotation_segment_annotation;
    self->parent.free = raw_annotation_segment_free;
    cork_buffer_init(&self->content);
    cork_buffer_set(&self->content, content, size);
    /*printf("  RAW \"%.*s\"\n", (int) size, content);*/
    cork_array_append(arr, &self->parent);
}


/*-----------------------------------------------------------------------
 * Annotation key sub-segment
 */

static void
key_segment_annotation(struct annotation_segment* self,
                       struct cork_buffer* dest, const char* key,
                       const char* value)
{
    cork_buffer_append_string(dest, key);
}

static void
key_segment_free(struct annotation_segment* self)
{
    cork_delete(struct annotation_segment, self);
}

static void
key_segment_new(annotation_segment_array *arr)
{
    struct annotation_segment* self = cork_new(struct annotation_segment);
    self->annotation = key_segment_annotation;
    self->free = key_segment_free;
    /*printf("  KEY\n");*/
    cork_array_append(arr, self);
}


/*-----------------------------------------------------------------------
 * Annotation value sub-segment
 */

static void
value_segment_annotation(struct annotation_segment* self,
                         struct cork_buffer* dest, const char* key,
                         const char* value)
{
    cork_buffer_append_string(dest, value);
}

static void
value_segment_free(struct annotation_segment* self)
{
    cork_delete(struct annotation_segment, self);
}

static void
value_segment_new(annotation_segment_array* arr)
{
    struct annotation_segment* self = cork_new(struct annotation_segment);
    self->annotation = value_segment_annotation;
    self->free = value_segment_free;
    /*printf("  VALUE\n");*/
    cork_array_append(arr, self);
}


/*-----------------------------------------------------------------------
 * Variable reference
 */

struct var_segment {
    struct segment parent;
    const char* name;
    const char* default_value;
    struct cork_buffer value;
    annotation_segment_array segments;
    bool value_given;
};

static void
var_segment_start(struct segment* vself)
{
    struct var_segment* self =
            cork_container_of(vself, struct var_segment, parent);
    self->value_given = false;
    cork_buffer_clear(&self->value);
}

static void
var_segment_annotation(struct segment* vself, const char* key,
                       const char* value)
{
    struct var_segment* self =
            cork_container_of(vself, struct var_segment, parent);
    if (strcmp(key, self->name) == 0) {
        size_t i;
        self->value_given = true;
        for (i = 0; i < cork_array_size(&self->segments); i++) {
            struct annotation_segment* segment =
                    cork_array_at(&self->segments, i);
            segment->annotation(segment, &self->value, key, value);
        }
    }
}

static void
var_segment_message(struct segment* vself, struct clog_message* msg)
{
}

static void
var_segment_append(struct segment* vself, struct cork_buffer* dest)
{
    struct var_segment* self =
            cork_container_of(vself, struct var_segment, parent);
    if (self->value_given) {
        cork_buffer_append(dest, self->value.buf, self->value.size);
    } else {
        cork_buffer_append_string(dest, self->default_value);
    }
}

static void
var_segment_free(struct segment* vself)
{
    size_t i;
    struct var_segment* self =
            cork_container_of(vself, struct var_segment, parent);

    cork_strfree(self->name);
    cork_strfree(self->default_value);
    cork_buffer_done(&self->value);
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct annotation_segment* segment = cork_array_at(&self->segments, i);
        segment->free(segment);
    }
    cork_array_done(&self->segments);
    cork_delete(struct var_segment, self);
}

static struct var_segment*
var_segment_new(struct clog_formatter* fmt, const char* default_value,
                size_t default_size, const char* name, size_t name_size)
{
    struct var_segment* self = cork_new(struct var_segment);
    self->parent.start = var_segment_start;
    self->parent.annotation = var_segment_annotation;
    self->parent.message = var_segment_message;
    self->parent.append = var_segment_append;
    self->parent.free = var_segment_free;
    cork_array_init(&self->segments);
    cork_buffer_init(&self->value);
    cork_buffer_set(&self->value, name, name_size);
    self->name = cork_strdup(self->value.buf);
    cork_buffer_set(&self->value, default_value, default_size);
    self->default_value = cork_strdup(self->value.buf);
#if 0
    printf("VAR \"%s\" \"%s\"\n", self->name, self->default_value);
#endif
    cork_array_append(&fmt->segments, &self->parent);
    return self;
}


/*-----------------------------------------------------------------------
 * Multiple variable reference
 */

struct multi_segment {
    struct segment parent;
    const char* default_value;
    struct cork_buffer value;
    annotation_segment_array segments;
    bool value_given;
};

static void
multi_segment_start(struct segment* vself)
{
    struct multi_segment* self =
            cork_container_of(vself, struct multi_segment, parent);
    self->value_given = false;
    cork_buffer_clear(&self->value);
}

static void
multi_segment_annotation(struct segment* vself, const char* key,
                         const char* value)
{
    size_t i;
    struct multi_segment* self =
            cork_container_of(vself, struct multi_segment, parent);

    self->value_given = true;
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct annotation_segment* segment = cork_array_at(&self->segments, i);
        segment->annotation(segment, &self->value, key, value);
    }
}

static void
multi_segment_message(struct segment* vself, struct clog_message* msg)
{
}

static void
multi_segment_append(struct segment* vself, struct cork_buffer* dest)
{
    struct multi_segment* self =
            cork_container_of(vself, struct multi_segment, parent);
    if (self->value_given) {
        cork_buffer_append(dest, self->value.buf, self->value.size);
    } else {
        cork_buffer_append_string(dest, self->default_value);
    }
}

static void
multi_segment_free(struct segment* vself)
{
    size_t i;
    struct multi_segment* self =
            cork_container_of(vself, struct multi_segment, parent);

    cork_strfree(self->default_value);
    cork_buffer_done(&self->value);
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct annotation_segment* segment = cork_array_at(&self->segments, i);
        segment->free(segment);
    }
    cork_array_done(&self->segments);
    cork_delete(struct multi_segment, self);
}

static struct multi_segment*
multi_segment_new(struct clog_formatter* fmt, const char* default_value,
                  size_t default_size)
{
    struct multi_segment* self = cork_new(struct multi_segment);
    self->parent.start = multi_segment_start;
    self->parent.annotation = multi_segment_annotation;
    self->parent.message = multi_segment_message;
    self->parent.append = multi_segment_append;
    self->parent.free = multi_segment_free;
    cork_array_init(&self->segments);
    cork_buffer_init(&self->value);
    cork_buffer_set(&self->value, default_value, default_size);
    self->default_value = cork_strdup(self->value.buf);
#if 0
    printf("MLT \"%s\"\n", self->default_value);
#endif
    cork_array_append(&fmt->segments, &self->parent);
    return self;
}


/*-----------------------------------------------------------------------
 * Format string
 */

static const char*
annotation_spec_parse(annotation_segment_array* arr, const char* fmt)
{
    const char* curr = fmt;

    while (curr[0] != '}') {
        /* curr is the start of the next segment. */

        if (curr[0] == '\0') {
            /* Unterminated annotation spec */
            clog_bad_format("Unterminated annotation spec");
            return NULL;
        } else if (curr[0] == '%') {
            if (curr[1] == '%') {
                raw_annotation_segment_new(arr, "%", 1);
                curr += 2;
            } else if (curr[1] == 'k') {
                key_segment_new(arr);
                curr += 2;
            } else if (curr[1] == 'v') {
                value_segment_new(arr);
                curr += 2;
            } else {
                /* Unknown "%" conversion */
                clog_bad_format("Unknown conversion %%%c", curr[1]);
                return NULL;
            }
        } else {
            /* We've got a raw segment.  It proceeds until the end of the string
             * or the first occurrence of "%" or "}" */
            const char* s_end = curr + 1;
            while (s_end[0] != '\0' && s_end[0] != '%' && s_end[0] != '}') {
                s_end++;
            }
            raw_annotation_segment_new(arr, curr, s_end - curr);
            curr = s_end;
        }
    }

    return curr + 1;
}

static int
format_string_parse(struct clog_formatter* self, const char* fmt)
{
    const char* curr = fmt;

    while (curr[0] != '\0') {
        /* curr is the start of the next segment. */

        if (curr[0] == '#') {
            if (curr[1] == '{') {
                struct var_segment* segment;
                /* We've got a variable reference.  Look for the closing "}" */
                const char* v_end = curr + 2;
                while (v_end[0] != '}') {
                    if (v_end[0] == '\0') {
                        /* Unterminated variable reference */
                        clog_bad_format("Unterminated variable reference");
                        return -1;
                    }
                    v_end++;
                }
                segment = var_segment_new(self, "", 0, curr + 2,
                                          v_end - curr - 2);
                value_segment_new(&segment->segments);
                curr = v_end + 1;
            } else if (curr[1] == '!') {
                if (curr[2] == '{') {
                    struct var_segment* segment;

                    /* Another variable reference.  Look for the closing "}" */
                    const char* v_end = curr + 3;
                    while (v_end[0] != '}') {
                        if (v_end[0] == '\0') {
                            /* Unterminated variable reference */
                            clog_bad_format("Unterminated variable reference");
                            return -1;
                        }
                        v_end++;
                    }

                    /* v_end points at a } */
                    if (v_end[1] != '{') {
                        clog_bad_format("Expected { in #! conversion");
                        return -1;
                    }

                    /* Create the variable segment */
                    segment = var_segment_new(self, "", 0, curr + 3,
                                              v_end - curr - 3);

                    /* Parse the annotation spec */
                    rip_check(curr = annotation_spec_parse(&segment->segments,
                                                           v_end + 2));
                } else {
                    clog_bad_format("Expected { in #! conversion");
                    return -1;
                }
            } else if (curr[1] == '*') {
                if (curr[2] == '{') {
                    struct multi_segment* segment;

                    /* Create the multi-variable segment */
                    segment = multi_segment_new(self, "", 0);

                    /* Parse the annotation spec */
                    rip_check(curr = annotation_spec_parse(&segment->segments,
                                                           curr + 3));
                } else {
                    clog_bad_format("Expected { in #! conversion");
                    return -1;
                }
            } else if (curr[1] == '#') {
                raw_segment_new(self, "#", 1);
                curr += 2;
            } else {
                /* Unknown "#" conversion */
                clog_bad_format("Unknown conversion #%c", curr[1]);
                return -1;
            }
        } else if (curr[0] == '%') {
            if (curr[1] == '%') {
                raw_segment_new(self, "%", 1);
                curr += 2;
            } else if (curr[1] == 'l') {
                msg_segment_new(self, MSG_LEVEL);
                curr += 2;
            } else if (curr[1] == 'L') {
                msg_segment_new(self, MSG_LEVEL_FIXED);
                curr += 2;
            } else if (curr[1] == 'c') {
                msg_segment_new(self, MSG_CHANNEL);
                curr += 2;
            } else if (curr[1] == 'm') {
                msg_segment_new(self, MSG_MESSAGE);
                curr += 2;
            } else {
                /* Unknown "%" conversion */
                clog_bad_format("Unknown conversion %%%c", curr[1]);
                return -1;
            }
        } else {
            /* We've got a raw segment.  It proceeds until the end of the string
             * or the first occurrence of "#" or "%" */
            const char* s_end = curr + 1;
            while (s_end[0] != '\0' && s_end[0] != '#' && s_end[0] != '%') {
                s_end++;
            }
            raw_segment_new(self, curr, s_end - curr);
            curr = s_end;
        }
    }

    return 0;
}


/*-----------------------------------------------------------------------
 * Formatters
 */

void
clog_formatter_free(struct clog_formatter* self)
{
    size_t i;
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment* segment = cork_array_at(&self->segments, i);
        segment->free(segment);
    }
    cork_array_done(&self->segments);
    cork_delete(struct clog_formatter, self);
}

struct clog_formatter*
clog_formatter_new(const char* fmt)
{
    struct clog_formatter* self = cork_new(struct clog_formatter);
    cork_array_init(&self->segments);
    ei_check(format_string_parse(self, fmt));
    return self;

error:
    clog_formatter_free(self);
    return NULL;
}

void
clog_formatter_start(struct clog_formatter* self)
{
    size_t i;
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment* segment = cork_array_at(&self->segments, i);
        segment->start(segment);
    }
}

void
clog_formatter_annotation(struct clog_formatter* self, const char* key,
                          const char* value)
{
    size_t i;
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment* segment = cork_array_at(&self->segments, i);
        segment->annotation(segment, key, value);
    }
}

void
clog_formatter_finish(struct clog_formatter* self, struct clog_message* msg,
                      struct cork_buffer* dest)
{
    size_t i;

    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment* segment = cork_array_at(&self->segments, i);
        segment->message(segment, msg);
    }

    cork_buffer_clear(dest);
    for (i = 0; i < cork_array_size(&self->segments); i++) {
        struct segment* segment = cork_array_at(&self->segments, i);
        segment->append(segment, dest);
    }
}
