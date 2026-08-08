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
#ifndef _TYPES_WORKER_H
#define _TYPES_WORKER_H

/* Data of a single worker thread. */
struct worker {
	pthread_t         thread;     /* The thread of the worker. */
	int               id;         /* The worker identifier. */
	int               fd;         /* The socket the program listens on. */
	struct ev_async   ev_async;   /* The watcher that wakes the worker up. */
	struct ev_loop   *ev_base;    /* The event loop of the worker. */
	struct ev_timer   ev_monitor; /* The timer of the monitor messages. */

	struct list       engines;    /* The SPOE engines of the worker. */

	unsigned int      nbclients;  /* The number of the served clients. */
	struct list       clients;    /* Clients that the worker serves. */

	struct list       frames;     /* Released frames that can be reused. */
	unsigned int      nbframes;   /* The number of the processed frames. */

#ifdef HAVE_LIBCURL
	struct curl_data  curl;       /* The cURL data of the worker. */
#endif
};

/* A signal watcher and the function that handles the signal. */
struct worker_signal {
	struct ev_signal signal;                                             /* The signal watcher. */
	int              signum;                                             /* The number of the watched signal. */
	void             (*func)(struct ev_loop *, struct ev_signal *, int); /* The callback of the watcher. */
};

#endif /* _TYPES_WORKER_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
