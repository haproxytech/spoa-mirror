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
#ifndef _TYPES_SPOE_H
#define _TYPES_SPOE_H

#define SPOE_FRAME_BUFFER_SET(f,a,b,c,d)   \
	do {                               \
		(f)->buf    = (a);         \
		(f)->offset = (b);         \
		(f)->len    = (c);         \
		(f)->flags  = (d);         \
	} while (0)
#define SPOE_BUFFER_ADVANCE(r)                    \
	if (_nERROR(r)) {                         \
		retval = ptr - (typeof(ptr))*buf; \
		*buf   = (typeof(*buf))ptr;       \
	}

/* Clients and frames that share the same SPOE engine identifier. */
struct spoe_engine {
	char       *id;                /* The engine identifier. */

	struct list processing_frames; /* Frames that are being processed. */
	struct list outgoing_frames;   /* Frames that wait to be sent. */

	struct list clients;           /* Clients that share the engine. */
	struct list list;              /* Engines of the worker. */
};

/* A single SPOP frame together with its data buffer. */
struct spoe_frame {
	enum spoa_frame_type  type;             /* Not used really, set only. */
	char                 *buf;              /* The buffer the frame is received in or sent from. */
	size_t                offset;           /* The number of the bytes already transferred. */
	size_t                len;              /* The length of the frame data. */
	int                   rd_errors;        /* The number of the consecutive read errors. */
	int                   wr_errors;        /* The number of the consecutive write errors. */

	unsigned int          stream_id;        /* The stream identifier of the frame. */
	unsigned int          frame_id;         /* The frame identifier of the frame. */
	unsigned int          flags;            /* The flags of the frame header. */
	bool                  hcheck;           /* true is the CONNECT frame is a healthcheck */
	bool                  fragmented;       /* true if the frame is fragmented */

	struct ev_timer       ev_process_frame; /* The timer of the message processing delay. */
	struct worker        *worker;           /* The worker that processes the frame. */
	struct spoe_engine   *engine;           /* The engine the frame belongs to. */
	struct client        *client;           /* The client the frame belongs to. */
	struct list           list;             /* Frames of a worker, client or engine. */

	struct buffer         frag;             /* used to accumulate payload of a fragmented frame */

	char                  data[0];          /* The data area of the frame. */
};

#endif /* _TYPES_SPOE_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
