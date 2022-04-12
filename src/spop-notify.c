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
 *   handle_hanotify -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   Decode a NOTIFY frame received from HAProxy.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_ERROR (-1) if an error occurred,
 *   0 if it must be ignored, otherwise the number of read bytes.
 */
int handle_hanotify(struct spoe_frame *frame)
{
	int rc;

	DBG_FUNC(FW_PTR, "%p", frame);

	/* Check frame type.  Retrieve flags, stream-id and frame-id. */
	rc = spoe_decode_frame("HAPROXY-NOTIFY", frame, SPOE_FRM_T_HAPROXY_NOTIFY, 0, SPOE_DEC_END);
	if (_ERROR(rc) || (rc == 0))
		DBG_RETURN_INT(rc);

	/* Check if fragmentation is supported. */
	if (!(frame->flags & SPOE_FRM_FL_FIN) && !(cfg.cap_flags & FLAG_CAP_FRAGMENTATION)) {
		FC_PTR->status_code = SPOE_FRM_ERR_FRAG_NOT_SUPPORTED;

		DBG_RETURN_INT(FUNC_RET_ERROR);
	}

	F_DBG(SPOA, frame, "--> HAPROXY-NOTIFY - %sfragmented frame received"
	      " - frag_len=%zu - len=%zu - offset=%zu",
	      (frame->flags & SPOE_FRM_FL_FIN) ? "un" : "",
	      frame->frag.len, frame->len, frame->offset);

	DBG_RETURN_INT(acc_payload(frame));
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
