dnl am-host.m4 by Miroslav Zagorac <mzagorac@haproxy.com>
dnl
AC_DEFUN([AM_HOST], [
	dnl Get current date and time.
	dnl
	DATE=`date`

	dnl Get cannonical host.
	dnl
	AC_CANONICAL_HOST
	AC_DEFINE_UNQUOTED([OSTYPE], ["${host}"], [Guessed OS type.])

	dnl Posix variants
	dnl
	AC_USE_SYSTEM_EXTENSIONS

	dnl because ${${host_os%-*}##*-} does not work on stupid bash...
	dnl
	_host_os=${host_os%-*}
	_host_os=${_host_os##*-}

	case "${host_os}" in
	  *osf*)
		test "${stdc_defined}" = "yes" || \
			AC_DEFINE([__STDC__], [1], [Define to 1 if you use ANSI C compiler.])
		AC_DEFINE([_REENTRANT], [1], [Define to 1 only for OSF Unix.])
		AC_DEFINE([ANSI_FUNC], [1], [Define to 1 if you use ANSI C compiler.])
		AC_DEFINE([OSF], [1], [Define to 1 for OSF Unix.])
		;;

	  *solaris*)
		AC_DEFINE([ANSI_FUNC], [1], [Define to 1 if you use ANSI C compiler.])
		AC_DEFINE([SOLARIS], [1], [Define to 1 for Solaris UNIX system.])
		;;

	  *sunos*)
		AC_DEFINE([SUNOS], [1], [Define to 1 for SUNOS UNIX system.])
		;;

	  *linux*)
		AC_DEFINE([LINUX], [1], [Define to 1 for Linux system.])
		;;
	esac
])
