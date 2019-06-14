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

#define SPOE_BUFFER_ADVANCE(r)                    \
	if (_nERROR(r)) {                         \
		retval = ptr - (typeof(ptr))*buf; \
		*buf   = ptr;                     \
	}

struct spoe_engine {
	char       *id;

	struct list processing_frames;
	struct list outgoing_frames;

	struct list clients;
	struct list list;
};

struct spoe_frame {
	enum spoa_frame_type  type;       /* Not used really, set only. */
	char                 *buf;
	size_t                offset;
	size_t                len;
	int                   rd_errors;
	int                   wr_errors;

	unsigned int          stream_id;
	unsigned int          frame_id;
	unsigned int          flags;
	bool                  hcheck;     /* true is the CONNECT frame is a healthcheck */
	bool                  fragmented; /* true if the frame is fragmented */

	struct ev_timer       ev_process_frame;
	struct worker        *worker;
	struct spoe_engine   *engine;
	struct client        *client;
	struct list           list;

	struct buffer         frag;       /* used to accumulate payload of a fragmented frame */

	char                  data[0];
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
