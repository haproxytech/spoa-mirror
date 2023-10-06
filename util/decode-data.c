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
#include <stdio.h>
#include <libgen.h>
#include <ctype.h>
#include <string.h>
#include <sysexits.h>


#define IN_RANGE(n,a,b)   (((n) >= (a)) && ((n) <= (b)))
#define STR_NIBBLE(a)     (IN_RANGE((a), '0', '9') ? ((a) - '0') : (IN_RANGE((a), 'a', 'f') ? ((a) - 'a' + 10) : (IN_RANGE((a), 'A', 'F') ? ((a) - 'A' + 10) : 0)))
#define STR_HEX(a,b)      ((STR_NIBBLE(a) << 4) | STR_NIBBLE(b))
#define STR_IDX(a,b,c)    ((a) + (b) * 2 + (c))


/***
 * NAME
 *   usage -
 *
 * ARGUMENTS
 *   program_name -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void usage(const char *program_name)
{
	(void)printf("\nUsage: %s data\n\n", program_name);
	(void)printf("Copyright 2023 HAProxy Technologies\n");
	(void)printf("SPDX-License-Identifier: GPL-2.0-or-later\n\n");
}


/***
 * NAME
 *   decode_data -
 *
 * ARGUMENTS
 *   data -
 *   len  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int decode_data(const char *data, size_t len)
{
	char buffer[BUFSIZ];
	int  i, j = 0, ch = 0;

	if (data == NULL)
		return -1;
	else if (len == 0)
		return 0;

	for (i = 0; i < len; i += (j > 0) ? (j * 2) : 2) {
		for (j = 0; j < (sizeof(buffer) - 1); j++) {
			if (STR_IDX(i, j, 0) == len) {
				/* End of data. */
				break;
			}
			else if (STR_IDX(i, j, 1) == len) {
				/* Single byte hex data must have two digits. */
				(void)printf("0x%04x: %.1s -- invalid hex value\n", i >> 1, data + STR_IDX(i, j, 0));

				return -1;
			}
			else {
				/* All printable data will be copied to the buffer. */
				ch = STR_HEX(data[STR_IDX(i, j, 0)], data[STR_IDX(i, j, 1)]);

				if (!isprint(ch))
					break;

				buffer[j] = ch;
			}
		}

		/* Buffer or one byte printing. */
		if (j > 0)
			(void)printf("0x%04x: %.*s\n", i >> 1, j, buffer);
		else
			(void)printf("0x%04x: 0x%02x %d\n", i >> 1, ch, ch);
	}

	return 0;
}


/***
 * NAME
 *   main -
 *
 * ARGUMENTS
 *   argv -
 *   argc -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
int main(int argc, char **argv)
{
	const char *program_name;
	int         retval = EX_USAGE;

	program_name = basename(argv[0]);

	if (argc == 2)
		retval = decode_data(argv[1], strlen(argv[1]));
	else
		usage(program_name);

	return retval;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
