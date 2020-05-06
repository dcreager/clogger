/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2020, clogger authors.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdarg.h>

#include <libcork/core.h>

#include "clogger/api.h"
#include "clogger/handlers.h"

static void
clog_null_handler_handle(struct clog_handler* self,
                         struct clog_message* message)
{
}

static void
clog_null_handler_free(struct clog_handler *self)
{
    cork_delete(struct clog_handler, self);
}

struct clog_handler *
clog_null_handler_new(void)
{
    struct clog_handler* self = cork_new(struct clog_handler);
    self->handle = clog_null_handler_handle;
    self->free = clog_null_handler_free;
    return self;
}
