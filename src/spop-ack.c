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
#include "include.h"


/***
 * NAME
 *   prepare_agentack -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   Encode an ACK frame to send it to HAProxy.
 *
 * RETURN VALUE
 *   It returns the number of written bytes,
 *   or FUNC_RET_ERROR (-1) in case of the error.
 */
int prepare_agentack(struct spoe_frame *frame)
{
	int retval;

	DBG_FUNC(FW_PTR, "%p", frame);

	frame->buf    = frame->data + SPOA_FRM_LEN;
	frame->offset = 0;
	frame->len    = 0;
	frame->flags  = 0;

	/* Set ACK frame type, flags, stream-id and frame-id. */
	retval = spoe_encode_frame("ACK", frame,
	                           SPOA_FRM_T_AGENT, SPOE_FRM_T_AGENT_ACK, SPOE_FRM_FL_FIN,
	                           SPOE_ENC_VARINT, frame->stream_id,
	                           SPOE_ENC_VARINT, frame->frame_id,
	                           SPOE_ENC_END);

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
