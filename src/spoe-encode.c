/***
 * Copyright 2018-2020 HAProxy Technologies
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
 *   spoe_encode_varint -
 *
 * ARGUMENTS
 *   value -
 *   buf   -
 *   end   -
 *
 * DESCRIPTION
 *   Encode the integer <value> into a varint (variable-length integer).
 *   The encoded value is copied in <*buf>.  Here is the encoding format:
 *
 *   X = [       0,        240): 1 byte  (7.875 bits)  [ XXXX XXXX ]
 *   X = [     240,       2288): 2 bytes (11 bits)     [ 1111 XXXX ] [ 0XXX XXXX ]
 *   X = [    2288,     264432): 3 bytes (18 bits)     [ 1111 XXXX ] [ 1XXX XXXX ]   [ 0XXX XXXX ]
 *   X = [  264432,   33818864): 4 bytes (25 bits)     [ 1111 XXXX ] [ 1XXX XXXX ]*2 [ 0XXX XXXX ]
 *   X = [33818864, 4328786160): 5 bytes (32 bits)     [ 1111 XXXX ] [ 1XXX XXXX ]*3 [ 0XXX XXXX ]
 *   ...
 *
 * RETURN VALUE
 *   On success, it returns the number of written bytes and <*buf> is moved
 *   after the encoded value.  Otherwise, it returns FUNC_RET_ERROR (-1).
 */
static __always_inline int spoe_encode_varint(uint64_t value, char **buf, const char *end)
{
	uint8_t *ptr = (uint8_t *)*buf;
	int      retval = FUNC_RET_ERROR;

	if (ptr >= (uint8_t *)end) {
		return retval;
	}
	else if (value >= 240) {
		*(ptr++) = (uint8_t)value | 240;

		for (value = (value - 240) >> 4; value >= 128; value = (value - 128) >> 7) {
			if (ptr >= (uint8_t *)end)
				return retval;

			*(ptr++) = (uint8_t)value | 128;
		}

		if (ptr >= (uint8_t *)end)
			return retval;
	}

	*(ptr++) = value;

	SPOE_BUFFER_ADVANCE(FUNC_RET_OK);

	return retval;
}


/***
 * NAME
 *   spoe_encode_buffer -
 *
 * ARGUMENTS
 *   str -
 *   len -
 *   buf -
 *   end -
 *
 * DESCRIPTION
 *   Encode a buffer.  Its length <len> is encoded as a varint, followed by a
 *   copy of <str>.  It must have enough space in <*buf> to encode the buffer,
 *   else an error is triggered.
 *
 * RETURN VALUE
 *   On success, it returns <len> and <*buf> is moved after the encoded value.
 *   If an error occurred, it returns FUNC_RET_ERROR (-1).
 */
static __always_inline int spoe_encode_buffer(const char *str, size_t len, char **buf, const char *end)
{
	char *ptr = *buf;
	int   retval = FUNC_RET_ERROR;

	if (ptr >= end) {
		/* Do nothing. */
	}
	else if (len == 0) {
		*(ptr++) = 0;
		*buf     = ptr;

		retval = 0;
	}
	else if (_ERROR(spoe_encode_varint(len, &ptr, end)) || ((ptr + len) > end)) {
		/* Do nothing. */
	}
	else {
		(void)memcpy(ptr, str, len);
		*buf = ptr + len;

		retval = len;
	}

	return retval;
}


/***
 * NAME
 *   spoe_vencode -
 *
 * ARGUMENTS
 *   frame -
 *   buf   -
 *   type  -
 *   ap    -
 *
 * DESCRIPTION
 *   Encode a data to send it to HAProxy.
 *
 * RETURN VALUE
 *   It returns the number of written bytes,
 *   or FUNC_RET_ERROR (-1) in case of the error.
 */
static int spoe_vencode(struct spoe_frame *frame, char **buf, int type, va_list ap)
{
	char       *ptr = *buf;
	const char *end;
	int         retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %d, %p", frame, DPTR_ARGS(buf), type, ap);

	end = frame->buf + cfg.max_frame_size;

	for ( ; _nERROR(retval) && (type != SPOE_ENC_END); type = va_arg(ap, typeof(type))) {
		if (type == SPOE_ENC_UINT8) {
			*(ptr++) = va_arg(ap, int);
		}
		else if (type == SPOE_ENC_VARINT) {
			retval = spoe_encode_varint(va_arg(ap, uint), &ptr, end);
		}
		else if (type == SPOE_ENC_STR) {
			const char *str = va_arg(ap, typeof(str));
			uint        len = va_arg(ap, typeof(len));

			retval = spoe_encode_buffer(str, len, &ptr, end);
		}
		else if (type == SPOE_ENC_KV) {
			const char *str  = va_arg(ap, typeof(str));
			uint        len  = va_arg(ap, typeof(len));
			int         data = va_arg(ap, typeof(data));

			retval = spoe_encode_buffer(str, len, &ptr, end);
			if (_ERROR(retval))
				break;

			*(ptr++) = data;
			if (data == SPOE_DATA_T_UINT32) {
				retval = spoe_encode_varint(va_arg(ap, uint32_t), &ptr, end);
			}
			else if (data == SPOE_DATA_T_STR) {
				str = va_arg(ap, typeof(str));
				len = va_arg(ap, typeof(len));

				retval = spoe_encode_buffer(str, len, &ptr, end);
			}
			else {
				retval = FUNC_RET_ERROR;
			}
		}
		else {
			retval = FUNC_RET_ERROR;
		}
	}

	frame->len = ptr - frame->buf;

	SPOE_BUFFER_ADVANCE(retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   spoe_encode -
 *
 * ARGUMENTS
 *   frame -
 *   buf   -
 *   type  -
 *
 * DESCRIPTION
 *   Encode a data to send it to HAProxy.
 *
 * RETURN VALUE
 *   It returns the number of written bytes,
 *   or FUNC_RET_ERROR (-1) in case of the error.
 */
int spoe_encode(struct spoe_frame *frame, char **buf, int type, ...)
{
	va_list ap;
	int     retval;

	va_start(ap, type);
	retval = spoe_vencode(frame, buf, type, ap);
	va_end(ap);

	return retval;
}


/***
 * NAME
 *   spoe_encode_frame -
 *
 * ARGUMENTS
 *   msg       -
 *   frame     -
 *   spoa_type -
 *   spoe_type -
 *   flags     -
 *   type      -
 *
 * DESCRIPTION
 *   Encode a frame to send it to HAProxy.
 *
 * RETURN VALUE
 *   It returns the number of written bytes,
 *   or FUNC_RET_ERROR (-1) in case of the error.
 */
int spoe_encode_frame(const char *msg, struct spoe_frame *frame, uint8_t spoa_type, uint8_t spoe_type, uint32_t flags, int type, ...)
{
	/* Be careful here, in async mode, frame->client can be NULL. */
	va_list  ap;
	char    *buf;
	int      retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "\"%s\", %p, %hhu, %hhu, 0x%08x, %d, ...", msg, frame, spoa_type, spoe_type, flags, type);

	F_DBG(SPOA, frame, "<-- %s encoding frame", msg);

	frame->type = spoa_type;

	buf = frame->buf;

	/* Frame type */
	*(buf++) = spoe_type;

	/* Set flags */
	*(uint32_t *)buf = htonl(flags);
	buf += sizeof(uint32_t);

	va_start(ap, type);
	retval = spoe_vencode(frame, &buf, type, ap);
	va_end(ap);

	F_DBG(SPOA, frame, "<-- %s stream-id=%u - frame-id=%u", msg, frame->stream_id, frame->frame_id);

	if (_nERROR(retval))
		retval = frame->len;
	else
		f_log(frame, _E("<-- %s Failed to encode frame"), msg);

	DBG_RETURN_INT(retval);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
