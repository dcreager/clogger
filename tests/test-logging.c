/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
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
#include "clogger/fields.h"
#include "clogger/handlers.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Test data
 */

#define CLOG_CHANNEL "test"

static unsigned int counter = 0;

static bool
bump_counter(void)
{
    return (counter++ % 2) == 0;
}

static void
generate_messages(void)
{
    clog_critical("Critical message");
    clog_error("Error message");
    clog_warning("Warning message");
    clog_notice("Notice message");
    clog_info("Info message");
    clog_debug("Debug message");
    /* Because there are an even number of calls to bump_counter, the results
     * will be consistent: Critical, Warning, and Info will get true; Error,
     * Notice, and Debug will get false. */
    cloge_critical {
        clog_add_field(field1, printf, "%s%s", "he", "llo");
        clog_set_message("Critical %s", bump_counter() ? "event" : "noooooo");
    }
    cloge_error {
        clog_add_field(field1, string, "hello");
        clog_set_message("Error %s", bump_counter() ? "noooooo" : "event");
    }
    cloge_warning {
        clog_add_field(field1, printf, "%s%s", "he", "llo");
        clog_set_message("Warning %s", bump_counter() ? "event" : "noooooo");
    }
    cloge_notice {
        clog_add_field(field1, string, "hello");
        clog_set_message("Notice %s", bump_counter() ? "noooooo" : "event");
    }
    cloge_info {
        clog_add_field(field1, printf, "%s%s", "he", "llo");
        clog_set_message("Info %s", bump_counter() ? "event" : "noooooo");
    }
    cloge_debug {
        clog_add_field(field1, string, "hello");
        clog_set_message("Debug %s", bump_counter() ? "noooooo" : "event");
    }
}

#undef CLOG_CHANNEL

#define DEFAULT_FORMAT "[%L] %c:#*{ %k=%v} %m"

static bool use_process;
static struct cork_buffer  *log_buf;
static struct cork_stream_consumer  *log_consumer;
static struct clog_handler  *handler;

static void
clog_handler_push_current(struct clog_handler *handler)
{
    use_process?
        clog_handler_push_process(handler):
        clog_handler_push_thread(handler);
}

static int
clog_handler_pop_current(struct clog_handler *handler)
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


#define create_log_handler(where) \
    do { \
        log_buf = cork_buffer_new(); \
        log_consumer = cork_buffer_to_stream_consumer(log_buf); \
        handler = clog_stream_handler_new_consumer \
            (log_consumer, DEFAULT_FORMAT); \
        clog_handler_push_##where(handler); \
    } while (0)

#define destroy_log_handler(where) \
    do { \
        fail_if_error(clog_handler_pop_##where(handler)); \
        clog_handler_free(handler); \
        cork_buffer_free(log_buf); \
    } while (0)

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

static void
annotate_handle(struct clog_handler *log, struct clog_message* message)
{
    if (log->next != NULL) {
        struct clog_string_field key1;
        struct clog_string_field key2;
        clog_message_add_string_field(&message->fields, &key1, "key1",
                                      "value1");
        clog_message_add_string_field(&message->fields, &key2, "key2",
                                      "value2");
        clog_handler_handle(log->next, message);
        clog_message_pop_field(message, &key2.parent);
        clog_message_pop_field(message, &key1.parent);
    }
}

static void
annotate_free(struct clog_handler *log)
{
    /* Nothing to do */
}

struct clog_handler  annotate = {
    annotate_handle,
    annotate_free,
    NULL
};


/*-----------------------------------------------------------------------
 * Defaults for everything
 */

static const char* EXPECTED_01 =
        "[CRITICAL] test: Critical message\n"
        "[ERROR   ] test: Error message\n"
        "[WARNING ] test: Warning message\n"
        "[CRITICAL] test: field1=hello Critical event\n"
        "[ERROR   ] test: field1=hello Error event\n"
        "[WARNING ] test: field1=hello Warning event\n";

START_TEST(test_01)
{
    DESCRIBE_TEST;
    /* This is the default, but we're setting it explicitly in case we run the
     * tests with CK_FORK=no */
    clog_set_minimum_level(CLOG_LEVEL_WARNING);
    create_log_handler(current);
    test_logs(EXPECTED_01);
    destroy_log_handler(current);
}
END_TEST


/*-----------------------------------------------------------------------
 * Show all levels
 */

static const char* EXPECTED_02 =
        "[CRITICAL] test: Critical message\n"
        "[ERROR   ] test: Error message\n"
        "[WARNING ] test: Warning message\n"
        "[NOTICE  ] test: Notice message\n"
        "[INFO    ] test: Info message\n"
        "[DEBUG   ] test: Debug message\n"
        "[CRITICAL] test: field1=hello Critical event\n"
        "[ERROR   ] test: field1=hello Error event\n"
        "[WARNING ] test: field1=hello Warning event\n"
        "[NOTICE  ] test: field1=hello Notice event\n"
        "[INFO    ] test: field1=hello Info event\n"
        "[DEBUG   ] test: field1=hello Debug event\n";

START_TEST(test_02)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    create_log_handler(current);
    test_logs(EXPECTED_02);
    destroy_log_handler(current);
}
END_TEST


/*-----------------------------------------------------------------------
 * Annotations
 */

static const char* EXPECTED_annotate_01 =
        "[CRITICAL] test: key1=value1 key2=value2 Critical message\n"
        "[ERROR   ] test: key1=value1 key2=value2 Error message\n"
        "[WARNING ] test: key1=value1 key2=value2 Warning message\n"
        "[NOTICE  ] test: key1=value1 key2=value2 Notice message\n"
        "[INFO    ] test: key1=value1 key2=value2 Info message\n"
        "[DEBUG   ] test: key1=value1 key2=value2 Debug message\n"
        "[CRITICAL] test: field1=hello key1=value1 key2=value2 Critical event\n"
        "[ERROR   ] test: field1=hello key1=value1 key2=value2 Error event\n"
        "[WARNING ] test: field1=hello key1=value1 key2=value2 Warning event\n"
        "[NOTICE  ] test: field1=hello key1=value1 key2=value2 Notice event\n"
        "[INFO    ] test: field1=hello key1=value1 key2=value2 Info event\n"
        "[DEBUG   ] test: field1=hello key1=value1 key2=value2 Debug event\n";


START_TEST(test_annotate_01)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    create_log_handler(current);
    clog_handler_push_current(&annotate);
    test_logs(EXPECTED_annotate_01);
    fail_if_error(clog_handler_pop_current(&annotate));
    destroy_log_handler(current);
}
END_TEST


/*-----------------------------------------------------------------------
 * Skipped annotations
 */

static const char* EXPECTED_annotate_02 =
        "[CRITICAL] test: Critical message\n"
        "[ERROR   ] test: Error message\n"
        "[WARNING ] test: Warning message\n"
        "[NOTICE  ] test: Notice message\n"
        "[INFO    ] test: Info message\n"
        "[DEBUG   ] test: Debug message\n"
        "[CRITICAL] test: field1=hello Critical event\n"
        "[ERROR   ] test: field1=hello Error event\n"
        "[WARNING ] test: field1=hello Warning event\n"
        "[NOTICE  ] test: field1=hello Notice event\n"
        "[INFO    ] test: field1=hello Info event\n"
        "[DEBUG   ] test: field1=hello Debug event\n";

START_TEST(test_annotate_02)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    clog_handler_push_current(&annotate);
    create_log_handler(current);
    test_logs(EXPECTED_annotate_02);
    destroy_log_handler(current);
    fail_if_error(clog_handler_pop_current(&annotate));
}
END_TEST


/*-----------------------------------------------------------------------
 * Thread/process ordering
 */

static const char* EXPECTED_ordering_01 =
        "[CRITICAL] test: Critical message\n"
        "[ERROR   ] test: Error message\n"
        "[WARNING ] test: Warning message\n"
        "[NOTICE  ] test: Notice message\n"
        "[INFO    ] test: Info message\n"
        "[DEBUG   ] test: Debug message\n"
        "[CRITICAL] test: field1=hello Critical event\n"
        "[ERROR   ] test: field1=hello Error event\n"
        "[WARNING ] test: field1=hello Warning event\n"
        "[NOTICE  ] test: field1=hello Notice event\n"
        "[INFO    ] test: field1=hello Info event\n"
        "[DEBUG   ] test: field1=hello Debug event\n";

START_TEST(test_ordering_01)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    /* Right now thread handlers are always executed before process handlers, so
     * the annotations shouldn't be run.  (We're also restricted in that we
     * can't push process handlers onto the stack after pushing any thread
     * handlers, which helps enforce this constraint.) */
    clog_handler_push_process(&annotate);
    create_log_handler(thread);
    test_logs(EXPECTED_ordering_01);
    destroy_log_handler(thread);
    fail_if_error(clog_handler_pop_process(&annotate));
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
    tcase_add_test(tc_process, test_ordering_01);
    suite_add_tcase(s, tc_process);

    TCase  *tc_thread = tcase_create("thread");
    tcase_add_checked_fixture(tc_thread, setup_thread, NULL);
    tcase_add_test(tc_thread, test_01);
    tcase_add_test(tc_thread, test_02);
    tcase_add_test(tc_thread, test_annotate_01);
    tcase_add_test(tc_thread, test_annotate_02);
    tcase_add_test(tc_thread, test_ordering_01);
    suite_add_tcase(s, tc_thread);

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
