dnl am-common.m4 by Miroslav Zagorac <mzagorac@haproxy.com>
dnl
AC_DEFUN([AM_TYPE_BOOL_T], [AC_CHECK_TYPE([bool_t], [unsigned char])])
AC_DEFUN([AM_TYPE_UNCHAR], [AC_CHECK_TYPE([unchar], [unsigned char])])
AC_DEFUN([AM_TYPE_USHORT], [AC_CHECK_TYPE([ushort], [unsigned short])])
AC_DEFUN([AM_TYPE_ULONG],  [AC_CHECK_TYPE([ulong],  [unsigned long])])
AC_DEFUN([AM_TYPE_UINT],   [AC_CHECK_TYPE([uint],   [unsigned int])])

AC_DEFUN([AM_VARIABLES_STORE], [
	_saved_cppflags="${CPPFLAGS}"
	_saved_cflags="${CFLAGS}"
	_saved_ldflags="${LDFLAGS}"
	_saved_libs="${LIBS}"
])

AC_DEFUN([AM_VARIABLES_RESTORE], [
	CPPFLAGS="${_saved_cppflags}"
	CFLAGS="${_saved_cflags}"
	LDFLAGS="${_saved_ldflags}"
	LIBS="${_saved_libs}"
])

AC_DEFUN([AM_VARIABLES_INIT], [
	SET_CPPFLAGS=
	SET_CFLAGS=
	SET_LDFLAGS=
	SET_LIBS=
])

AC_DEFUN([AM_VARIABLES_SET], [
	CPPFLAGS="${CPPFLAGS} ${SET_CPPFLAGS}"
	CFLAGS="${CFLAGS} ${SET_CFLAGS}"
	LDFLAGS="${LDFLAGS} ${SET_LDFLAGS}"
	LIBS="${LIBS} ${SET_LIBS}"
])

dnl
dnl
AC_DEFUN([AM_PROG_CC_SET], [
	typeset _var_cflags=
	typeset _loop_cflags=

	AM_VARIABLES_STORE

	case "${CC}" in
	  *gcc*)
		_var_cflags="\
			-Wall \
			-Wextra \
			-Waggregate-return \
			-Wbad-function-cast \
			-Wcast-align \
			-Wchar-subscripts \
			-Wcomment \
			-Wfloat-equal \
			-Wimplicit \
			-Winline \
			-Wmain \
			-Wmissing-braces \
			-Wmissing-declarations \
			-Wmissing-noreturn \
			-Wmissing-prototypes \
			-Wnested-externs \
			-Wno-pointer-sign \
			-Wparentheses \
			-Wpointer-arith \
			-Wreturn-type \
			-Wsequence-point \
			-Wshadow \
			-Wstrict-prototypes \
			-Wswitch \
			-Wtrigraphs \
			-Wundef \
			-Wuninitialized \
			-Wunused \
			-Wwrite-strings"
		if test "${enable_debug}" = "yes"; then
			_var_cflags="${_var_cflags} -Wformat=2"
		else
			_var_cflags="${_var_cflags} -Wformat-security -Wformat-y2k"
		fi
		;;
	  cc)
		case "${host_os}" in
		  *solaris*)
			_var_cflags="-xCC"
			;;
		esac
		;;
	esac

	TMP_CFLAGS="${CFLAGS}"
	for _loop_cflags in ${_var_cflags}; do
		AC_MSG_CHECKING([whether ${CC} accepts ${_loop_cflags}])
		CFLAGS="${TMP_CFLAGS} ${_loop_cflags}"
		AC_TRY_COMPILE([], [],
			[AC_MSG_RESULT([ yes])
			SET_CFLAGS="${SET_CFLAGS} ${_loop_cflags}"],
			[AC_MSG_RESULT([ no])]
		)
	done
	CFLAGS="${TMP_CFLAGS}"

	AM_VARIABLES_RESTORE
])

dnl Check whether the C compiler has __DATE__ macro.
dnl
AC_DEFUN([AM_CHECK___DATE__], [
	AC_MSG_CHECKING([whether the C compiler (${CC}) has __DATE__ macro])
	AC_TRY_COMPILE([],
		[char *test=__DATE__;],
		[AC_MSG_RESULT([ yes])],
		[AC_DEFINE_UNQUOTED([__DATE__], ["`date`"], [Define if your C compiled doesn't have __DATE__ macro.])
		 AC_MSG_RESULT([ no])]
	)
])

dnl Check whether the C compiler has __func__ variable.
dnl
AC_DEFUN([AM_CHECK___FUNC__], [
	AC_MSG_CHECKING([whether the C compiler (${CC}) has __func__ variable])
	AC_TRY_COMPILE([#include <stdio.h>],
		[printf ("%s", __func__);],
		[AC_MSG_RESULT([ yes])],
		[AC_DEFINE_UNQUOTED([__func__], ["__unknown__"], [Define if your C compiler doesn't have __func__ variable.])
		 AC_MSG_RESULT([ no])]
	)
])

dnl Check whether the C compiler defines __STDC__.
dnl
AC_DEFUN([AM_CHECK___STDC__], [
	AC_MSG_CHECKING([whether the C compiler (${CC}) defines __STDC__])
	AC_TRY_COMPILE([],
			[#ifndef __STDC__
			    test_stdc ();
			 #endif],
		[AC_MSG_RESULT([ yes])
		 AC_DEFINE_UNQUOTED([ANSI_FUNC], [1], [Define if you use an ANSI C compiler.])
		 stdc_defined="yes"],
		[AC_MSG_RESULT([ no])]
	)
])

dnl
dnl
AC_DEFUN([AM_PROG_PKGCONFIG], [
	AC_CHECK_PROG(PKG_CONFIG, [pkg-config], [yes], [no], [], [])
	test "${PKG_CONFIG}" = "no" && AC_MSG_ERROR([pkg-config not found])
])

dnl
dnl
AC_DEFUN([AM_PATH_PKGCONFIG], [
	PKG_CONFIG_PATH=
        _pc_prefix="${ac_default_prefix}"
	# ovdje se ne smije zamijeniti $n sa ${n} jer su to argumenti m4 funkcije AM_PATH_PKGCONFIG
        test "$1" = "yes" -o "$1" = "check" || _pc_prefix="$1"

	for _loop_path in \
		${_pc_prefix}/lib \
		${_pc_prefix}/lib/i386-linux-gnu \
		${_pc_prefix}/lib/x86_64-linux-gnu \
		${_pc_prefix}/amd64 \
		${_pc_prefix}/lib32 \
		${_pc_prefix}/lib64 \
		${_pc_prefix}/share
	do
		test -d "${_loop_path}/pkgconfig" && \
			PKG_CONFIG_PATH="${PKG_CONFIG_PATH}${PKG_CONFIG_PATH:+:}${_loop_path}/pkgconfig"
	done
	AC_MSG_NOTICE([PKG_CONFIG_PATH=${PKG_CONFIG_PATH}])
])

dnl Check whether the C compiler has __attribute__ keyword.
dnl
AC_DEFUN([AM_CHECK___ATTRIBUTE__], [
	AC_MSG_CHECKING([whether the C compiler (${CC}) has __attribute__ keyword])
	AC_TRY_COMPILE([void t1 () __attribute__ ((noreturn)); void t1 () { return; };],
		[t1 ();],
		[AC_MSG_RESULT([ yes])
		 AC_DEFINE_UNQUOTED([__ATTRIBUTE__], [1], [Define if your C compiler has __attribute__ keyword.])],
		[AC_MSG_RESULT([ no])]
	)
])

AC_DEFUN([AM_ENABLE_DEBUG], [
	AC_ARG_ENABLE([debug],
		[AS_HELP_STRING([--enable-debug], [compile with debugging symbols/functions])],
		[if test "${enableval}" = "yes"; then
			AC_DEFINE([DEBUG], [1], [Define to 1 if you want to include debugging options.])
			CFLAGS="${CFLAGS} -g -O0"
		 fi]
	)
])

AC_DEFUN([AM_ENABLE_GPROF], [
	AC_ARG_ENABLE([gprof],
		[AS_HELP_STRING([--enable-gprof], [enable profiling with gprof])],
		[if test "${enableval}" = "yes"; then
			AC_DEFINE([GPROF], [1], [Define to 1 if you want to enable profiling with gprof.])
			CFLAGS="${CFLAGS} -pg"
			LDFLAGS="${LDFLAGS} -pg"
		 fi]
	)
])

AC_DEFUN([AM_VARIABLE_SET], [
_am_cache_test ()
{
	typeset _c=

	for _c in ${2}; do test "${_c}" = "${3}" && return 1; done
	eval "${1}=\"${2} ${3}\""
}

	_am_var_resolved=
	# ovdje se ne smije zamijeniti $n sa ${n} jer su to argumenti m4 funkcije AM_VARIABLE_SET
	for _am_var_loop in $2; do
		_am_cache_test _am_var_resolved "${_am_var_resolved}" "${_am_var_loop}"
	done
	$1=${_am_var_resolved}
])

AC_DEFUN([AM_SHOW_CONFIG], [
	eval "bindir=${bindir}"
	eval "datadir=${datadir}"
	eval "sysconfdir=${sysconfdir}"
	eval "mandir=${mandir}"

	echo
	echo "${PACKAGE_NAME} configuration:"
	echo "--------------------------------------------------"
	echo "  package version         : ${PACKAGE_VERSION}"
	echo "  host operating system   : ${host}"
	echo "  source code location    : ${srcdir}"
	echo "  compiler                : ${CC}"
	echo "  preprocessor flags      : ${CPPFLAGS}"
	echo "  compiler flags          : ${CFLAGS}"
	echo "  linker flags            : ${LDFLAGS}"
	echo "  libraries               : ${LIBS}"
	echo "  configure options       : ${ac_configure_args}"
	echo "  binary install path     : ${bindir}"
	echo "  data install path       : ${datadir}"
	echo "  configuration file path : ${sysconfdir}"
	echo "  man page install path   : ${mandir}"
	echo
])
