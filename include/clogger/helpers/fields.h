/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef CLOGGER_HELPERS_FIELDS_H
#define CLOGGER_HELPERS_FIELDS_H

#include <clogger/api.h>
#include <clogger/fields.h>

#define clf(field_name, field_type, ...)                                       \
    do {                                                                       \
        clog_add_##field_type##_field(clog_pending_event(), #field_name,       \
                                      __VA_ARGS__);                            \
    } while (0)

#endif /* CLOGGER_HELPERS_FIELDS_H */
