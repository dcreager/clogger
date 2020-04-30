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
#include "clogger/helpers/fields.h"
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

    cloge_critical ("Critical event") {
        clf(field1, string, "hello");
    }
    cloge_error ("Error event") {
        clf(field1, string, "hello");
    }
    cloge_warning ("Warning event") {
        clf(field1, string, "hello");
    }
    cloge_notice ("Notice event") {
        clf(field1, string, "hello");
    }
    cloge_info ("Info event") {
        clf(field1, string, "hello");
    }
    cloge_debug ("Debug event") {
        clf(field1, string, "hello");
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


/*-----------------------------------------------------------------------
 * Testing harness
 */

Suite *
test_suite()
{
    Suite  *s = suite_create("stash");

    TCase  *tc_stash = tcase_create("stash");
    tcase_add_test(tc_stash, test_stash);
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
    srunner_run_all(runner, CK_NORMAL);
    number_failed = srunner_ntests_failed(runner);
    srunner_free(runner);

    return (number_failed == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
