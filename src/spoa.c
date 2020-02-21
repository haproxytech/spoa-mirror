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
 *   acc_payload -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
int acc_payload(struct spoe_frame *frame)
{
	size_t len = frame->len - frame->offset;
	int    retval = frame->offset;

	DBG_FUNC(FW_PTR, "%p", frame);

	if (!frame->fragmented) {
		/* No need to accumulation payload. */
	}
	else if (_ERROR(buffer_grow(&(frame->frag), frame->buf + frame->offset, len))) {
		FC_PTR->status_code = SPOE_FRM_ERR_RES;

		retval = FUNC_RET_ERROR;
	}
	else if (frame->flags & SPOE_FRM_FL_FIN) {
		frame->buf    = (char *)frame->frag.ptr;
		frame->len    = frame->frag.len;
		frame->offset = 0;
	}
	else {
		/* Wait for next parts. */
		frame->buf    = frame->data;
		frame->offset = 0;
		frame->len    = 0;
		frame->flags  = 0;

		retval = 1;
	}

	return retval;
}


/***
 * NAME
 *   release_frame -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void release_frame(struct spoe_frame *frame)
{
	struct worker *w;

	DBG_FUNC(STRUCT_ELEM_SAFE(frame, worker, NULL), "%p", frame);

	if (_NULL(frame))
		return;

	if (ev_is_active(&(frame->ev_process_frame)) || ev_is_pending(&(frame->ev_process_frame))) {
		ev_timer_stop(FW_PTR->ev_base, &(frame->ev_process_frame));
		ev_async_send(FW_PTR->ev_base, &(FW_PTR->ev_async));
	}

	w = FW_PTR;
	LIST_DEL(&(frame->list));
	buffer_free(&(frame->frag));
	(void)memset(frame, 0, sizeof(*frame) + cfg.max_frame_size + SPOA_FRM_LEN);
	LIST_ADDQ(&(w->frames), &(frame->list));
}


/***
 * NAME
 *   unuse_spoe_engine -
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
static void unuse_spoe_engine(struct client *client)
{
	struct spoe_engine *engine;
	struct spoe_frame  *f, *fback;

	DBG_FUNC(CW_PTR, "%p", client);

	if (_NULL(client) || _NULL(client->engine))
		return;

	engine = client->engine;
	client->engine = NULL;
	LIST_DEL(&(client->by_engine));
	if (!LIST_ISEMPTY(&(engine->clients)))
		return;

	C_DBG(SPOA, client, "Remove SPOE engine '%s'", engine->id);
	LIST_DEL(&(engine->list));

	list_for_each_entry_safe(f, fback, &(engine->processing_frames), list)
		release_frame(f);
	list_for_each_entry_safe(f, fback, &(engine->outgoing_frames), list)
		release_frame(f);
	PTR_FREE(engine->id);
	PTR_FREE(engine);
}


/***
 * NAME
 *   release_client -
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
void release_client(struct client *client)
{
	struct spoe_frame *f, *fback;
	bool_t             flag_ev_async_send = 0;

	DBG_FUNC(CW_PTR, "%p", client);

	if (_NULL(client))
		return;

	C_DBG(SPOA, client, "Release client");

	LIST_DEL(&(client->by_worker));
	CW_PTR->nbclients--;

	unuse_spoe_engine(client);
	PTR_FREE(client->engine_id);

	if (ev_is_active(&(client->ev_frame_rd)) || ev_is_pending(&(client->ev_frame_rd))) {
		ev_io_stop(CW_PTR->ev_base, &(client->ev_frame_rd));
		flag_ev_async_send = 1;
	}
	if (ev_is_active(&(client->ev_frame_wr)) || ev_is_pending(&(client->ev_frame_wr))) {
		ev_io_stop(CW_PTR->ev_base, &(client->ev_frame_wr));
		flag_ev_async_send = 1;
	}
	if (flag_ev_async_send)
		ev_async_send(CW_PTR->ev_base, &(CW_PTR->ev_async));

	release_frame(client->incoming_frame);
	release_frame(client->outgoing_frame);
	list_for_each_entry_safe(f, fback, &(client->processing_frames), list)
		release_frame(f);
	list_for_each_entry_safe(f, fback, &(client->outgoing_frames), list)
		release_frame(f);

	FD_CLOSE(client->fd);
	PTR_FREE(client);
}


/***
 * NAME
 *   reset_frame -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void reset_frame(struct spoe_frame *frame)
{
	DBG_FUNC(FW_PTR, "%p", frame);

	if (_NULL(frame))
		return;

	buffer_free(&(frame->frag));

	frame->type       = SPOA_FRM_T_UNKNOWN;
	frame->buf        = frame->data;
	frame->offset     = 0;
	frame->len        = 0;
	frame->stream_id  = 0;
	frame->frame_id   = 0;
	frame->flags      = 0;
	frame->hcheck     = false;
	frame->fragmented = false;
	LIST_INIT(&(frame->list));
}


/***
 * NAME
 *   write_frame -
 *
 * ARGUMENTS
 *   client -
 *   frame  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void write_frame(struct client *client, struct spoe_frame *frame)
{
	bool_t flag_ev_async_send = 0;

	DBG_FUNC(STRUCT_ELEM_SAFE(client, worker, NULL), "%p, %p", client, frame);

	LIST_DEL(&(frame->list));

	frame->buf    = frame->data;
	frame->offset = 0;
	*(uint32_t *)frame->buf = htonl(frame->len);

	if (_nNULL(client)) {
		/* HELLO or DISCONNECT frames. */
		ev_io_start(CW_PTR->ev_base, &(client->ev_frame_wr));

		/*
		 * Try to process the frame as soon as possible, and always
		 * attach it to the client
		 */
		if (client->async || client->pipelining) {
			if (_NULL(client->outgoing_frame))
				client->outgoing_frame = frame;
			else
				LIST_ADD(&(client->outgoing_frames), &(frame->list));
		} else {
			client->outgoing_frame = frame;
			ev_io_stop(CW_PTR->ev_base, &(client->ev_frame_rd));
		}

		flag_ev_async_send = 1;
	} else {
		/* For all other frames. */
		if (_NULL(FC_PTR)) {
			/* async mode! */
			LIST_ADDQ(&(frame->engine->outgoing_frames), &(frame->list));
			list_for_each_entry(client, &(frame->engine->clients), by_engine) {
				ev_io_start(CW_PTR->ev_base, &(client->ev_frame_wr));
				flag_ev_async_send = 1;
			}
		}
		else if (FC_PTR->pipelining) {
			LIST_ADDQ(&(FC_PTR->outgoing_frames), &(frame->list));
			ev_io_start(FC_PTR->worker->ev_base, &(FC_PTR->ev_frame_wr));
			flag_ev_async_send = 1;
		}
		else {
			FC_PTR->outgoing_frame = frame;
			ev_io_start(FC_PTR->worker->ev_base, &(FC_PTR->ev_frame_wr));
			ev_io_stop(FC_PTR->worker->ev_base, &(FC_PTR->ev_frame_rd));
			flag_ev_async_send = 1;
		}
	}

	if (flag_ev_async_send)
		ev_async_send(FW_PTR->ev_base, &(FW_PTR->ev_async));
}


/***
 * NAME
 *   process_frame_cb -
 *
 * ARGUMENTS
 *   loop    -
 *   ev      -
 *   revents -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void process_frame_cb(struct ev_loop *loop __maybe_unused, struct ev_timer *ev, int revents __maybe_unused)
{
	STRUCT_ADDR(spoe_frame, frame, ev_process_frame);
	const char *ptr, *str, *end;
	char       *buf;
	uint64_t    len;
	int         rc = FUNC_RET_OK, ip_score = SPOE_MSG_IPREP_UNSET;

	DBG_FUNC(FW_PTR, "%p, %p, 0x%08x", loop, ev, revents);

	FW_PTR->nbframes++;

	F_DBG(SPOA, frame,
	      "Process frame messages: stream-id=%u - frame-id=%u - length=%zu bytes",
	      frame->stream_id, frame->frame_id, frame->len - frame->offset);

	ptr = frame->buf + frame->offset;
	end = frame->buf + frame->len;

	/* Loop on messages. */
	while (_nERROR(rc) && (ptr < end)) {
		/* Decode the message name. */
		rc = spoe_decode(frame, &ptr, end, SPOE_DEC_STR0, &str, &len, SPOE_DEC_END);
		if (_ERROR(rc) || _NULL(str))
			break;

		F_DBG(SPOA, frame, "Process SPOE Message '%.*s'", (int)len, str);

		if ((len == STR_SIZE(SPOE_MSG_IPREP)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_IPREP)) == 0))
			rc = spoa_msg_iprep(frame, &ptr, end, &ip_score);
		else if ((len == STR_SIZE(SPOE_MSG_TEST)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_TEST)) == 0))
			rc = spoa_msg_test(frame, &ptr, end);
		else if ((len == STR_SIZE(SPOE_MSG_MIRROR)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_MIRROR)) == 0))
			rc = spoa_msg_mirror(frame, &ptr, end);
		else
			rc = 0;

		if (rc == 0)
			rc = spoe_decode_skip_msg(frame, &ptr, end);
	}

	/* Prepare agent ACK frame. */
	rc  = prepare_agentack(frame);
	buf = frame->buf + rc;

	if (ip_score != SPOE_MSG_IPREP_UNSET)
		spoa_msg_iprep_action(frame, &buf, ip_score);

	write_frame(NULL, frame);
}


/***
 * NAME
 *   acquire_incoming_frame -
 *
 * ARGUMENTS
 *   client -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static struct spoe_frame *acquire_incoming_frame(struct client *client)
{
	struct spoe_frame *frame;

	DBG_FUNC(CW_PTR, "%p", client);

	if (_nNULL(client->incoming_frame))
		return client->incoming_frame;

	if (LIST_ISEMPTY(&(CW_PTR->frames))) {
		if (_NULL(frame = calloc(1, sizeof(*frame) + cfg.max_frame_size + SPOA_FRM_LEN))) {
			c_log(client, _E("Failed to allocate new frame: %m"));

			return NULL;
		}
	} else {
		frame = LIST_NEXT(&(CW_PTR->frames), typeof(frame), list);
		LIST_DEL(&(frame->list));
	}

	reset_frame(frame);
	FW_PTR        = CW_PTR;
	frame->engine = client->engine;
	FC_PTR        = client;

	ev_timer_init(&(frame->ev_process_frame), process_frame_cb, cfg.processing_delay_us / 1e6, 0.0);

	client->incoming_frame = frame;

	return frame;
}


/***
 * NAME
 *   acquire_outgoing_frame -
 *
 * ARGUMENTS
 *   client -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static struct spoe_frame *acquire_outgoing_frame(struct client *client)
{
	struct spoe_frame *frame = NULL;

	DBG_FUNC(CW_PTR, "%p", client);

	if (_nNULL(client->outgoing_frame)) {
		frame = client->outgoing_frame;
	}
	else if (!LIST_ISEMPTY(&(client->outgoing_frames))) {
		frame = LIST_NEXT(&(client->outgoing_frames), typeof(frame), list);
		LIST_DEL(&(frame->list));
		client->outgoing_frame = frame;
	}
	else if (_nNULL(client->engine) && !LIST_ISEMPTY(&(client->engine->outgoing_frames))) {
		frame = LIST_NEXT(&(client->engine->outgoing_frames), typeof(frame), list);
		LIST_DEL(&(frame->list));
		client->outgoing_frame = frame;
	}

	return frame;
}


/***
 * NAME
 *   process_incoming_frame -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void process_incoming_frame(struct spoe_frame *frame)
{
	DBG_FUNC(FW_PTR, "%p", frame);

	ev_timer_start(FW_PTR->ev_base, &(frame->ev_process_frame));

	if (FC_PTR->async) {
		FC_PTR = NULL;
		LIST_ADDQ(&(frame->engine->processing_frames), &(frame->list));
	}
	else if (FC_PTR->pipelining) {
		LIST_ADDQ(&(FC_PTR->processing_frames), &(frame->list));
	}
	else {
		ev_io_stop(FC_PTR->worker->ev_base, &(FC_PTR->ev_frame_rd));
	}

	ev_async_send(FW_PTR->ev_base, &(FW_PTR->ev_async));
}


/***
 * NAME
 *   frame_recv -
 *
 * ARGUMENTS
 *   frame -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static ssize_t frame_recv(struct spoe_frame *frame)
{
	ssize_t retval = FUNC_RET_ERROR;

	DBG_FUNC(FW_PTR, "%p", frame);

	if (_NULL(frame))
		return retval;

	C_DBG(SPOA, FC_PTR, "--> Receiving data");

	if (frame->buf == frame->data) {
		frame->type = SPOA_FRM_T_HAPROXY;

		/*
		 * Receive the frame length:
		 *   frame->buf points on length part (frame->data)
		 */
		retval = tcp_recv(frame, SPOA_FRM_LEN, " length");
		if (retval == SPOA_FRM_LEN) {
			frame->len  = ntohl(*(uint32_t *)frame->buf);
			frame->buf += SPOA_FRM_LEN;
		} else {
			return (retval > 0) ? 0 : retval;
		}
	}

	/*
	 * Receive the frame data:
	 *   frame->buf points on frame part (frame->data + SPOA_FRM_LEN)
	 */
	retval = tcp_recv(frame, frame->len, " data");
	if (retval == (typeof(retval))frame->len)
		C_DBG(SPOA, FC_PTR, "New frame of %zu bytes received: <%s> <%s>",
		      frame->len, str_hex(frame->buf, frame->len), str_ctrl(frame->buf, frame->len));
	else
		retval = (retval > 0) ? 0 : retval;

	return retval;
}


/***
 * NAME
 *   read_frame_cb -
 *
 * ARGUMENTS
 *   loop    -
 *   ev      -
 *   revents -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void read_frame_cb(struct ev_loop *loop __maybe_unused, ev_io *ev, int revents __maybe_unused)
{
	STRUCT_ADDR(client, client, ev_frame_rd);
	struct spoe_frame *f;
	ssize_t            n;

	DBG_FUNC(CW_PTR, "%p, %p, 0x%08x", loop, ev, revents);

	f = acquire_incoming_frame(client);
	n = frame_recv(f);
	if (n <= 0) {
		if (_ERROR(n))
			release_client(client);

		return;
	}

	if (client->state == SPOA_ST_CONNECTING) {
		if (handle_hahello(f) < 0) {
			c_log(client, _E("Failed to decode HELLO frame"));

			goto disconnect;
		}

		prepare_agenthello(f);

		goto write_frame;
	}
	else if (client->state == SPOA_ST_PROCESSING) {
		if (f->buf[0] == SPOE_FRM_T_HAPROXY_DISCON) {
			client->state = SPOA_ST_DISCONNECTING;

			goto disconnecting;
		}

		if (f->buf[0] == SPOE_FRM_T_UNSET)
			n = handle_hafrag(f);
		else
			n = handle_hanotify(f);

		if (n < 0) {
			c_log(client, _E("Failed to decode frame: %s"),
			      spoe_frm_err_reasons(client->status_code));

			goto disconnect;
		}
		else if (n == 0) {
			c_log(client, _W("Ignore invalid/unknown/aborted frame"));

			reset_frame(f);

			return;
		}
		else if (n == 1) {
			return;
		}
		else {
			/* Process frame. */
			process_incoming_frame(f);
			client->incoming_frame = NULL;

			return;
		}
	}
	else if (client->state == SPOA_ST_DISCONNECTING) {
	  disconnecting:
		if (handle_hadiscon(f) < 0)
			c_log(client, _E("--> HAPROXY-DISCONNECT Failed to decode frame"));
		else if (client->status_code != SPOE_FRM_ERR_NONE)
			c_log(client, _E("--> HAPROXY-DISCONNECT peer closed connection: %s"),
			      spoe_frm_err_reasons(client->status_code));
	}

  disconnect:
	client->state = SPOA_ST_DISCONNECTING;

	if (prepare_agentdicon(f) < 0) {
		c_log(client, _E("Failed to encode DISCONNECT frame"));

		release_client(client);

		return;
	}

  write_frame:
	write_frame(client, f);
	client->incoming_frame = NULL;
}


/***
 * NAME
 *   frame_send -
 *
 * ARGUMENTS
 *   client -
 *   frame  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static ssize_t frame_send(const struct client *client, struct spoe_frame *frame)
{
	ssize_t retval = FUNC_RET_ERROR;

	DBG_FUNC(FW_PTR, "%p, %p", client, frame);

	C_DBG(SPOA, client, "<-- Sending data");

	if (frame->buf == frame->data) {
		/*
		 * Send the frame length:
		 *   frame->buf points on length part (frame->data)
		 */
		retval = tcp_send(client, frame, SPOA_FRM_LEN, " length");
		if (retval == SPOA_FRM_LEN)
			frame->buf += SPOA_FRM_LEN;
		else
			return (retval > 0) ? 0 : retval;
	}

	/*
	 * Send the frame data:
	 *   frame->buf points on frame part (frame->data + SPOA_FRM_LEN)
	 */
	retval = tcp_send(client, frame, frame->len, " data");
	if (retval == (typeof(retval))frame->len)
		C_DBG(SPOA, client, "Frame of %zu bytes sent: <%s> <%s>",
		      frame->len, str_hex(frame->buf, frame->len), str_ctrl(frame->buf, frame->len));
	else
		retval = (retval > 0) ? 0 : retval;

	return retval;
}


/***
 * NAME
 *   write_frame_cb -
 *
 * ARGUMENTS
 *   loop    -
 *   ev      -
 *   revents -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void write_frame_cb(struct ev_loop *loop __maybe_unused, ev_io *ev, int revents __maybe_unused)
{
	STRUCT_ADDR(client, client, ev_frame_wr);
	struct spoe_frame *f;
	int                n;

	DBG_FUNC(CW_PTR, "%p, %p, 0x%08x", loop, ev, revents);

	if (_NULL(f = acquire_outgoing_frame(client))) {
		ev_io_stop(CW_PTR->ev_base, &(client->ev_frame_wr));
		ev_async_send(CW_PTR->ev_base, &(CW_PTR->ev_async));

		return;
	}

	n = frame_send(client, f);
	if (n <= 0) {
		if (_ERROR(n))
			release_client(client);

		return;
	}

	if (client->state == SPOA_ST_CONNECTING) {
		if (f->hcheck) {
			C_DBG(SPOA, client, "Close client after healthcheck");

			release_client(client);

			return;
		}

		client->state = SPOA_ST_PROCESSING;
	}
	else if (client->state == SPOA_ST_PROCESSING) {
		/* Do nothing. */
	}
	else if (client->state == SPOA_ST_DISCONNECTING) {
		release_client(client);

		return;
	}

	release_frame(f);
	client->outgoing_frame = NULL;

	if (!client->async && !client->pipelining) {
		ev_io_stop(CW_PTR->ev_base, &(client->ev_frame_wr));
		ev_io_start(CW_PTR->ev_base, &(client->ev_frame_rd));
		ev_async_send(CW_PTR->ev_base, &(CW_PTR->ev_async));
	}
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
