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
 *   check_discon_status_code_cb -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   Check disconnect status code.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   the number of read bytes otherwise.
 */
static int check_discon_status_code_cb(struct spoe_frame *frame __maybe_unused, void *arg1 __maybe_unused, void *arg2 __maybe_unused)
{
#ifdef DEBUG
	/* Get the "status-code" value. */
	uint64_t status = *(uint64_t *)arg1;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, arg1, arg2);

	F_DBG(SPOA, frame, "--> HAPROXY-DISCONNECT status code: %d (%s)", (int)status, spoe_frm_err_reasons(status));
#endif

	DBG_RETURN_INT(FUNC_RET_OK);
}


/***
 * NAME
 *   check_discon_message_cb -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   Check the disconnect message.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   the number of read bytes otherwise.
 */
static int check_discon_message_cb(struct spoe_frame *frame __maybe_unused, void *arg1 __maybe_unused, void *arg2 __maybe_unused)
{
#ifdef DEBUG
	/* Get the "message" value */
	const char *str = arg1;
	uint        len = *(uint64_t *)arg2;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, arg1, arg2);

	F_DBG(SPOA, frame, "--> HAPROXY-DISCONNECT message: %.*s", (int)len, str);
#endif

	DBG_RETURN_INT(FUNC_RET_OK);
}


/***
 * NAME
 *   handle_hadiscon -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   Decode a DISCONNECT frame received from HAProxy.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred, otherwise the number
 *   of read bytes.  DISCONNECT frame cannot be ignored and having another
 *   frame than a DISCONNECT frame is an error.
 */
int handle_hadiscon(struct spoe_frame *frame)
{
	const char *buf, *end;
	int         retval;

	DBG_FUNC(FW_PTR, "%p", frame);

	/* Check frame type: we really want a DISCONNECT frame */
	retval = spoe_decode_frame("HAPROXY-DISCONNECT", frame, SPOE_FRM_T_HAPROXY_DISCON, FUNC_RET_ERROR, SPOE_DEC_END);
	if (_ERROR(retval))
		DBG_RETURN_INT(retval);

	buf = frame->buf + frame->offset;
	end = frame->buf + frame->len;

	/* Decode and check the K/V items. */
	retval = spoe_decode_kv(frame, &buf, end,
	                        SPOE_DEC_VARINT, STR_ADDRSIZE("status-code"), check_discon_status_code_cb,
	                        SPOE_DEC_STR, STR_ADDRSIZE("message"), check_discon_message_cb,
	                        SPOE_DEC_END);
	if (_nERROR(retval))
		retval = buf - frame->buf;

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   prepare_agentdicon -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   Encode a DISCONNECT frame to send it to HAProxy.
 *
 * RETURN VALUE
 *   It returns the number of written bytes,
 *   or FUNC_RET_ERROR (-1) in case of the error.
 */
int prepare_agentdicon(struct spoe_frame *frame)
{
	const char *reason;
	int         retval;

	DBG_FUNC(FW_PTR, "%p", frame);

	if (FC_PTR->status_code >= SPOE_FRM_ERRS)
		FC_PTR->status_code = SPOE_FRM_ERR_UNKNOWN;
	reason = spoe_frm_err_reasons(FC_PTR->status_code);

	/*
	 * Set AGENT-DISCONNECT frame type, flags, stream-id and frame-id.
	 * There are also 2 mandatory K/V items: "status-code" and "message".
	 */
	retval = spoe_encode_frame("AGENT-DISCONNECT", frame,
	                           SPOA_FRM_T_AGENT, SPOE_FRM_T_AGENT_DISCON, SPOE_FRM_FL_FIN,
	                           SPOE_ENC_UINT8, 0,
	                           SPOE_ENC_UINT8, 0,
	                           SPOE_ENC_KV, STR_ADDRSIZE("status-code"), SPOE_DATA_T_UINT32, FC_PTR->status_code,
	                           SPOE_ENC_KV, STR_ADDRSIZE("message"), SPOE_DATA_T_STR, reason, strlen(reason),
	                           SPOE_ENC_END);

	F_DBG(SPOA, frame, "<-- AGENT-DISCONNECT status code: %u", FC_PTR->status_code);
	F_DBG(SPOA, frame, "<-- AGENT-DISCONNECT message: %s", reason);

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
