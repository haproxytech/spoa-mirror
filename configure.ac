dnl Process this file with autoconf to produce a configure script.
dnl
dnl configure.ac - Miroslav Zagorac <mzagorac@haproxy.com>

AC_PREREQ([2.69])
AC_COPYRIGHT([Copyright (C) 2018-2020 HAProxy Technologies])
AC_INIT(m4_esyscmd_s([basename ${PWD}]), m4_esyscmd_s([cat VERSION]), [mzagorac@haproxy.com])

echo "*** configuring for ${PACKAGE_NAME} v${PACKAGE_VERSION} ***"

AC_PREFIX_DEFAULT([/usr])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([1.13])
AM_VARIABLES_INIT
AM_SILENT_RULES([yes])
AM_HOST

dnl Check for source directory.
dnl
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config/config.h])

AM_PROG_PKGCONFIG

dnl Checking command line options.
dnl
AM_ENABLE_DEBUG
AM_ENABLE_GPROF
AM_ENABLE_THREADS
dnl
dnl Misc
dnl
AM_WITH_CURL
AM_WITH_LIBEV

dnl Checks for programs.
dnl
AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for header files.
dnl
AC_CHECK_HEADERS([fcntl.h inttypes.h libgen.h stddef.h stdint.h stdlib.h string.h strings.h unistd.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h])
AC_CHECK_HEADERS([sys/param.h sys/socket.h sys/time.h])

dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AM_CHECK___DATE__
AM_CHECK___FUNC__
AM_CHECK___STDC__
AM_CHECK___ATTRIBUTE__
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AM_TYPE_BOOL_T
AM_TYPE_UNCHAR
AM_TYPE_USHORT
AM_TYPE_ULONG
AM_TYPE_UINT

dnl Checks for libraries.
dnl
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([inet_ntop], [nsl])

dnl Checks for library functions.
dnl
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([gettimeofday localtime_r])
AC_CHECK_FUNCS([memchr memset])
AC_CHECK_FUNCS([dup2 socket])
AC_CHECK_FUNCS([strcasecmp strncasecmp strchr strdup strndup strerror strtoll strtoull])

dnl Set the compiler flags
dnl
AM_PROG_CC_SET([-fno-strict-aliasing])

AM_CONDITIONAL([WANT_CURL],    [test "${HAVE_CURL}" = "yes"])
AM_CONDITIONAL([WANT_DEBUG],   [test "${enable_debug}" = "yes"])
AM_CONDITIONAL([WANT_LIBEV],   [test "${HAVE_LIBEV}" = "yes"])
AM_CONDITIONAL([WANT_THREADS], [test "${HAVE_THREADS}" = "yes"])

AM_VARIABLES_SET

AM_VARIABLE_SET([SPOA_MIRROR_CPPFLAGS], [ ${CURL_CPPFLAGS} ${LIBEV_CPPFLAGS} ${THREADS_CPPFLAGS} ])
AM_VARIABLE_SET([SPOA_MIRROR_CFLAGS],   [ ${CURL_CFLAGS}   ${LIBEV_CFLAGS}   ${THREADS_CFLAGS}   ])
AM_VARIABLE_SET([SPOA_MIRROR_LDFLAGS],  [ ${CURL_LDFLAGS}  ${LIBEV_LDFLAGS}  ${THREADS_LDFLAGS}  ])
AM_VARIABLE_SET([SPOA_MIRROR_LIBS],     [ ${CURL_LIBS}     ${LIBEV_LIBS}     ${THREADS_LIBS}     ])

AC_SUBST([SPOA_MIRROR_CPPFLAGS])
AC_SUBST([SPOA_MIRROR_CFLAGS])
AC_SUBST([SPOA_MIRROR_LDFLAGS])
AC_SUBST([SPOA_MIRROR_LIBS])

dnl version.h
dnl
CONFIGURE_OPTIONS="${ac_configure_args}"
AC_SUBST([DATE])
AC_SUBST([PACKAGE_VERSION])
AC_SUBST([CONFIGURE_OPTIONS])

AC_CONFIG_FILES([Makefile src/Makefile include/common/version.h])
AC_OUTPUT

AM_SHOW_CONFIG


AH_TOP([
#ifndef _COMMON_CONFIG_H
#define _COMMON_CONFIG_H
])

AH_BOTTOM([
#endif /* _COMMON_CONFIG_H */
])
