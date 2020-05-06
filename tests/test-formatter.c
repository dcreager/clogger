/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/helpers/errors.h>

#include "clogger/api.h"
#include "clogger/fields.h"
#include "clogger/formatter.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Format strings
 */

#define GOOD(str) \
    do { \
        struct clog_formatter  *fmt; \
        fail_if_error(fmt = clog_formatter_new(str)); \
        clog_formatter_free(fmt); \
    } while (0)

#define BAD(str) \
    do { \
        fail_unless_error(clog_formatter_new(str)); \
    } while (0)

START_TEST(test_format_parse_01)
{
    DESCRIBE_TEST;
    GOOD("");
    GOOD("##");
    GOOD("%%");
    GOOD("%l %L %c %m");
    GOOD("#{var}");
    GOOD("#!{var}{ %% %k %v}");
    GOOD("#*{ %% %k %v}");
    GOOD("test 1");
    GOOD("test ##1 %%1");
    BAD("#{unterminated var");
    BAD("#^");
    BAD("#!");
    BAD("#!{unterminated var");
    BAD("#!{var}");
    BAD("#!{var}{unterminated spec");
    BAD("#!{var}{%!}");
    BAD("#*");
    BAD("#*{unterminated spec");
    BAD("#*{%!}");
    BAD("%!");
}
END_TEST


/*-----------------------------------------------------------------------
 * Formatting results
 */

static void
test_message(struct clog_formatter *self, struct cork_buffer *dest,
             enum clog_level level, const char *channel, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    struct clog_message message;
    struct clog_string_field var1;
    struct clog_string_field var2;
    clog_message_init(&message, level, channel);
    clog_message_add_string_field(&message, &var1, "var1", "value1");
    clog_message_add_string_field(&message, &var2, "var2", "value2");
    message.fmt = fmt;
    va_start(message.args, fmt);
    clog_formatter_format_message(self, dest, &message);
    va_end(message.args);
    clog_message_done(&message);
}

START_TEST(test_format_01)
{
    DESCRIBE_TEST;
    struct clog_formatter  *fmt;
    struct cork_buffer  dest = CORK_BUFFER_INIT();
    const char  *fmt_str =
        "  hello #{var1} ## #{var2} "
        "[%l] [%L] %c %m "
        "#!{var1}{%k = %v }"
        "#!{var2}{%% }"
        "#!{var3}{%% }"
        "#*{%k=%v }"
        "world #{var1} #{var3}";
    const char  *expected =
        "  hello value1 # value2 "
        "[INFO] [INFO    ] test This is only a test. "
        "var1 = value1 "
        "% "
        ""
        "var1=value1 var2=value2 "
        "world value1 ";

    fail_if_error(fmt = clog_formatter_new(fmt_str));
    test_message(fmt, &dest, CLOG_LEVEL_INFO, "test", "This is only a test.");

    ck_assert_str_eq((char *) dest.buf, expected);
    cork_buffer_done(&dest);
    clog_formatter_free(fmt);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("logging");

    TCase  *tc_format = tcase_create("format");
    tcase_add_test(tc_format, test_format_parse_01);
    tcase_add_test(tc_format, test_format_01);
    suite_add_tcase(s, tc_format);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    setup_allocator();
    /* Use TAP for our stderr output instead of libcheck's default. */
    srunner_set_tap(runner, "-");
    srunner_run_all(runner, CK_SILENT);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
