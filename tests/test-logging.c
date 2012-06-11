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

#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/helpers/errors.h>

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

static bool use_process;
static struct cork_buffer  *log_buf;
static struct cork_stream_consumer  *log_consumer;
static struct clog_handler  *handler;

static void
push_handler(struct clog_handler *handler)
{
    use_process?
        clog_handler_push_process(handler):
        clog_handler_push_thread(handler);
}

static int
pop_handler(struct clog_handler *handler)
{
    return use_process?
        clog_handler_pop_process(handler):
        clog_handler_pop_thread(handler);
}

static void
setup_process(void)
{
    use_process = true;
}

static void
setup_thread(void)
{
    use_process = false;
}

static void
create_log_handler(void)
{
    log_buf = cork_buffer_new();
    log_consumer = cork_buffer_to_stream_consumer(log_buf);
    handler = clog_stream_handler_new_consumer(log_consumer);
    push_handler(handler);
}

static void
destroy_log_handler(void)
{
    fail_if_error(pop_handler(handler));
    clog_handler_free(handler);
    cork_buffer_free(log_buf);
}

static void
test_logs(const char *expected)
{
    generate_messages();
    fail_unless(strcmp(log_buf->buf, expected) == 0,
                "Unexpected logging results\n\nGot\n%s\n\nExpected\n%s",
                (char *) log_buf->buf, expected);
}


/*-----------------------------------------------------------------------
 * Test annotator
 */

static int
annotate__annotation(struct clog_handler *log, struct clog_message *msg,
                     const char *key, const char *value)
{
    return 0;
}

static int
annotate__message(struct clog_handler *log, struct clog_message *msg)
{
    rii_check(clog_annotate_message(log, msg, "key1", "value1"));
    rii_check(clog_annotate_message(log, msg, "key2", "value2"));
    return 0;
}

static void
annotate__free(struct clog_handler *log)
{
    /* Nothing to do */
}

struct clog_handler  annotate = {
    annotate__annotation,
    annotate__message,
    annotate__free,
    NULL
};


/*-----------------------------------------------------------------------
 * Defaults for everything
 */

static const char  *EXPECTED_01 =
    "[CRITICAL] test: Critical message\n"
    "[ERROR   ] test: Error message\n"
    "[WARNING ] test: Warning message\n";

START_TEST(test_01)
{
    DESCRIBE_TEST;
    /* This is the default, but we're setting it explicitly in case we run the
     * tests with CK_FORK=no */
    clog_set_minimum_level(CLOG_LEVEL_WARNING);
    create_log_handler();
    test_logs(EXPECTED_01);
    destroy_log_handler();
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

START_TEST(test_02)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    create_log_handler();
    test_logs(EXPECTED_02);
    destroy_log_handler();
}
END_TEST


/*-----------------------------------------------------------------------
 * Annotations
 */

static const char  *EXPECTED_annotate_01 =
    "[CRITICAL] test: key1=value1 key2=value2 Critical message\n"
    "[ERROR   ] test: key1=value1 key2=value2 Error message\n"
    "[WARNING ] test: key1=value1 key2=value2 Warning message\n"
    "[NOTICE  ] test: key1=value1 key2=value2 Notice message\n"
    "[INFO    ] test: key1=value1 key2=value2 Info message\n"
    "[DEBUG   ] test: key1=value1 key2=value2 Debug message\n";


START_TEST(test_annotate_01)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    create_log_handler();
    push_handler(&annotate);
    test_logs(EXPECTED_annotate_01);
    fail_if_error(pop_handler(&annotate));
    destroy_log_handler();
}
END_TEST


/*-----------------------------------------------------------------------
 * Skipped annotations
 */

static const char  *EXPECTED_annotate_02 =
    "[CRITICAL] test: Critical message\n"
    "[ERROR   ] test: Error message\n"
    "[WARNING ] test: Warning message\n"
    "[NOTICE  ] test: Notice message\n"
    "[INFO    ] test: Info message\n"
    "[DEBUG   ] test: Debug message\n";

START_TEST(test_annotate_02)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    push_handler(&annotate);
    create_log_handler();
    test_logs(EXPECTED_annotate_02);
    destroy_log_handler();
    fail_if_error(pop_handler(&annotate));
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
    tcase_add_checked_fixture(tc_process, setup_process, NULL);
    tcase_add_test(tc_process, test_01);
    tcase_add_test(tc_process, test_02);
    tcase_add_test(tc_process, test_annotate_01);
    tcase_add_test(tc_process, test_annotate_02);
    suite_add_tcase(s, tc_process);

    TCase  *tc_thread = tcase_create("thread");
    tcase_add_checked_fixture(tc_thread, setup_thread, NULL);
    tcase_add_test(tc_thread, test_01);
    tcase_add_test(tc_thread, test_02);
    tcase_add_test(tc_thread, test_annotate_01);
    tcase_add_test(tc_thread, test_annotate_02);
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
