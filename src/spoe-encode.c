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
 *   spoe_encode_varint - encode a variable-length integer
 *
 * ARGUMENTS
 *   value - value that is encoded
 *   buf   - pointer to the position in the frame data
 *   end   - pointer to the end of the frame buffer
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
 *   spoe_encode_buffer - encode a buffer
 *
 * ARGUMENTS
 *   str - buffer that is encoded
 *   len - length of the buffer
 *   buf - pointer to the position in the frame data
 *   end - pointer to the end of the frame buffer
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
 *   spoe_vencode - encode the data items of a frame
 *
 * ARGUMENTS
 *   frame - frame that is encoded
 *   buf   - pointer to the position in the frame data
 *   type  - type of the first data item
 *   ap    - list of the remaining arguments
 *
 * DESCRIPTION
 *   Encode the data items that are sent to HAProxy into the buffer of the frame
 *   <frame>, starting at the position <*buf>.  Every item is described with its
 *   type, one of the SPOE_ENC_* values, followed by the values that have to be
 *   encoded; the list of the items ends with SPOE_ENC_END.  The length of the
 *   frame is updated, and on success <*buf> is moved after the encoded data.
 *
 * RETURN VALUE
 *   It returns the number of written bytes, or FUNC_RET_ERROR (-1) in case of
 *   the error.
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
 *   spoe_encode - encode the data items of a frame
 *
 * ARGUMENTS
 *   frame - frame that is encoded
 *   buf   - pointer to the position in the frame data
 *   type  - type of the first data item
 *
 * DESCRIPTION
 *   Encode the data items of the frame <frame>, taking them from the variable
 *   arguments and passing them to spoe_vencode().
 *
 * RETURN VALUE
 *   It returns the number of written bytes, or FUNC_RET_ERROR (-1) in case of
 *   the error.
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
 *   spoe_encode_frame - encode a frame to send it to HAProxy
 *
 * ARGUMENTS
 *   msg       - name of the frame, used in the logged messages
 *   frame     - frame that is encoded
 *   spoa_type - type that is saved in the frame structure
 *   spoe_type - type of the frame that is sent
 *   flags     - flags of the frame that is sent
 *   type      - type of the first data item
 *
 * DESCRIPTION
 *   Encode a frame to send it to HAProxy; the frame type <spoe_type> and the
 *   flags <flags> are written first, followed by the data items given in the
 *   variable arguments.  The type <spoa_type> is saved in the frame structure,
 *   and the name <msg> is used in the logged messages only.
 *
 * RETURN VALUE
 *   It returns the length of the encoded frame, or FUNC_RET_ERROR (-1) in case
 *   of the error.
 */
int spoe_encode_frame(const char *msg, struct spoe_frame *frame, uint8_t spoa_type, uint8_t spoe_type, uint32_t flags, int type, ...)
{
	/* Be careful here, in async mode, frame->client can be NULL. */
	va_list  ap;
	char    *buf;
	uint32_t netflags;
	int      retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "\"%s\", %p, %hhu, %hhu, 0x%08x, %d, ...", msg, frame, spoa_type, spoe_type, flags, type);

	F_DBG(SPOA, frame, "<-- %s encoding frame", msg);

	frame->type = spoa_type;

	buf = frame->buf;

	/* Frame type */
	*(buf++) = spoe_type;

	/* Set flags; they are not aligned in the buffer. */
	netflags = htonl(flags);
	(void)memcpy(buf, &netflags, sizeof(netflags));
	buf += sizeof(netflags);

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
