/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <check.h>

#include "clogger/api.h"
#include "clogger/fields.h"
#include "clogger/stash.h"

#include "helpers.h"


/*-----------------------------------------------------------------------
 * Test data
 */

#define CLOG_CHANNEL "test"

START_TEST(test_stash)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    struct clog_stash* stash = clog_stash_new();
    struct clog_handler* handler = clog_stashing_handler_new(stash);
    clog_handler_push_process(handler);

    cloge_critical {
        clog_add_field(field1, string, "hello");
        clog_set_message("Critical event");
    }
    cloge_error {
        clog_add_field(field1, string, "hello");
        clog_set_message("Error event");
    }
    cloge_warning {
        clog_add_field(field1, string, "hello");
        clog_set_message("Warning event");
    }
    cloge_notice {
        clog_add_field(field1, string, "hello");
        clog_set_message("Notice event");
    }
    cloge_info {
        clog_add_field(field1, string, "hello");
        clog_set_message("Info event");
    }
    cloge_debug {
        clog_add_field(field1, string, "hello");
        clog_set_message("Debug event");
    }

    ck_assert(clog_stash_contains_event(stash, "__message",
                                            "Critical event", NULL));
    ck_assert(clog_stash_contains_event(stash, "field1", "hello", NULL));
    ck_assert(!clog_stash_contains_event(stash, "field1", "there", NULL));

    fail_if_error(clog_handler_pop_process(handler));
    clog_handler_free(handler);
    clog_stash_free(stash);
}
END_TEST

static unsigned int counter = 0;

static bool
bump_counter(void)
{
    return (counter++ % 2) == 0;
}

START_TEST(test_two_stashes)
{
    DESCRIBE_TEST;
    clog_set_minimum_level(CLOG_LEVEL_DEBUG);
    struct clog_stash* stash1 = clog_stash_new();
    struct clog_handler* handler1 = clog_stashing_handler_new(stash1);
    clog_handler_push_process(handler1);
    struct clog_stash* stash2 = clog_stash_new();
    struct clog_handler* handler2 = clog_stashing_handler_new(stash2);
    clog_handler_push_process(handler2);

    /* Because there are an even number of calls to bump_counter, the results
     * will be consistent: Critical, Warning, and Info will get true; Error,
     * Notice, and Debug will get false. */
    cloge_critical {
        clog_add_field(field1, string, "hello");
        clog_set_message("Critical %s", bump_counter() ? "event" : "noooooo");
    }
    cloge_error {
        clog_add_field(field1, string, "hello");
        clog_set_message("Error %s", bump_counter() ? "noooooo" : "event");
    }
    cloge_warning {
        clog_add_field(field1, string, "hello");
        clog_set_message("Warning %s", bump_counter() ? "event" : "noooooo");
    }
    cloge_notice {
        clog_add_field(field1, string, "hello");
        clog_set_message("Notice %s", bump_counter() ? "noooooo":"event");
    }
    cloge_info {
        clog_add_field(field1, string, "hello");
        clog_set_message("Info %s", bump_counter() ? "event" : "noooooo");
    }
    cloge_debug {
        clog_add_field(field1, string, "hello");
        clog_set_message("Debug %s", bump_counter() ? "noooooo" : "event");
    }

    ck_assert(clog_stash_contains_event(stash1, "__message",
                                            "Critical event", NULL));
    ck_assert(clog_stash_contains_event(stash1, "field1", "hello", NULL));
    ck_assert(!clog_stash_contains_event(stash1, "field1", "there", NULL));

    ck_assert(clog_stash_contains_event(stash2, "__message",
                                            "Critical event", NULL));
    ck_assert(clog_stash_contains_event(stash2, "field1", "hello", NULL));
    ck_assert(!clog_stash_contains_event(stash2, "field1", "there", NULL));

    fail_if_error(clog_handler_pop_process(handler2));
    clog_handler_free(handler2);
    clog_stash_free(stash2);
    fail_if_error(clog_handler_pop_process(handler1));
    clog_handler_free(handler1);
    clog_stash_free(stash1);
}
END_TEST


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("stash");

    TCase  *tc_stash = tcase_create("stash");
    tcase_add_test(tc_stash, test_stash);
    tcase_add_test(tc_stash, test_two_stashes);
    suite_add_tcase(s, tc_stash);

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
