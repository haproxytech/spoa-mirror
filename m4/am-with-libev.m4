dnl am-with-libev.m4 by Miroslav Zagorac <mzagorac@haproxy.com>
dnl
AC_DEFUN([AM_WITH_LIBEV], [
	AC_ARG_WITH([libev],
		[AS_HELP_STRING([--with-libev@<:@=DIR@:>@], [use LIBEV library @<:@default=check@:>@])],
		[with_libev="${withval}"],
		[with_libev=check]
	)

	if test "${with_libev}" != "no"; then
		HAVE_LIBEV=
		LIBEV_CFLAGS=
		LIBEV_CPPFLAGS=
		LIBEV_LDFLAGS=
		LIBEV_LIBS=

		AM_PATH_PKGCONFIG([${with_libev}])
		if pkg-config --exists libev; then
			LIBEV_CPPFLAGS="`PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --cflags libev`"
			LIBEV_LDFLAGS="`PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --libs-only-L libev`"
			LIBEV_LDFLAGS="${LIBEV_LDFLAGS} `PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --libs-only-other libev`"
			LIBEV_LIBS="`PKG_CONFIG_PATH=${PKG_CONFIG_PATH} pkg-config --libs-only-l libev`"
		else
			if test -d "${with_libev}"; then
				LIBEV_CPPFLAGS="-I${with_libev}/include"

				if test "${with_libev}" != "/usr"; then
					if test "`uname`" = "Linux"; then
						LIBEV_LDFLAGS="-L${with_libev}/lib -Wl,--rpath,${with_libev}/lib"
					else
						LIBEV_LDFLAGS="-L${with_libev}/lib -R${with_libev}/lib"
					fi
				fi
			fi

			LIBEV_LIBS="-lev"
		fi

		AM_VARIABLES_STORE

		LDFLAGS="${LDFLAGS} ${LIBEV_LDFLAGS}"
		CPPFLAGS="${CPPFLAGS} ${LIBEV_CPPFLAGS}"

		AC_CHECK_LIB([ev], [ev_version_major], [], [AC_MSG_ERROR([LIBEV library not found])])
		AC_CHECK_HEADER([ev.h], [], [AC_MSG_ERROR([LIBEV library headers not found])])

		HAVE_LIBEV=yes

		AM_VARIABLES_RESTORE

		AC_MSG_NOTICE([LIBEV environment variables:])
		AC_MSG_NOTICE([  LIBEV_CFLAGS=${LIBEV_CFLAGS}])
		AC_MSG_NOTICE([  LIBEV_CPPFLAGS=${LIBEV_CPPFLAGS}])
		AC_MSG_NOTICE([  LIBEV_LDFLAGS=${LIBEV_LDFLAGS}])
		AC_MSG_NOTICE([  LIBEV_LIBS=${LIBEV_LIBS}])

		AC_SUBST([LIBEV_CFLAGS])
		AC_SUBST([LIBEV_CPPFLAGS])
		AC_SUBST([LIBEV_LDFLAGS])
		AC_SUBST([LIBEV_LIBS])
	fi
])
