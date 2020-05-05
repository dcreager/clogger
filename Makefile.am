# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright © 2020, clogger authors.
# Please see the COPYING file in this distribution for license details.
# ------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# .commit-stamp and .version-stamp

# These should only appear in distribution tarballs.
dist-hook:
	echo $(GIT_SHA1) > $(distdir)/.commit-stamp
	echo $(VERSION) > $(distdir)/.version-stamp

#-------------------------------------------------------------------------------
# Preliminaries

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = \
    -I$(srcdir)/include

bin_PROGRAMS =
check_PROGRAMS =
CLEANFILES =
dist_doc_DATA =
EXTRA_DIST =
EXTRA_PROGRAMS =
lib_LTLIBRARIES =
pkgconfig_DATA =
TESTS =

#-------------------------------------------------------------------------------
# Extras

EXTRA_DIST += \
    build-aux/calculate \
    src/clogger.pc.in

#-------------------------------------------------------------------------------
# Docs

dist_doc_DATA += \
    README.md

#-------------------------------------------------------------------------------
# libclogger

lib_LTLIBRARIES += libclogger.la
pkgconfig_DATA += src/clogger.pc

cloggerincludedir = $(includedir)/clogger
cloggerinclude_HEADERS = \
    include/clogger.h \
    include/clogger/api.h \
    include/clogger/error.h \
    include/clogger/fields.h \
    include/clogger/formatter.h \
    include/clogger/handlers.h \
    include/clogger/logging.h \
    include/clogger/stash.h

helpersincludedir = $(includedir)/clogger/helpers
helpersinclude_HEADERS = \
    include/clogger/helpers/fields.h

libclogger_la_SOURCES = \
    $(cloggerinclude_HEADERS) \
    $(helpersinclude_HEADERS) \
    src/libclogger/fields.c \
    src/libclogger/filter.c \
    src/libclogger/formatter.c \
    src/libclogger/levels.c \
    src/libclogger/logging.c \
    src/libclogger/null.c \
    src/libclogger/stack.c \
    src/libclogger/stash.c \
    src/libclogger/stream.c

libclogger_la_CPPFLAGS = @CORK_CFLAGS@ $(AM_CPPFLAGS) $(CPPFLAGS)
libclogger_la_LIBADD = @CORK_LIBS@
libclogger_la_LDFLAGS = $(AM_LDFLAGS) $(LDFLAGS) -version-info 3:0:1

#-----------------------------------------------------------------------
# Tests

# Standalone tests

# Keep this sorted generally in order of dependency, so that more basic features
# are checked before larger features that build on them.
standalone_tests = \
    tests/test-formatter \
    tests/test-logging \
    tests/test-stash \
    tests/test-benchmark

EXTRA_DIST += tap-driver.sh
EXTRA_PROGRAMS += $(standalone_tests)
LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh

if RUN_TESTS

check_PROGRAMS += $(standalone_tests)
TESTS += $(standalone_tests)

tests_CPPFLAGS_ = @CORK_CFLAGS@ @CHECK_CFLAGS@ $(AM_CPPFLAGS) $(CPPFLAGS)
tests_LDADD_ = libclogger.la @CORK_LIBS@ @CHECK_LIBS@
tests_LDFLAGS_ = $(AM_LDFLAGS) $(LDFLAGS) -no-install

tests_test_formatter_SOURCES = tests/test-formatter.c tests/helpers.h
tests_test_formatter_CPPFLAGS = $(tests_CPPFLAGS_)
tests_test_formatter_LDADD = $(tests_LDADD_)
tests_test_formatter_LDFLAGS = $(tests_LDFLAGS_)

tests_test_logging_SOURCES = tests/test-logging.c tests/helpers.h
tests_test_logging_CPPFLAGS = $(tests_CPPFLAGS_)
tests_test_logging_LDADD = $(tests_LDADD_)
tests_test_logging_LDFLAGS = $(tests_LDFLAGS_)

tests_test_stash_SOURCES = tests/test-stash.c tests/helpers.h
tests_test_stash_CPPFLAGS = $(tests_CPPFLAGS_)
tests_test_stash_LDADD = $(tests_LDADD_)
tests_test_stash_LDFLAGS = $(tests_LDFLAGS_)

tests_test_benchmark_SOURCES = tests/test-benchmark.c tests/helpers.h
tests_test_benchmark_CPPFLAGS = $(tests_CPPFLAGS_)
tests_test_benchmark_LDADD = $(tests_LDADD_)
tests_test_benchmark_LDFLAGS = $(tests_LDFLAGS_)

endif RUN_TESTS

# Cram tests

TEST_EXTENSIONS = .t
EXTRA_DIST += \
    tests/COPYING.cram.txt \
    tests/ccram \
    tests/cram.py

cram_tests = \
    tests/channels.t \
    tests/formats.t \
    tests/levels.t

EXTRA_DIST += $(cram_tests)

if RUN_TESTS

TESTS += $(cram_tests)

T_LOG_COMPILER = $(srcdir)/tests/ccram
AM_T_LOG_FLAGS = \
    --python $(PYTHON) \
    --root $(abs_srcdir) \
    --build $(abs_builddir) \
    --cram $(abs_srcdir)/tests/cram.py \
    --tests

check_PROGRAMS += clog-test
clog_test_SOURCES = src/clog-test/clog-test.c
clog_test_CPPFLAGS = @CORK_CFLAGS@ $(AM_CPPFLAGS) $(CPPFLAGS)
clog_test_LDADD = libclogger.la @CORK_LIBS@

endif

# If you have test failures during distcheck, cram won't be able to print out
# the diff, since `make distcheck` causes the unpacked distribution tarball to
# be read-only.  Uncomment the following to (temporarily) make the unpacked
# tests directory writeable, so that you can see what caused the test failure.
#distcheck-hook:
#	chmod -R ug+w $(distdir)/tests

# Test harness

@VALGRIND_CHECK_RULES@

if !RUN_TESTS
check-local:
	$(error Cannot run test suite without check and Python installed!)
endif
