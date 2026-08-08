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
#ifndef _DECODE_DATA_H
#define _DECODE_DATA_H

#define STR_NIBBLE(a)    (IN_RANGE((a), '0', '9') ? ((a) - '0') : (IN_RANGE((a), 'a', 'f') ? ((a) - 'a' + 10) : (IN_RANGE((a), 'A', 'F') ? ((a) - 'A' + 10) : 0)))
#define STR_HEX(a,b)     ((STR_NIBBLE(a) << 4) | STR_NIBBLE(b))
#define STR_IDX(a,b,c)   ((a) + (b) * 2 + (c))

/* The configuration and the input data of the decoding program. */
struct _prg_data {
	const char *name;       /* The program name. */
	uint8_t     opt_flags;  /* Options set on the command line. */
	const char *bin_data;   /* The hexadecimal data to write as strings. */
	const char *frame_data; /* The hexadecimal data of the frame to decode. */
};

#endif /* _DECODE_DATA_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
