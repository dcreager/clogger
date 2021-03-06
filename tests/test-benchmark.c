/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>

#include <libcork/os.h>

#include "clogger/api.h"
#include "clogger/fields.h"
#include "clogger/handlers.h"

#define CLOG_CHANNEL "benchmark"
#define DEFAULT_FORMAT "[%L] %c:#*{ %k=%v} %m"

static size_t
parse_size(const char* str)
{
    char* endstr;
    unsigned long value = strtoul((str == NULL) ? "10" : str, &endstr, 10);
    if (*endstr == '\0') {
        return value;
    } else {
        return 10000;
    }
}

int
main(int argc, const char** argv)
{
    size_t iteration_count = parse_size(cork_env_get(NULL, "ITERATIONS"));

    struct clog_handler* handler = clog_null_handler_new();
    clog_handler_push_process(handler);
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);

    printf("1..1\n");
    printf("==== Generating %zu log messages\n", iteration_count);
    for (size_t i = 0; i < iteration_count; i++) {
        cloge_debug {
            clog_add_field(field1, string, "value");
            clog_add_field(field2, string, "another value");
            clog_add_field(field3, string, "yet another value");
            clog_set_message("Interesting %s things are%s happening",
                             clog_field_value(field1),
                             ((i % 2) == 0) ? "" : " not");
        }
    }
    printf("ok 1\n");

    return EXIT_SUCCESS;
}
