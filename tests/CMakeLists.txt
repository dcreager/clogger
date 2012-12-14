# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011, RedJack, LLC.
# All rights reserved.
#
# Please see the COPYING file in this distribution for license
# details.
# ----------------------------------------------------------------------

include_directories(../include)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)

#-----------------------------------------------------------------------
# Check for prerequisite libraries

find_package(PkgConfig)

pkg_check_modules(CHECK REQUIRED check)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

#-----------------------------------------------------------------------
# Build the test cases

macro(make_test test_name)
    add_executable(${test_name} ${test_name}.c)
    target_link_libraries(${test_name} ${CHECK_LIBRARIES} libclogger)
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro(make_test)

make_test(test-formatter)
make_test(test-logging)

#-----------------------------------------------------------------------
# Command-line tests

find_program(
    CRAM_EXECUTABLE
    NAMES cram
    HINTS ENV CRAM_DIR
    PATH_SUFFIXES bin
    DOC "Cram test harness"
)

if (CRAM_EXECUTABLE)
    configure_file(ccram ${CMAKE_BINARY_DIR}/ccram COPYONLY)
    add_test(
        NAME cram-tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_BINARY_DIR}/ccram
            --cram ${CRAM_EXECUTABLE}
            --tests ${CMAKE_CURRENT_SOURCE_DIR}
    )
else (CRAM_EXECUTABLE)
    message(WARNING "Unable to find cram test harness; skipping tests.")
endif (CRAM_EXECUTABLE)
