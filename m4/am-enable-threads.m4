dnl am-enable-threads.m4 by Miroslav Zagorac <mzagorac@haproxy.com>
dnl
AC_DEFUN([AM_ENABLE_THREADS], [
	AC_ARG_ENABLE([threads],
		[AS_HELP_STRING([--enable-threads], [enable threads @<:@default=check@:>@])],
		[enable_threads="${enableval}"],
		[enable_threads=check]
	)

	if test "${enable_threads}" != "no"; then
		HAVE_THREADS=
		THREADS_CFLAGS=
		THREADS_CPPFLAGS=
		THREADS_LDFLAGS=
		THREADS_LIBS=

		AM_VARIABLES_STORE

		LDFLAGS="${LDFLAGS} ${THREADS_LDFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${THREADS_CPPFLAGS}"

		AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_ERROR([THREADS library not found])])
		AC_CHECK_HEADER([pthread.h], [], [AC_MSG_ERROR([THREADS library headers not found])])

		HAVE_THREADS=yes
		THREADS_LIBS="-lpthread"

		AC_DEFINE([USE_THREADS], [1], [Define to 1 for multi-thread support.])
		AC_DEFINE([_REENTRANT], [1], [Define to 1 for multi-thread support.])

		AM_VARIABLES_RESTORE

		AC_MSG_NOTICE([THREADS environment variables:])
		AC_MSG_NOTICE([  THREADS_CFLAGS=${THREADS_CFLAGS}])
		AC_MSG_NOTICE([  THREADS_CPPFLAGS=${THREADS_CPPFLAGS}])
		AC_MSG_NOTICE([  THREADS_LDFLAGS=${THREADS_LDFLAGS}])
		AC_MSG_NOTICE([  THREADS_LIBS=${THREADS_LIBS}])

		AC_SUBST([THREADS_CFLAGS])
		AC_SUBST([THREADS_CPPFLAGS])
		AC_SUBST([THREADS_LDFLAGS])
		AC_SUBST([THREADS_LIBS])
	fi
])
