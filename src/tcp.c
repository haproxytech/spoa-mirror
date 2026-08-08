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
 *   tcp_recv - receive frame data
 *
 * ARGUMENTS
 *   frame - frame in which the data is received
 *   len   - number of the data bytes that are expected
 *   msg   - text added to the logged messages
 *
 * DESCRIPTION
 *   Receive the data of the frame <frame> from the socket of its client, adding
 *   it to the data that is already received.  The offset in the frame is reset
 *   once all <len> bytes are collected.  A socket that returns nothing, and an
 *   error that only asks for a repeated attempt, are counted; when the count
 *   reaches the allowed limit, the connection is reported as broken.
 *
 * RETURN VALUE
 *   It returns the number of the received bytes once the whole part of the
 *   frame is received, 0 if the reading has to be repeated, or FUNC_RET_ERROR
 *   (-1) in case of the error.
 */
ssize_t tcp_recv(struct spoe_frame *frame, size_t len, const char *msg)
{
	ssize_t retval;

	DBG_FUNC(NULL, "%p, %zu, \"%s\"", frame, len, msg);

	retval = recv(FC_PTR->fd, frame->buf + frame->offset, len - frame->offset, 0);
	if (retval == 0) {
		if (++(frame->rd_errors) >= TCP_RECV_ERR_MAX) {
			C_DBG(SPOA, FC_PTR, "socket zero receive limit reached");

			errno  = ESTALE;
			retval = FUNC_RET_ERROR;
		}
	}
	else if (retval > 0) {
		frame->offset += retval;

		C_DBG(SPOA, FC_PTR, "%zu/%zu/%zu byte(s) received frame%s", retval, frame->offset, len, msg);

		if (frame->offset == len) {
			retval = frame->offset;

			frame->offset    = 0;
			frame->rd_errors = 0;
		}
	}
	else if (TEST_OR3(errno, EAGAIN, EWOULDBLOCK, EINTR)) {
		if (++(frame->rd_errors) >= TCP_RECV_ERR_MAX)
			C_DBG(SPOA, FC_PTR, "socket error receive limit reached");
		else
			retval = 0;
	}

	if (_ERROR(retval))
		c_log(FC_PTR, _E("Failed to receive frame%s: %m"), msg);

	DBG_RETURN_SSIZE(retval);
}


/***
 * NAME
 *   tcp_send - send frame data
 *
 * ARGUMENTS
 *   client - client to which the data is sent
 *   frame  - frame whose data is sent
 *   len    - number of the data bytes that are sent
 *   msg    - text added to the logged messages
 *
 * DESCRIPTION
 *   Send the data of the frame <frame> to the socket of the client <client>,
 *   continuing where the previous call stopped.  The offset in the frame is
 *   reset once all <len> bytes are sent.  A socket that takes nothing, and an
 *   error that only asks for a repeated attempt, are counted; when the count
 *   reaches the allowed limit, the connection is reported as broken.
 *
 * RETURN VALUE
 *   It returns the number of the sent bytes once the whole part of the frame is
 *   sent, 0 if the sending has to be repeated, or FUNC_RET_ERROR (-1) in case
 *   of the error.
 */
ssize_t tcp_send(const struct client *client, struct spoe_frame *frame, size_t len, const char *msg)
{
	ssize_t retval;

	DBG_FUNC(NULL, "%p, %p, %zu, \"%s\"", client, frame, len, msg);

	retval = send(client->fd, frame->buf + frame->offset, len - frame->offset, 0);
	if (retval == 0) {
		if (++(frame->wr_errors) >= TCP_SEND_ERR_MAX) {
			C_DBG(SPOA, client, "socket zero send limit reached");

			errno  = ESTALE;
			retval = FUNC_RET_ERROR;
		}
	}
	else if (retval > 0) {
		frame->offset += retval;

		C_DBG(SPOA, client, "%zu/%zu/%zu byte(s) send frame%s", retval, frame->offset, len, msg);

		if (frame->offset == len) {
			retval = frame->offset;

			frame->offset    = 0;
			frame->wr_errors = 0;
		}
	}
	else if (TEST_OR3(errno, EAGAIN, EWOULDBLOCK, EINTR)) {
		if (++(frame->wr_errors) >= TCP_SEND_ERR_MAX)
			C_DBG(SPOA, client, "socket error send limit reached");
		else
			retval = 0;
	}

	if (_ERROR(retval))
		c_log(client, _E("Failed to send frame%s: %m"), msg);

	DBG_RETURN_SSIZE(retval);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
