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


struct client {
	int                 fd;
	unsigned long       id;
	enum spoa_state     state;

	struct ev_io        ev_frame_rd;
	struct ev_io        ev_frame_wr;

	struct spoe_frame  *incoming_frame;
	struct spoe_frame  *outgoing_frame;

	struct list         processing_frames;
	struct list         outgoing_frames;

	unsigned int        max_frame_size;
	int                 status_code;

	char               *engine_id;
	struct spoe_engine *engine;
	bool                pipelining;
	bool                async;
	bool                fragmentation;

	struct worker      *worker;
	struct list         by_worker;
	struct list         by_engine;
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
