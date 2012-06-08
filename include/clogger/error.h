/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef CLOGGER_ERROR_H
#define CLOGGER_ERROR_H

#include <libcork/core.h>


/* hash of "clogger.h" */
#define CLOG_ERROR  0x9c7ff224

enum clog_error {
    CLOG_BAD_STACK
};

#define clog_set_error(code, ...) \
    (cork_error_set(CLOG_ERROR, code, __VA_ARGS__))

#define clog_bad_stack(...) \
    clog_set_error(CLOG_BAD_STACK, __VA_ARGS__)


#endif /* CLOGGER_ERROR_H */
