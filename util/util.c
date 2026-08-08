/***
 * Copyright 2023 HAProxy Technologies
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
#include "include.h"
#include "util.h"


bool_t flag_log_nl = 0;


/***
 * NAME
 *   f_log - write a log message
 *
 * ARGUMENTS
 *   frame  - frame for which the message is written, not used
 *   format - printf(3) style format of the message
 *
 * DESCRIPTION
 *   Write the message <format> to the standard output.  A newline is added to
 *   the message when the flag flag_log_nl is set, as it is while the header of
 *   a frame is decoded.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void f_log(const struct spoe_frame *frame __maybe_unused, const char *format, ...)
{
	va_list     ap;
	const char *ptr = format;
	char        fmt[BUFSIZ];

	if (flag_log_nl) {
		(void)snprintf(fmt, sizeof(fmt), "%s\n", format);

		ptr = fmt;
	}

	va_start(ap, format);
	(void)vfprintf(stdout, ptr, ap);
	va_end(ap);
}


/***
 * NAME
 *   w_log - write a worker log message
 *
 * ARGUMENTS
 *   worker - worker for which the message is written, not used
 *   format - printf(3) style format of the message, not used
 *
 * DESCRIPTION
 *   Do nothing.  The function only replaces the one of the program, so that the
 *   sources which decode the frames can be linked with this utility.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void w_log(const struct worker *worker __maybe_unused, const char *format __maybe_unused, ...)
{
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
