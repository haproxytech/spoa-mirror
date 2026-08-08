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
#ifndef _TYPES_SPOA_H
#define _TYPES_SPOA_H

#define SPOA_FRM_LEN        sizeof(uint32_t)
#define SPOA_FRM_READ_CNT   3

/* A fragmented payload may not grow beyond this many frames. */
#define SPOA_FRAG_MAX_FRM   64

#define FC_PTR              (frame->client)
#define FW_PTR              (frame->worker)
#define CW_PTR              (client->worker)

enum spoa_state {
	SPOA_ST_CONNECTING = 0,
	SPOA_ST_PROCESSING,
	SPOA_ST_DISCONNECTING,
};

enum spoa_frame_type {
	SPOA_FRM_T_UNKNOWN = 0,
	SPOA_FRM_T_HAPROXY,
	SPOA_FRM_T_AGENT,
};


/* Data of a single HAProxy connection to the agent. */
struct client {
	int                 fd;                /* The socket of the connection. */
	unsigned long       id;                /* The client identifier. */
	enum spoa_state     state;             /* The state of the SPOP connection. */

	struct ev_io        ev_frame_rd;       /* The watcher that reads the frames. */
	struct ev_io        ev_frame_wr;       /* The watcher that writes the frames. */

	struct spoe_frame  *incoming_frame;    /* The frame the data is received in. */
	struct spoe_frame  *outgoing_frame;    /* The frame the data is sent from. */

	struct list         processing_frames; /* Frames that are being processed. */
	struct list         outgoing_frames;   /* Frames that wait to be sent. */

	unsigned int        max_frame_size;    /* The maximum frame size of the client. */
	int                 status_code;       /* The status code of the last frame error. */

	char               *engine_id;         /* The engine identifier the client announced. */
	struct spoe_engine *engine;            /* The engine the client is attached to. */
	bool                pipelining;        /* Set when the pipelining capability is used. */
	bool                async;             /* Set when the async capability is used. */
	bool                fragmentation;     /* Set when the fragmentation capability is used. */

	struct worker      *worker;            /* The worker that serves the client. */
	struct list         by_worker;         /* Clients of the worker. */
	struct list         by_engine;         /* Clients of the engine. */
};

#endif /* _TYPES_SPOA_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
