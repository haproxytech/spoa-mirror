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
 *   spoe_decode_varint - decode a variable-length integer
 *
 * ARGUMENTS
 *   buf   - pointer to the position in the frame data
 *   end   - pointer to the end of the frame data
 *   value - pointer to the decoded value
 *
 * DESCRIPTION
 *   Decode a varint from <*buf> and save the decoded value in <*value>.  See
 *   spoe_encode_varint() for the details about the varint.
 *
 * RETURN VALUE
 *   On success, it returns the number of read bytes and <*buf> is moved after
 *   the varint.  Otherwise, it returns FUNC_RET_ERROR (-1).
 */
static __always_inline int spoe_decode_varint(const char **buf, const char *end, uint64_t *value)
{
	uint8_t *ptr = (uint8_t *)*buf;
	int      shift = 4, retval = FUNC_RET_ERROR;

	if (*buf >= end)
		return retval;

	*value = *(ptr++);
	if (*value >= 240) {
		do {
			if (ptr >= (uint8_t *)end)
				return retval;

			*value += (uint64_t)*ptr << shift;
			shift  += 7;
		} while (*(ptr++) >= 128);
	}

	SPOE_BUFFER_ADVANCE(FUNC_RET_OK);

	return retval;
}


/***
 * NAME
 *   spoe_decode_buffer - decode a buffer
 *
 * ARGUMENTS
 *   buf - pointer to the position in the frame data
 *   end - pointer to the end of the frame data
 *   str - pointer to the first byte of the buffer
 *   len - pointer to the length of the buffer
 *
 * DESCRIPTION
 *   Decode a buffer.  The buffer length is decoded and saved in <*len>, and
 *   <*str> points on the first byte of the buffer.  An empty buffer leaves
 *   <*str> set to a NULL pointer.
 *
 * RETURN VALUE
 *   On success, it returns the buffer length and <*buf> is moved after the
 *   encoded buffer.  Otherwise, it returns FUNC_RET_ERROR (-1).
 */
static __always_inline int spoe_decode_buffer(const char **buf, const char *end, const char **str, uint64_t *len)
{
	const char *ptr = *buf;
	uint64_t    retval;
	int         rc;

	*str = NULL;
	*len = 0;

	/*
	 * The decoded length is compared to the remaining space instead of
	 * computing 'ptr + retval', which may wrap around.
	 */
	rc = spoe_decode_varint(&ptr, end, &retval);
	if (_ERROR(rc) || (retval > (uint64_t)(end - ptr)))
		return FUNC_RET_ERROR;

	if (retval > 0) {
		*str = (char *)ptr;
		*len = retval;
	}

	*buf = ptr + retval;

	return retval;
}


/***
 * NAME
 *   spoe_skip_data - skip a typed data
 *
 * ARGUMENTS
 *   buf - pointer to the position in the frame data
 *   end - pointer to the end of the frame data
 *
 * DESCRIPTION
 *   Skip a typed data.
 *
 *   A typed data is composed of a type (1 byte) and corresponding data:
 *     - boolean: non additional data (0 bytes)
 *     - integers: a variable-length integer (see spoe_decode_varint)
 *     - ipv4: 4 bytes
 *     - ipv6: 16 bytes
 *     - binary and string: a buffer prefixed by its size, a variable-length
 *       integer (see spoe_decode_buffer)
 *
 * RETURN VALUE
 *   If an error occurred, FUNC_RET_ERROR (-1) is returned, otherwise the number
 *   of skipped bytes is returned and the <*buf> is moved after skipped data.
 */
static __always_inline int spoe_skip_data(const char **buf, const char *end)
{
	const char *str, *ptr = *buf;
	uint8_t     type;
	uint64_t    value;
	int         retval = FUNC_RET_ERROR;

	if (ptr >= end)
		return retval;

	type = *(ptr++) & SPOE_DATA_T_MASK;

	if (TEST_OR2(type, SPOE_DATA_T_NULL, SPOE_DATA_T_BOOL)) {
		retval = FUNC_RET_OK;
	}
	else if (TEST_OR4(type, SPOE_DATA_T_INT32, SPOE_DATA_T_INT64, SPOE_DATA_T_UINT32, SPOE_DATA_T_UINT64)) {
		retval = spoe_decode_varint(&ptr, end, &value);
	}
	else if (TEST_OR2(type, SPOE_DATA_T_IPV4, SPOE_DATA_T_IPV6)) {
		ptr += (type == SPOE_DATA_T_IPV4) ? sizeof(struct in_addr) : sizeof(struct in6_addr);
		if (ptr <= end)
			retval = FUNC_RET_OK;
	}
	else if (TEST_OR2(type, SPOE_DATA_T_STR, SPOE_DATA_T_BIN)) {
		/* The entire buffer must be skipped. */
		retval = spoe_decode_buffer(&ptr, end, &str, &value);
	}

	SPOE_BUFFER_ADVANCE(retval);

	return retval;
}


/***
 * NAME
 *   spoe_decode_data - decode a typed data
 *
 * ARGUMENTS
 *   buf  - pointer to the position in the frame data
 *   end  - pointer to the end of the frame data
 *   data - pointer to the decoded data
 *   type - pointer to the type of the decoded data
 *
 * DESCRIPTION
 *   Decode a typed data and save it in <*data>, with its type in <*type>.  See
 *   spoe_skip_data() for the description of a typed data.
 *
 * RETURN VALUE
 *   On success, it returns the number of read bytes and <*buf> is moved after
 *   the decoded data.  Otherwise, it returns FUNC_RET_ERROR (-1).
 */
static __always_inline int spoe_decode_data(const char **buf, const char *end, union spoe_data *data, enum spoe_data_type *type)
{
	const char *str, *ptr = *buf;
	uint64_t    value;
	uint8_t     data_type;
	int         retval = FUNC_RET_ERROR;

	if (ptr >= end)
		return retval;

	data_type = *(ptr++);
	*type     = data_type & SPOE_DATA_T_MASK;

	if (*type == SPOE_DATA_T_NULL) {
		retval = FUNC_RET_OK;
	}
	else if (*type == SPOE_DATA_T_BOOL) {
		data->boolean = (data_type & SPOE_DATA_FL_MASK) == SPOE_DATA_FL_TRUE;

		retval = FUNC_RET_OK;
	}
	else if (*type == SPOE_DATA_T_INT32) {
		retval = spoe_decode_varint(&ptr, end, &value);
		if (_nERROR(retval))
			data->int32 = value;
	}
	else if (*type == SPOE_DATA_T_INT64) {
		retval = spoe_decode_varint(&ptr, end, &value);
		if (_nERROR(retval))
			data->int64 = value;
	}
	else if (*type == SPOE_DATA_T_UINT32) {
		retval = spoe_decode_varint(&ptr, end, &value);
		if (_nERROR(retval))
			data->uint32 = value;
	}
	else if (*type == SPOE_DATA_T_UINT64) {
		retval = spoe_decode_varint(&ptr, end, &value);
		if (_nERROR(retval))
			data->uint64 = value;
	}
	else if (*type == SPOE_DATA_T_IPV4) {
		if ((ptr + sizeof(data->ipv4)) <= end) {
			(void)memcpy(&(data->ipv4), ptr, sizeof(data->ipv4));
			ptr += sizeof(data->ipv4);

			retval = FUNC_RET_OK;
		}
	}
	else if (*type == SPOE_DATA_T_IPV6) {
		if ((ptr + sizeof(data->ipv6)) <= end) {
			(void)memcpy(&(data->ipv6), ptr, sizeof(data->ipv6));
			ptr += sizeof(data->ipv6);

			retval = FUNC_RET_OK;
		}
	}
	else if (TEST_OR2(*type, SPOE_DATA_T_STR, SPOE_DATA_T_BIN)) {
		/* The entire buffer must be decoded. */
		retval = spoe_decode_buffer(&ptr, end, &str, &value);
		if (_nERROR(retval)) {
			data->chk.ptr = (char *)str;
			data->chk.len = value;
		}
	}

	SPOE_BUFFER_ADVANCE(retval);

	return retval;
}


/***
 * NAME
 *   spoe_vdecode - decode the data items of a frame
 *
 * ARGUMENTS
 *   frame - frame that is decoded
 *   buf   - pointer to the position in the frame data
 *   end   - pointer to the end of the frame data
 *   type  - type of the first data item
 *   ap    - list of the remaining arguments
 *
 * DESCRIPTION
 *   Decode the data items that HAProxy sent in the frame <frame>, starting at
 *   the position <*buf>.  Every item is described with its type, one of the
 *   SPOE_DEC_* values, followed by the addresses at which the decoded values
 *   are saved, and the list of the items ends with SPOE_DEC_END.  The status
 *   code of the client is set when an item cannot be decoded, and on success
 *   <*buf> is moved after the decoded data.
 *
 * RETURN VALUE
 *   It returns the number of the read bytes, or FUNC_RET_ERROR (-1) in case of
 *   the error.
 */
static int spoe_vdecode(struct spoe_frame *frame, const char **buf, const char *end, int type, va_list ap)
{
	const char *ptr = *buf;
	uint8_t     data_type;
	int         retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %p, %d, %p", frame, DPTR_ARGS(buf), end, type, ap);

	if (_nNULL(FC_PTR))
		FC_PTR->status_code = SPOE_FRM_ERR_NONE;

	for ( ; _nERROR(retval) && (type != SPOE_DEC_END); type = va_arg(ap, typeof(type))) {
		if (ptr >= end) {
			retval = FUNC_RET_ERROR;
		}
		else if (type == SPOE_DEC_UINT8) {
			uint8_t *addr8 = va_arg(ap, typeof(addr8));

			*addr8 = *(ptr++);
		}
		else if (type == SPOE_DEC_UINT32) {
			uint32_t *addr32 = va_arg(ap, typeof(addr32));

			if (sizeof(*addr32) > (size_t)(end - ptr)) {
				retval = FUNC_RET_ERROR;
			} else {
				/* The frame flags are not aligned in the buffer. */
				(void)memcpy(addr32, ptr, sizeof(*addr32));
				ptr += sizeof(*addr32);
			}
		}
		else if (type == SPOE_DEC_VARINT0) {
			uint64_t *value = va_arg(ap, typeof(value));

			retval = spoe_decode_varint(&ptr, end, value);
		}
		else if (type == SPOE_DEC_VARINT) {
			uint64_t *value = va_arg(ap, typeof(value));

			data_type = *(ptr++) & SPOE_DATA_T_MASK;
			if (TEST_OR4(data_type, SPOE_DATA_T_INT32, SPOE_DATA_T_INT64, SPOE_DATA_T_UINT32, SPOE_DATA_T_UINT64))
				retval = spoe_decode_varint(&ptr, end, value);
			else
				retval = FUNC_RET_ERROR;
		}
		else if (type == SPOE_DEC_STR0) {
			const char **str = va_arg(ap, typeof(str));
			uint64_t    *len = va_arg(ap, typeof(len));

			retval = spoe_decode_buffer(&ptr, end, str, len);
		}
		else if (type == SPOE_DEC_STR) {
			const char **str = va_arg(ap, typeof(str));
			uint64_t    *len = va_arg(ap, typeof(len));

			data_type = *(ptr++) & SPOE_DATA_T_MASK;
			if (data_type == SPOE_DATA_T_STR)
				retval = spoe_decode_buffer(&ptr, end, str, len);
			else
				retval = FUNC_RET_ERROR;
		}
		else if (type == SPOE_DEC_DATA) {
			union spoe_data     *data  = va_arg(ap, typeof(data));
			enum spoe_data_type *dtype = va_arg(ap, typeof(dtype));

			retval = spoe_decode_data(&ptr, end, data, dtype);
		}
		else {
			retval = FUNC_RET_ERROR;
		}

		if (_ERROR(retval))
			f_log(frame, _E("Failed to decode frame, type=%hhu, ptr=%p end=%p"), type, ptr, end);
	}

	SPOE_BUFFER_ADVANCE(retval);

	if (_ERROR(retval) && _nNULL(FC_PTR) && (FC_PTR->status_code == SPOE_FRM_ERR_NONE))
		FC_PTR->status_code = SPOE_FRM_ERR_INVALID;

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   spoe_decode - decode the data items of a frame
 *
 * ARGUMENTS
 *   frame - frame that is decoded
 *   buf   - pointer to the position in the frame data
 *   end   - pointer to the end of the frame data
 *   type  - type of the first data item
 *
 * DESCRIPTION
 *   Decode the data items of the frame <frame>, taking them from the variable
 *   arguments and passing them to spoe_vdecode().
 *
 * RETURN VALUE
 *   It returns the number of the read bytes, or FUNC_RET_ERROR (-1) in case of
 *   the error.
 */
int spoe_decode(struct spoe_frame *frame, const char **buf, const char *end, int type, ...)
{
	va_list ap;
	int     retval;

	va_start(ap, type);
	retval = spoe_vdecode(frame, buf, end, type, ap);
	va_end(ap);

	return retval;
}


/***
 * NAME
 *   spoe_decode_kv_item - decode a key/value item
 *
 * ARGUMENTS
 *   frame   - frame that is decoded
 *   buf     - pointer to the position in the frame data
 *   end     - pointer to the end of the frame data
 *   type    - type of the item value
 *   cb_func - function that is called with the decoded value
 *
 * DESCRIPTION
 *   Decode the value of the key/value item of the type <type>, which is one of
 *   the SPOE_DEC_* values, and call the function <cb_func> with the decoded
 *   value.  The types SPOE_DEC_VARINT0 and SPOE_DEC_STR0 are not supported
 *   here, and neither is an unknown type.  On success <*buf> is moved after the
 *   decoded value.
 *
 * RETURN VALUE
 *   It returns the number of the read bytes, or FUNC_RET_ERROR (-1) in case of
 *   the error.
 */
static int spoe_decode_kv_item(struct spoe_frame *frame, const char **buf, const char *end, int type, spoe_dec_kv_cb_t cb_func)
{
	const char *ptr = *buf;
	uint8_t     data_type;
	int         retval = FUNC_RET_ERROR;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %p, %d, %p", frame, DPTR_ARGS(buf), end, type, cb_func);

	if (ptr >= end) {
		/* Do nothing. */
	}
	else if (type == SPOE_DEC_UINT8) {
		retval = cb_func(frame, (char *)ptr++, NULL);
	}
	else if (type == SPOE_DEC_UINT32) {
		if (sizeof(uint32_t) <= (size_t)(end - ptr)) {
			retval  = cb_func(frame, (char *)ptr, NULL);
			ptr    += sizeof(uint32_t);
		}
	}
	else if (type == SPOE_DEC_VARINT0) {
		/* Do nothing. */
	}
	else if (type == SPOE_DEC_VARINT) {
		uint64_t value;

		data_type = *(ptr++) & SPOE_DATA_T_MASK;
		if (TEST_OR4(data_type, SPOE_DATA_T_INT32, SPOE_DATA_T_INT64, SPOE_DATA_T_UINT32, SPOE_DATA_T_UINT64)) {
			retval = spoe_decode_varint(&ptr, end, &value);
			if (_nERROR(retval))
				retval = cb_func(frame, &value, NULL);
		} else {
			retval = FUNC_RET_ERROR;
		}
	}
	else if (type == SPOE_DEC_STR0) {
		/* Do nothing. */
	}
	else if (type == SPOE_DEC_STR) {
		const char *str;
		uint64_t    len;

		data_type = *(ptr++) & SPOE_DATA_T_MASK;
		if (data_type == SPOE_DATA_T_STR) {
			retval = spoe_decode_buffer(&ptr, end, &str, &len);
			if (_nERROR(retval))
				retval = cb_func(frame, (char *)str, &len);
		} else {
			retval = FUNC_RET_ERROR;
		}
	}
	else if (type == SPOE_DEC_DATA) {
		union spoe_data     data;
		enum spoe_data_type dtype;

		retval = spoe_decode_data(&ptr, end, &data, &dtype);
		if (_nERROR(retval))
			retval = cb_func(frame, &data, &dtype);
	}
	else {
		/* Do nothing. */
	}

	SPOE_BUFFER_ADVANCE(retval);

	if (_ERROR(retval))
		f_log(frame, _E("Failed to decode K/V item"));

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   spoe_decode_kv - decode the key/value items of a frame
 *
 * ARGUMENTS
 *   frame - frame that is decoded
 *   buf   - pointer to the position in the frame data
 *   end   - pointer to the end of the frame data
 *
 * DESCRIPTION
 *   Decode all the key/value items that the frame <frame> holds, between the
 *   positions <*buf> and <end>.  Every known item is described with its type,
 *   its name and the length of the name, and the function that is called with
 *   the decoded value; the list of the items ends with SPOE_DEC_END.  An item
 *   that is not known is skipped, while an item that cannot be decoded, as well
 *   as a frame that holds no item at all, aborts the whole frame.
 *
 * RETURN VALUE
 *   It returns the number of the read bytes, or FUNC_RET_ERROR (-1) in case of
 *   the error.
 */
int spoe_decode_kv(struct spoe_frame *frame, const char **buf, const char *end, ...)
{
	va_list     ap;
	const char *ptr = *buf, *str;
	uint64_t    len;
	int         type, retval = FUNC_RET_ERROR;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %p, ...", frame, DPTR_ARGS(buf), end);

	/* Loop on K/V items. */
	while (ptr < end) {
		/* Decode the item name. */
		retval = spoe_decode_buffer(&ptr, end, &str, &len);
		if (_ERROR(retval) || _NULL(str)) {
			if (_nNULL(FC_PTR))
				FC_PTR->status_code = SPOE_FRM_ERR_INVALID;

			DBG_RETURN_INT(FUNC_RET_ERROR);
		}

		F_DBG(SPOA, frame, "K/V item: key=%.*s", (int)len, str);

		va_start(ap, end);
		type = va_arg(ap, typeof(type));
		for ( ; _nERROR(retval) && (type != SPOE_DEC_END); type = va_arg(ap, typeof(type))) {
			const char             *name    = va_arg(ap, typeof(name));
			uint                    size    = va_arg(ap, typeof(size));
			const spoe_dec_kv_cb_t  cb_func = va_arg(ap, typeof(cb_func));

			if ((len == size) && (memcmp(str, name, len) == 0)) {
				retval = spoe_decode_kv_item(frame, &ptr, end, type, cb_func);

				break;
			}
		}
		va_end(ap);

		/* An item that cannot be decoded aborts the whole frame. */
		if (_ERROR(retval)) {
			if (_nNULL(FC_PTR))
				FC_PTR->status_code = SPOE_FRM_ERR_INVALID;

			DBG_RETURN_INT(FUNC_RET_ERROR);
		}

		/* Silently ignore unknown item. */
		if (type == SPOE_DEC_END) {
			F_DBG(SPOA, frame, "Skip K/V item: key=%.*s", (int)len, str);

			if (_ERROR(spoe_skip_data(&ptr, end))) {
				if (_nNULL(FC_PTR))
					FC_PTR->status_code = SPOE_FRM_ERR_INVALID;

				DBG_RETURN_INT(FUNC_RET_ERROR);
			}
		}
	}

	SPOE_BUFFER_ADVANCE(retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   spoe_decode_skip_msg - skip a message
 *
 * ARGUMENTS
 *   frame - frame that is decoded
 *   buf   - pointer to the position in the frame data
 *   end   - pointer to the end of the frame data
 *
 * DESCRIPTION
 *   Skip the message that starts at the position <*buf>.  The number of the
 *   message arguments is decoded first, and then the name and the value of
 *   every argument are skipped.  On success <*buf> is moved after the skipped
 *   message.
 *
 * RETURN VALUE
 *   It returns the number of the skipped bytes, or FUNC_RET_ERROR (-1) in case
 *   of the error.
 */
int spoe_decode_skip_msg(struct spoe_frame *frame, const char **buf, const char *end)
{
	const char *ptr = *buf, *str;
	uint64_t    len;
	uint8_t     nbargs;
	int         retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %p, ...", frame, DPTR_ARGS(buf), end);

	retval = spoe_decode(frame, &ptr, end, SPOE_DEC_UINT8, &nbargs, SPOE_DEC_END);

	/* Silently ignore arguments: its name and its value. */
	for ( ; _nERROR(retval) && (nbargs > 0); nbargs--) {
		if (_ERROR(spoe_decode_buffer(&ptr, end, &str, &len)))
			retval = FUNC_RET_ERROR;
		else {
			F_DBG(SPOA, frame, "Skip message argument: name=%.*s", (int)len, str);

			if (_ERROR(spoe_skip_data(&ptr, end)))
				retval = FUNC_RET_ERROR;
		}
	}

	SPOE_BUFFER_ADVANCE(retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   spoe_decode_frame - decode a frame received from HAProxy
 *
 * ARGUMENTS
 *   msg         - name of the frame, used in the logged messages
 *   frame       - frame that is decoded
 *   spoe_type   - expected type of the frame
 *   spoe_retval - value returned for a frame of another type
 *   type        - type of the first data item
 *
 * DESCRIPTION
 *   Decode a frame received from HAProxy; its type, flags, stream identifier
 *   and frame identifier, followed by the data items given in the variable
 *   arguments.  A NOTIFY frame sets the identifiers of the frame and marks it
 *   as fragmented when the last fragment is not reached yet.  A fragment has to
 *   belong to the frame that is being assembled, while a HELLO and a DISCONNECT
 *   frame must not be fragmented and must have both identifiers cleared; the
 *   status code of the client is set when that is not the case.
 *
 * RETURN VALUE
 *   It returns the offset at which the frame data that is not decoded starts,
 *   or <spoe_retval> for a frame that is not of the type <spoe_type>.  In case
 *   of the error, FUNC_RET_ERROR (-1) is returned.
 */
int spoe_decode_frame(const char *msg, struct spoe_frame *frame, uint8_t spoe_type, int spoe_retval, int type, ...)
{
	const char *buf, *end;
	uint64_t    stream_id, frame_id;
	uint32_t    flags;
	uint8_t     stype;
	int         retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "\"%s\", %p, %hhu, %d, %d, ...", msg, frame, spoe_type, spoe_retval, type);

	F_DBG(SPOA, frame, "--> %s decoding frame", msg);

	buf = frame->buf;
	end = frame->buf + frame->len;

	retval = spoe_decode(frame, &buf, end,
	                     SPOE_DEC_UINT8, &stype,
	                     SPOE_DEC_UINT32, &flags,
	                     SPOE_DEC_VARINT0, &stream_id,
	                     SPOE_DEC_VARINT0, &frame_id,
	                     SPOE_DEC_END);

	F_DBG(SPOA, frame, "--> %s header %hhu 0x%08x %"PRIu64" %"PRIu64,
	      msg, stype, flags, stream_id, frame_id);

	if (_ERROR(retval))
		DBG_RETURN_INT(retval);
	else if (stype != spoe_type)
		DBG_RETURN_INT(spoe_retval);

	frame->flags  = ntohl(flags);
	frame->offset = retval;

	if (stype == SPOE_FRM_T_HAPROXY_NOTIFY) {
		frame->stream_id  = stream_id;
		frame->frame_id   = frame_id;
		frame->fragmented = !(frame->flags & SPOE_FRM_FL_FIN);
	}

	F_DBG(SPOA, frame, "--> %s stream-id=%u - frame-id=%u",
	      msg, frame->stream_id, frame->frame_id);

	if (stype == SPOE_FRM_T_UNSET) {
		if (!frame->fragmented
		    || (frame->stream_id != stream_id)
		    || (frame->frame_id  != frame_id)) {
			if (_nNULL(FC_PTR))
				FC_PTR->status_code = SPOE_FRM_ERR_INTERLACED_FRAMES;

			DBG_RETURN_INT(FUNC_RET_ERROR);
		}
	}
	else if (TEST_OR2(stype, SPOE_FRM_T_HAPROXY_DISCON, SPOE_FRM_T_HAPROXY_HELLO)) {
		/* Fragmentation is not supported. */
		if ((frame->flags & SPOE_FRM_FL_FIN) == 0) {
			if (_nNULL(FC_PTR))
				FC_PTR->status_code = SPOE_FRM_ERR_FRAG_NOT_SUPPORTED;

			DBG_RETURN_INT(FUNC_RET_ERROR);
		}

		/* stream-id and frame-id must be cleared. */
		if ((stream_id != 0) || (frame_id != 0)) {
			if (_nNULL(FC_PTR))
				FC_PTR->status_code = SPOE_FRM_ERR_INVALID;

			DBG_RETURN_INT(FUNC_RET_ERROR);
		}
	}

	if (type != SPOE_DEC_END) {
		va_list ap;

		va_start(ap, type);
		retval = spoe_vdecode(frame, &buf, end, type, ap);
		va_end(ap);

		if (_nERROR(retval))
			frame->offset += retval;
	}

	if (_nERROR(retval))
		retval = frame->offset;
	else
		f_log(frame, _E("--> %s Failed to decode frame"), msg);

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
