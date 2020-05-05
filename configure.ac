# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright © 2020, clogger authors.
# Please see the COPYING file in this distribution for license details.
# ------------------------------------------------------------------------------

AC_INIT([clogger],
        m4_esyscmd([build-aux/calculate version .version-stamp]),
        [info@libcork.io])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([include/clogger.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects])
AM_MAINTAINER_MODE([enable])  # Allow packagers to disable if they want
AM_SILENT_RULES([yes])
LT_INIT

# Generating version-stamp files
AC_PATH_PROG(GIT, [git])
if test -z "$GIT" -a ! -f .version-stamp ; then
  AC_MSG_FAILURE([cannot find 'git'])
fi

AC_PROG_CC
AC_PROG_CC_C99

# Dependencies
PKG_CHECK_MODULES([CORK], [libcork >= 0.14])

# pkg-config
PKG_INSTALLDIR
AC_CONFIG_FILES([src/clogger.pc])

# Tests
AM_PATH_PYTHON([2.3], , [:])
PKG_CHECK_MODULES(CHECK, [check], [check=yes], [check=no])
AM_CONDITIONAL(RUN_TESTS, [test "$PYTHON" != : -a $check = yes])

# TAP support
AC_PROG_AWK

# Valgrind support
AX_VALGRIND_DFLT([memcheck], [on])
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_DFLT([sgcheck], [off])
AX_VALGRIND_CHECK()

# Turn on fatal warnings by default; you can override this by setting CPPFLAGS
# to something else when running configure.
: ${CPPFLAGS="-Wall -Werror"}

AC_OUTPUT([Makefile])

cat <<EOF

------------------ Summary ------------------
 $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  C Compiler.....: $CC $CFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
---------------------------------------------

Check the above options and compile with:
 ${MAKE-make}

EOF
