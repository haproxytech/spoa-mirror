dnl am-with-curl.m4 by Miroslav Zagorac <mzagorac@haproxy.com>
dnl
AC_DEFUN([AM_WITH_CURL], [
	AC_ARG_WITH([curl],
		[AS_HELP_STRING([--with-curl@<:@=DIR@:>@], [use CURL library @<:@default=check@:>@])],
		[with_curl="${withval}"],
		[with_curl=check]
	)

	if test "${with_curl}" != "no"; then
		HAVE_CURL=
		CURL_CFLAGS=
		CURL_CPPFLAGS=
		CURL_LDFLAGS=
		CURL_LIBS=

		AM_PATH_PKGCONFIG([${with_curl}])
		CURL_CPPFLAGS="`PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --cflags libcurl`"
		CURL_LDFLAGS="`PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --libs-only-L libcurl`"
		CURL_LDFLAGS="${CURL_LDFLAGS} `PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --libs-only-other libcurl`"
		CURL_LIBS="`PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --libs-only-l libcurl`"

		AM_VARIABLES_STORE

		LDFLAGS="${LDFLAGS} ${CURL_LDFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${CURL_CPPFLAGS}"

		AC_CHECK_LIB([curl], [curl_easy_init], [], [AC_MSG_ERROR([CURL library not found])])
		AC_CHECK_HEADER([curl/curl.h], [], [AC_MSG_ERROR([CURL library headers not found])])

		HAVE_CURL=yes

		AM_VARIABLES_RESTORE

		AC_MSG_NOTICE([CURL environment variables:])
		AC_MSG_NOTICE([  CURL_CFLAGS=${CURL_CFLAGS}])
		AC_MSG_NOTICE([  CURL_CPPFLAGS=${CURL_CPPFLAGS}])
		AC_MSG_NOTICE([  CURL_LDFLAGS=${CURL_LDFLAGS}])
		AC_MSG_NOTICE([  CURL_LIBS=${CURL_LIBS}])

		AC_SUBST([CURL_CFLAGS])
		AC_SUBST([CURL_CPPFLAGS])
		AC_SUBST([CURL_LDFLAGS])
		AC_SUBST([CURL_LIBS])
	fi
])
