# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

include_directories(../include)

#-----------------------------------------------------------------------
# Build the library

set(LIBCLOGGER_SRC
    libclogger/foo.c
)

add_library(libclogger SHARED ${LIBCLOGGER_SRC})
set_target_properties(libclogger PROPERTIES
    OUTPUT_NAME clogger
    SOVERSION 0.0.0)
target_link_libraries(libclogger
    ${CORK_LIBRARIES}
)

install(TARGETS libclogger DESTINATION lib)

#-----------------------------------------------------------------------
# Generate the pkg-config file

set(prefix ${CMAKE_INSTALL_PREFIX})
configure_file(clogger.pc.in clogger.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/clogger.pc
        DESTINATION lib/pkgconfig)
