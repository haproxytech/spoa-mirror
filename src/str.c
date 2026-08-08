/***
 * Copyright 2018-2026 HAProxy Technologies
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


/***
 * NAME
 *   str_hex - convert data to a hexadecimal string
 *
 * ARGUMENTS
 *   data - pointer to the data that is converted
 *   size - size of the data, in bytes
 *
 * DESCRIPTION
 *   Convert the data pointed to by <data> into a hexadecimal string, written
 *   into a static buffer.  Each byte of the data is shown as a pair of the
 *   hexadecimal digits, and the result is truncated if the buffer is too small.
 *
 * RETURN VALUE
 *   It returns a pointer to the converted string, "(null)" if <data> is a NULL
 *   pointer, or "()" if <size> is 0.
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
 *   str_ctrl - convert data to a printable string
 *
 * ARGUMENTS
 *   data - pointer to the data that is converted
 *   size - size of the data, in bytes
 *
 * DESCRIPTION
 *   Copy the data pointed to by <data> into a static buffer, replacing all
 *   characters that cannot be printed with a dot.  The result is truncated if
 *   the buffer is too small.
 *
 * RETURN VALUE
 *   It returns a pointer to the converted string, "(null)" if <data> is a NULL
 *   pointer, or "()" if <size> is 0.
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
