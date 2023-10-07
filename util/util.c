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
 *   f_log -
 *
 * ARGUMENTS
 *   frame  -
 *   format -
 *
 * DESCRIPTION
 *   -
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
 *   w_log -
 *
 * ARGUMENTS
 *   worker -
 *   format -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void w_log(const struct worker *worker __maybe_unused, const char *format __maybe_unused, ...)
{
}


/***
 * NAME
 *   str_hex -
 *
 * ARGUMENTS
 *   data -
 *   size -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
const char *str_hex(const void *data, size_t size)
{
	static __THR char  retbuf[BUFSIZ];
	const uint8_t     *ptr = data;
	size_t             i;

	if (_NULL(data))
		return "(null)";
	else if (size == 0)
		return "()";

	for (i = 0, size <<= 1; (i < SIZEOF_N(retbuf, 2)) && (i < size); ptr++) {
		retbuf[i++] = NIBBLE_TO_HEX(*ptr >> 4);
		retbuf[i++] = NIBBLE_TO_HEX(*ptr & 0x0f);
	}

	retbuf[i] = '\0';

	return retbuf;
}


/***
 * NAME
 *   str_ctrl -
 *
 * ARGUMENTS
 *   data -
 *   size -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
const char *str_ctrl(const void *data, size_t size)
{
	static __THR char  retbuf[BUFSIZ];
	const uint8_t     *ptr = data;
	size_t             i, n = 0;

	if (_NULL(data))
		return "(null)";
	else if (size == 0)
		return "()";

	for (i = 0; (n < SIZEOF_N(retbuf, 1)) && (i < size); i++)
		retbuf[n++] = IN_RANGE(ptr[i], 0x20, 0x7e) ? ptr[i] : '.';

	retbuf[n] = '\0';

	return retbuf;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
