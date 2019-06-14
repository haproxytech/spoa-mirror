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
#ifndef _TYPES_SPOE_DECODE_H
#define _TYPES_SPOE_DECODE_H

enum SPOE_DEC_type {
	SPOE_DEC_UINT8 = 0,
	SPOE_DEC_UINT32,
	SPOE_DEC_VARINT0,
	SPOE_DEC_VARINT,
	SPOE_DEC_STR0,
	SPOE_DEC_STR,
	SPOE_DEC_DATA,
	SPOE_DEC_END,
};

typedef int (*spoe_dec_kv_cb_t)(struct spoe_frame *frame, void *arg1, void *arg2);

#endif /* _TYPES_SPOE_DECODE_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
