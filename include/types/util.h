/***
 * Copyright 2018,2019 HAProxy Technologies
 *
 * This file is part of spoa-mirror.
 *
 * spoa-mirror is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * spoa-mirror is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef _TYPES_UTIL_H
#define _TYPES_UTIL_H

#define STR_HTTP_PFX         "http://"
#define STR_HTTPS_PFX        "https://"

#ifdef USE_LOG_STARTTIME
#  define LOG_FMT            "[%2d][%12.6f] "
#  define LOG_RUNTIME(a,b)   ((a) / 1000000.0)
#else
#  define LOG_FMT            "[%2d][%17.6f] "
#  define LOG_RUNTIME(a,b)   ((b) / 1000000.0)
#endif

#ifdef DEBUG
#  define LOG_FMT_INDENT     "%.*s"
#  define LOG_INDENT         dbg_indent, "                                                            >>>",
#else
#  define LOG_FMT_INDENT
#  define LOG_INDENT
#endif

#define _F(s)                "(F) " s
#define _E(s)                "(E) " s
#define _W(s)                "(W) " s
#define _I(s)                "(I) " s

#define PARSE_DELAY_US(t)    do { if (retval > (ULLONG_MAX / (t))) errno = ERANGE; else retval *= (t); } while (0)
#define TIMEINT_S(t)         ((t) * 1000000ULL)

enum flag_getopt_enum {
	FLAG_GETOPT_DIST_ERRORS     = 0x01,
	FLAG_GETOPT_POSIXLY_CORRECT = 0x02,
	FLAG_GETOPT_NONOPTION_ARG   = 0x04,
};

struct buffer {
	struct list  list;
	uint8_t     *ptr;
	size_t       len;
	size_t       size;
};

#endif /* _TYPES_UTIL_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
