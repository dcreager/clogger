/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include <libcork/ds.h>

#include "clogger/api.h"
#include "clogger/handlers.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Test data
 */

#define CLOG_CHANNEL "test"

static void
generate_messages(void)
{
    clog_critical("Critical message");
    clog_error("Error message");
    clog_warning("Warning message");
    clog_notice("Notice message");
    clog_info("Info message");
    clog_debug("Debug message");
}

#undef CLOG_CHANNEL

static void
test_process(const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    struct cork_stream_consumer  *consumer =
        cork_buffer_to_stream_consumer(&buf);
    struct clog_handler  *handler =
        clog_stream_handler_new_consumer(consumer);

    clog_handler_push_process(handler);
    generate_messages();
    fail_unless(strcmp(buf.buf, expected) == 0,
                "Unexpected logging results\n\nGot\n%s\n\nExpected\n%s",
                (char *) buf.buf, expected);
    fail_if_error(clog_handler_pop_process(handler));

    clog_handler_free(handler);
    cork_buffer_done(&buf);
}

static void
test_thread(const char *expected)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    struct cork_stream_consumer  *consumer =
        cork_buffer_to_stream_consumer(&buf);
    struct clog_handler  *handler =
        clog_stream_handler_new_consumer(consumer);

    clog_handler_push_thread(handler);
    generate_messages();
    fail_unless(strcmp(buf.buf, expected) == 0,
                "Unexpected logging results\n\nGot\n%s\n\nExpected\n%s",
                (char *) buf.buf, expected);
    fail_if_error(clog_handler_pop_thread(handler));

    clog_handler_free(handler);
    cork_buffer_done(&buf);
}


/*-----------------------------------------------------------------------
 * Defaults for everything
 */

static const char  *EXPECTED_01 =
    "[CRITICAL] test: Critical message\n"
    "[ERROR   ] test: Error message\n"
    "[WARNING ] test: Warning message\n";

START_TEST(test_process_01)
{
    DESCRIBE_TEST;
    /* This is the default, but we're setting it explicitly in case we run the
     * tests with CK_FORK=no */
    clog_set_maximum_level(CLOG_LEVEL_WARNING);
    test_process(EXPECTED_01);
}
END_TEST

START_TEST(test_thread_01)
{
    DESCRIBE_TEST;
    /* This is the default, but we're setting it explicitly in case we run the
     * tests with CK_FORK=no */
    clog_set_maximum_level(CLOG_LEVEL_WARNING);
    test_thread(EXPECTED_01);
}
END_TEST


/*-----------------------------------------------------------------------
 * Show all levels
 */

static const char  *EXPECTED_02 =
    "[CRITICAL] test: Critical message\n"
    "[ERROR   ] test: Error message\n"
    "[WARNING ] test: Warning message\n"
    "[NOTICE  ] test: Notice message\n"
    "[INFO    ] test: Info message\n"
    "[DEBUG   ] test: Debug message\n";

START_TEST(test_process_02)
{
    DESCRIBE_TEST;
    clog_set_maximum_level(CLOG_LEVEL_DEBUG);
    test_process(EXPECTED_02);
}
END_TEST

START_TEST(test_thread_02)
{
    DESCRIBE_TEST;
    clog_set_maximum_level(CLOG_LEVEL_DEBUG);
    test_thread(EXPECTED_02);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("logging");

    TCase  *tc_process = tcase_create("process");
    tcase_add_test(tc_process, test_process_01);
    tcase_add_test(tc_process, test_process_02);
    suite_add_tcase(s, tc_process);

    TCase  *tc_thread = tcase_create("thread");
    tcase_add_test(tc_thread, test_thread_01);
    tcase_add_test(tc_thread, test_thread_02);
    suite_add_tcase(s, tc_thread);

    return s;
}


int
main(int argc, const char **argv)
{
    int  number_failed;
    Suite  *suite = test_suite();
    SRunner  *runner = srunner_create(suite);

    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
