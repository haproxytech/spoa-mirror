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
 *   check_proto_version_cb -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   Check the protocol version.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   the number of read bytes otherwise.
 */
static int check_proto_version_cb(struct spoe_frame *frame __maybe_unused, void *arg1, void *arg2 __maybe_unused)
{
	const char *str = arg1;
#ifdef DEBUG
	uint        len = *(uint64_t *)arg2;
#endif
	int         retval = _NULL(str) ? FUNC_RET_ERROR : FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, arg1, arg2);

	if (_nNULL(str))
		F_DBG(SPOA, frame, "--> HAPROXY-HELLO supported versions: %.*s", len, str);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   check_max_frame_size_cb -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   Check max frame size value.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   the number of read bytes otherwise.
 */
static int check_max_frame_size_cb(struct spoe_frame *frame, void *arg1, void *arg2 __maybe_unused)
{
	uint64_t size = *(uint64_t *)arg1;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, arg1, arg2);

	/* Keep the lower value. */
	if (size < FC_PTR->max_frame_size)
		FC_PTR->max_frame_size = size;

	F_DBG(SPOA, frame, "--> HAPROXY-HELLO maximum frame size: %"PRIu64, size);

	DBG_RETURN_INT(FUNC_RET_OK);
}


/***
 * NAME
 *   check_healthcheck_cb -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   Check healthcheck value.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   the number of read bytes otherwise.
 */
static int check_healthcheck_cb(struct spoe_frame *frame, void *arg1, void *arg2 __maybe_unused)
{
	uint8_t value = *(uint8_t *)arg1;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, arg1, arg2);

	/* Get the "healthcheck" value */
	frame->hcheck = (value & SPOE_DATA_FL_TRUE) == SPOE_DATA_FL_TRUE;

	F_DBG(SPOA, frame, "--> HAPROXY-HELLO healthcheck: %s", STR_BOOL(frame->hcheck));

	DBG_RETURN_INT(FUNC_RET_OK);
}


/***
 * NAME
 *   check_capabilities_cb -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   Check the capabilities value.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   the number of read bytes otherwise.
 */
static int check_capabilities_cb(struct spoe_frame *frame, void *arg1, void *arg2)
{
	const char *str = arg1;
	uint        len = _NULL(str) ? SIZEOF_N(STR_CAP_NONE, 1) : *(uint64_t *)arg2;
	int         retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, arg1, arg2);

	F_DBG(SPOA, frame, "--> HAPROXY-HELLO capabilities check: %.*s", len, PTR_SAFE(str, STR_CAP_NONE));

	/*
	 * This is not an error.
	 *
	 * In the case of using health checking, this function is called with
	 * argument arg1 which is a NULL pointer.
	 */
	if (_NULL(str))
		DBG_RETURN_INT(retval);

	while (len > 0) {
		/* Skip leading spaces. */
		for ( ; (len > 0) && (*str == ' '); len--, str++);

		if ((len >= STR_SIZE(STR_CAP_FRAGMENTATION)) && (memcmp(str, STR_ADDRSIZE(STR_CAP_FRAGMENTATION)) == 0)) {
			str += STR_SIZE(STR_CAP_FRAGMENTATION);
			len -= STR_SIZE(STR_CAP_FRAGMENTATION);

			if ((len == 0) || TEST_OR2(*str, ' ', ','))
				FC_PTR->fragmentation = true;
		}
		else if ((len >= STR_SIZE(STR_CAP_PIPELINING)) && (memcmp(str, STR_ADDRSIZE(STR_CAP_PIPELINING)) == 0)) {
			str += STR_SIZE(STR_CAP_PIPELINING);
			len -= STR_SIZE(STR_CAP_PIPELINING);

			if ((len == 0) || TEST_OR2(*str, ' ', ','))
				FC_PTR->pipelining = true;
		}
		else if ((len >= STR_SIZE(STR_CAP_ASYNC)) && (memcmp(str, STR_ADDRSIZE(STR_CAP_ASYNC)) == 0)) {
			str += STR_SIZE(STR_CAP_ASYNC);
			len -= STR_SIZE(STR_CAP_ASYNC);

			if ((len == 0) || TEST_OR2(*str, ' ', ','))
				FC_PTR->async = true;
		}

		/* Skip trailing spaces. */
		for ( ; (len > 0) && (*str == ' '); len--, str++);

		if ((len > 0) && (*str == ',')) {
			str++;
			len--;
		} else {
			retval = (len == 0) ? FUNC_RET_OK : FUNC_RET_ERROR;

			break;
		}
	}

	if (FC_PTR->pipelining || FC_PTR->async || FC_PTR->fragmentation)
		F_DBG(SPOA, frame, "--> HAPROXY-HELLO capabilities set: %s %s %s",
		      FC_PTR->pipelining ? STR_CAP_PIPELINING : "",
		      FC_PTR->async ? STR_CAP_ASYNC : "",
		      FC_PTR->fragmentation ? STR_CAP_FRAGMENTATION : "");

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   check_engine_id_cb -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   Check the engine-id value.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   the number of read bytes otherwise.
 */
static int check_engine_id_cb(struct spoe_frame *frame, void *arg1, void *arg2)
{
	const char *str = arg1;
	uint        len = *(uint64_t *)arg2;
	int         retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, arg1, arg2);

	if (_nNULL(str) && _NULL(FC_PTR->engine_id)) {
		FC_PTR->engine_id = mem_dup(str, len);
		if (_NULL(FC_PTR->engine_id))
			retval = FUNC_RET_ERROR;
	}

	F_DBG(SPOA, frame, "--> HAPROXY-HELLO engine id: %.*s", len, str);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   use_spoe_engine -
 *
 * ARGUMENTS
 *   client -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void use_spoe_engine(struct client *client)
{
	struct spoe_engine *e;

	DBG_FUNC(CW_PTR, "%p", client);

	if (_NULL(client->engine_id))
		DBG_RETURN();

	list_for_each_entry(e, &(CW_PTR->engines), list)
		if (strcmp(e->id, client->engine_id) == 0)
			goto end;

	if (_NULL(e = calloc(1, sizeof(*e)))) {
		client->async = false;

		c_log(client, _E("--> HAPROXY-HELLO Failed to allocate memory: %m"));

		DBG_RETURN();
	}

	e->id = strdup(client->engine_id);
	LIST_INIT(&(e->clients));
	LIST_INIT(&(e->processing_frames));
	LIST_INIT(&(e->outgoing_frames));
	LIST_ADDQ(&(CW_PTR->engines), &(e->list));

	C_DBG(SPOA, client, "--> HAPROXY-HELLO add new SPOE engine '%s'", e->id);

end:
	client->engine = e;
	LIST_ADDQ(&(e->clients), &(client->by_engine));

	DBG_RETURN();
}


/***
 * NAME
 *   handle_hahello -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   Decode a HELLO frame received from HAProxy.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred, otherwise the number
 *   of read bytes.  HELLO frame cannot be ignored and having another frame
 *   than a HELLO frame is an error.
 */
int handle_hahello(struct spoe_frame *frame)
{
	const char *buf, *end;
	int         retval;

	DBG_FUNC(FW_PTR, "%p", frame);

	/* Check frame type: we really want a HELLO frame. */
	retval = spoe_decode_frame("HAPROXY-HELLO", frame, SPOE_FRM_T_HAPROXY_HELLO, FUNC_RET_ERROR, SPOE_DEC_END);
	if (_ERROR(retval))
		DBG_RETURN_INT(retval);

	buf = frame->buf + retval;
	end = frame->buf + frame->len;

	/* Decode and check the K/V items. */
	retval = spoe_decode_kv(frame, &buf, end,
	                     SPOE_DEC_STR, STR_ADDRSIZE("supported-versions"), check_proto_version_cb,
	                     SPOE_DEC_VARINT, STR_ADDRSIZE("max-frame-size"), check_max_frame_size_cb,
	                     SPOE_DEC_UINT8, STR_ADDRSIZE("healthcheck"), check_healthcheck_cb,
	                     SPOE_DEC_STR, STR_ADDRSIZE("capabilities"), check_capabilities_cb,
	                     SPOE_DEC_STR, STR_ADDRSIZE("engine-id"), check_engine_id_cb,
	                     SPOE_DEC_END);
	if (_ERROR(retval))
		DBG_RETURN_INT(retval);

	retval = buf - frame->buf;

	if (!(cfg.cap_flags & FLAG_CAP_ASYNC) || _NULL(FC_PTR->engine_id))
		FC_PTR->async = false;
	if (!(cfg.cap_flags & FLAG_CAP_PIPELINING))
		FC_PTR->pipelining = false;
	if (FC_PTR->async)
		use_spoe_engine(FC_PTR);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   prepare_agenthello -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   Encode a HELLO frame to send it to HAProxy.
 *
 * RETURN VALUE
 *   It returns the number of written bytes,
 *   or FUNC_RET_ERROR (-1) in case of the error.
 */
int prepare_agenthello(struct spoe_frame *frame)
{
	char cap[BUFSIZ], *ptr = cap;
	int  retval;

	DBG_FUNC(FW_PTR, "%p", frame);

	if (cfg.cap_flags & FLAG_CAP_FRAGMENTATION) {
		(void)memcpy(ptr, STR_ADDRSIZE(STR_CAP_FRAGMENTATION));
		ptr += STR_SIZE(STR_CAP_FRAGMENTATION);
	}
	if (FC_PTR->pipelining) {
		if (cfg.cap_flags & FLAG_CAP_FRAGMENTATION)
			*(ptr++) = ',';
		(void)memcpy(ptr, STR_ADDRSIZE(STR_CAP_PIPELINING));
		ptr += STR_SIZE(STR_CAP_PIPELINING);
	}
	if (FC_PTR->async) {
		if ((cfg.cap_flags & FLAG_CAP_FRAGMENTATION) || FC_PTR->pipelining)
			*(ptr++) = ',';
		(void)memcpy(ptr, STR_ADDRSIZE(STR_CAP_ASYNC));
		ptr += STR_SIZE(STR_CAP_ASYNC);
	}

	/*
	 * Set AGENT-HELLO frame type, flags, stream-id and frame-id.
	 * There are also 3 mandatory K/V items: "version", "max-frame-size"
	 * and "capabilities".
	 */
	retval = spoe_encode_frame("AGENT-HELLO", frame,
	                           SPOA_FRM_T_AGENT, SPOE_FRM_T_AGENT_HELLO, SPOE_FRM_FL_FIN,
	                           SPOE_ENC_UINT8, 0,
	                           SPOE_ENC_UINT8, 0,
	                           SPOE_ENC_KV, STR_ADDRSIZE("version"), SPOE_DATA_T_STR, STR_ADDRSIZE(SPOP_VERSION),
	                           SPOE_ENC_KV, STR_ADDRSIZE("max-frame-size"), SPOE_DATA_T_UINT32, FC_PTR->max_frame_size,
	                           SPOE_ENC_KV, STR_ADDRSIZE("capabilities"), SPOE_DATA_T_STR, cap, (int)(ptr - cap),
	                           SPOE_ENC_END);

	F_DBG(SPOA, frame, "<-- AGENT-HELLO version: %s", SPOP_VERSION);
	F_DBG(SPOA, frame, "<-- AGENT-HELLO maximum frame size: %u", FC_PTR->max_frame_size);
	F_DBG(SPOA, frame, "<-- AGENT-HELLO capabilities: %.*s", (int)(ptr - cap), cap);

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
