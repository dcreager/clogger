/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <libcork/core.h>

#include "clogger/api.h"
#include "clogger/handlers.h"

static int
clog_null_handler__annotation(struct clog_handler *self,
                              struct clog_message *msg,
                              const char *key, const char *value)
{
    return CLOG_SKIP;
}

static int
clog_null_handler__message(struct clog_handler *self, struct clog_message *msg)
{
    return CLOG_SKIP;
}

static struct clog_handler  NULL_HANDLER = {
    clog_null_handler__annotation,
    clog_null_handler__message,
    NULL
};

struct clog_handler *
clog_null_handler(void)
{
    return &NULL_HANDLER;
}
