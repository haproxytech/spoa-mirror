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
 *   worker_async_cb -
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
static void worker_async_cb(struct ev_loop *loop __maybe_unused, struct ev_async *ev __maybe_unused, int revents __maybe_unused)
{
#ifdef DEBUG
	const STRUCT_ADDR(worker, w, ev_async);

	DBG_FUNC(w, "%p, %p, 0x%08x", loop, ev, revents);
#endif

	DBG_RETURN();
}


/***
 * NAME
 *   worker_async_init -
 *
 * ARGUMENTS
 *   worker -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void worker_async_init(struct worker *worker)
{
	DBG_FUNC(worker, "%p", worker);

	ev_async_init(&(worker->ev_async), worker_async_cb);
	ev_async_start(worker->ev_base, &(worker->ev_async));

	DBG_RETURN();
}


/***
 * NAME
 *   bind_server_socket -
 *
 * ARGUMENTS
 *   ai -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int bind_server_socket(struct addrinfo *ai)
{
	int    retval, yes = 1;
	bool_t flag_ok = 0;

	DBG_FUNC(NULL, "%p", ai);

	retval = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (_ERROR(retval))
		w_log(NULL, _W("Failed to create server socket: %m"));
	else if (_ERROR(setsockopt(retval, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))))
		w_log(NULL, _W("Failed to set SO_REUSEADDR on server socket: %m"));
	else if (_ERROR(setsockopt(retval, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes))))
		w_log(NULL, _W("Failed to set TCP_NODELAY on server socket: %m"));
	else if (_ERROR(socket_set_keepalive(retval, -1, 4, 1, 3)))
		w_log(NULL, _W("Failed to set KEEPALIVE on server socket: %m"));
	else if (_ERROR(bind(retval, ai->ai_addr, ai->ai_addrlen)))
		w_log(NULL, _W("Failed to bind server socket: %m"));
	else if (_ERROR(listen(retval, cfg.connection_backlog)))
		w_log(NULL, _W("Failed to listen on server socket: %m"));
	else
		flag_ok = 1;

	if (flag_ok)
		W_DBG(WORKER, NULL, "Server socket created: %d", retval);
	else
		FD_CLOSE(retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   create_server_socket -
 *
 * ARGUMENTS
 *   This function takes no arguments.
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int create_server_socket(void)
{
	static const struct addrinfo hints =
	{
		.ai_flags    = AI_PASSIVE | AI_CANONNAME, /* Set the official name of the host */
		.ai_family   = AF_UNSPEC,                 /* Allow IPv4 or IPv6 */
		.ai_socktype = SOCK_STREAM,               /* Use connection-based protocol (TCP) */
		.ai_protocol = 0                          /* Any protocol */
	};
	char             servname[16];
	struct addrinfo *res = NULL, *ai;
	int              rc, retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "");

	(void)snprintf(servname, sizeof(servname), "%d", cfg.server_port);

	if (_nOK(rc = getaddrinfo(cfg.server_address, servname, &hints, &res))) {
		w_log(NULL, _E("Failed to get address info for '%s': %s"), cfg.server_address, gai_strerror(rc));
	} else {
		for (ai = res; _nNULL(ai); ai = ai->ai_next) {
			W_DBG(INFO, NULL, (ai == res) ? "--- addrinfo: %p" : "---", ai);
			W_DBG(INFO, NULL, "  ai_flags:     %d", ai->ai_flags);
			W_DBG(INFO, NULL, "  ai_family:    %d", ai->ai_family);
			W_DBG(INFO, NULL, "  ai_socktype:  %d", ai->ai_socktype);
			W_DBG(INFO, NULL, "  ai_protocol:  %d", ai->ai_protocol);
			W_DBG(INFO, NULL, "  ai_addrlen:   %d", ai->ai_addrlen);
			W_DBG(INFO, NULL, "  ai_canonname: \"%s\"", ai->ai_canonname);
			W_DBG(INFO, NULL, "  ai_addr:      %p", ai->ai_addr);
			W_DBG(INFO, NULL, "  ai_next:      %p", ai->ai_next);

			/*
			 * getaddrinfo() returns a list of address structures.
			 * Try each address until we successfully bind().
			 * If bind_server_socket() fail, try the next address.
			 */
			if (_ERROR(retval) && (ai->ai_family == AF_INET))
				retval = bind_server_socket(ai);
		}
	}

	freeaddrinfo(res);

	if (_ERROR(retval))
		w_log(NULL, _E("Failed to create server socket: %m"));

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   worker_thread_monitor_cb -
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
static void worker_thread_monitor_cb(struct ev_loop *loop __maybe_unused, struct ev_timer *ev __maybe_unused, int revents __maybe_unused)
{
#ifdef DEBUG
	const STRUCT_ADDR(worker, w, ev_monitor);

	DBG_FUNC(w, "%p, %p, 0x%08x", loop, ev, revents);

	if (w->nbclients || ev_async_pending(&(w->ev_async)))
		W_DBG(WORKER, w, "%u clients connected (%u frames), async event %spending",
		      w->nbclients, w->nbframes, ev_async_pending(&(w->ev_async)) ? "" : "not ");
#endif

	DBG_RETURN();
}


/***
 * NAME
 *   worker_thread_exit -
 *
 * ARGUMENTS
 *   worker -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
__noreturn static void *worker_thread_exit(struct worker *worker)
{
	DBG_FUNC(worker, "%p", worker);

	if (ev_is_active(&(worker->ev_monitor)) || ev_is_pending(&(worker->ev_monitor)))
		ev_timer_stop(worker->ev_base, &(worker->ev_monitor));

	if (_nNULL(worker->ev_base) && !ev_is_default_loop(worker->ev_base)) {
		ev_loop_destroy(worker->ev_base);

		worker->ev_base = NULL;
	}

	W_DBG(WORKER, worker, "Worker is stopped");

	DBG_FUNC_END("} = %p", NULL);
	DBG_FUNC_END("} = %p", NULL);

	pthread_exit(NULL);
}


/***
 * NAME
 *   worker_thread -
 *
 * ARGUMENTS
 *   data -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static void *worker_thread(void *data)
{
	char               name[16];
	struct client     *c, *cback;
	struct spoe_frame *f, *fback;
	struct worker     *w = data;

	DBG_FUNC(w, "%p", data);

#ifdef __linux__
	W_DBG(WORKER, w, "Worker started, thread id: %"PRI_PTHREADT, syscall(SYS_gettid));
#else
	W_DBG(WORKER, w, "Worker started, thread id: %"PRI_PTHREADT, pthread_self());
#endif

	/* Can w->thread be used instead of the function pthread_self()? */
	(void)snprintf(name, sizeof(name), "sm/wrk: %d", w->id);
	(void)pthread_setname_np(pthread_self(), name);

	w->nbclients = 0;
	LIST_INIT(&(w->engines));
	LIST_INIT(&(w->clients));
	LIST_INIT(&(w->frames));

	w->ev_base = ev_loop_new(cfg.ev_backend);
	if (_NULL(w->ev_base)) {
		w_log(w, _F("Failed to initialize libev for worker %02d: %m"), w->id);

		DBG_RETURN_PTR(worker_thread_exit(w));
	}

	W_DBG(WORKER, w, "libev: using backend '%s'", ev_backend_type(w->ev_base));

	worker_async_init(w);

#ifdef HAVE_LIBCURL
	if (_nNULL(cfg.mir_url) && _ERROR(mir_curl_init(w->ev_base, &(w->ev_async), &(w->curl)))) {
		w_log(w, _E("Failed to initialize cURL mirroring"));

		DBG_RETURN_PTR(worker_thread_exit(w));
	}
#endif

	ev_timer_init(&(w->ev_monitor), worker_thread_monitor_cb, cfg.monitor_interval_us / 1e6, cfg.monitor_interval_us / 1e6);
	ev_timer_start(w->ev_base, &(w->ev_monitor));

	W_DBG(WORKER, w, "Worker ready to process client messages");

	(void)ev_run(w->ev_base, 0);

	list_for_each_entry_safe(c, cback, &(w->clients), by_worker)
		release_client(c);

	list_for_each_entry_safe(f, fback, &(w->frames), list) {
		LIST_DEL(&(f->list));
		free(f);
	}

	DBG_RETURN_PTR(worker_thread_exit(w));
}


/***
 * NAME
 *   worker_stop_ev -
 *
 * ARGUMENTS
 *   revents -
 *   base    -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void worker_stop_ev(int revents __maybe_unused, void *base)
{
	DBG_FUNC(NULL, "0x%08x, %p", revents, base);

	ev_break(base, EVBREAK_ONE);

	DBG_RETURN();
}


/***
 * NAME
 *   worker_stop -
 *
 * ARGUMENTS
 *   loop -
 *   msg  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void worker_stop(struct ev_loop *loop, const char *msg __maybe_unused)
{
	static bool_t flag_stop = 0;
	int           i;

	DBG_FUNC(NULL, "%p, \"%s\"", loop, msg);

	/*
	 * In case HAProxy is used in master-worker mode, it is possible that
	 * it sends a SIGINT signal several times in a row.  Also, it may
	 * happen that the program runs out of time and immediately after
	 * that the program receives a SIGINT signal.  To disable this, the
	 * variable flag_stop is used.
	 */
	if (flag_stop) {
		W_DBG(WORKER, NULL, "Server stopping already in progress, canceled: %s", msg);

		DBG_RETURN();
	}

	flag_stop = 1;

	W_DBG(WORKER, NULL, "Stopping the server, %s", msg);

	ev_once(loop, -1, 0, 0, worker_stop_ev, loop);

	W_DBG(WORKER, NULL, "Main event loop stopped");

	for (i = 0; i < cfg.num_workers; i++) {
		ev_once(prg.workers[i].ev_base, -1, 0, 0, worker_stop_ev, prg.workers[i].ev_base);
		ev_async_send(prg.workers[i].ev_base, &(prg.workers[i].ev_async));

		W_DBG(WORKER, NULL, "Worker %02d: event loop stopped", prg.workers[i].id);
	}

	DBG_RETURN();
}


/***
 * NAME
 *   worker_signal_stop_cb -
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
static void worker_signal_stop_cb(struct ev_loop *loop, struct ev_signal *ev, int revents __maybe_unused)
{
	char        buffer[BUFSIZ];
	const char *sig_name;

	DBG_FUNC(NULL, "%p, %p, 0x%08x", loop, ev, revents);

	if (_NULL(sig_name = strsignal(ev->signum)))
		sig_name = "Unknown signal";
	(void)snprintf(buffer, sizeof(buffer), "signal received - %s (%d)", sig_name, ev->signum);

	worker_stop(loop, buffer);

	DBG_RETURN();
}


/***
 * NAME
 *   worker_signal_ignore_cb -
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
static void worker_signal_ignore_cb(struct ev_loop *loop __maybe_unused, struct ev_signal *ev, int revents __maybe_unused)
{
	const char *sig_name;

	DBG_FUNC(NULL, "%p, %p, 0x%08x", loop, ev, revents);

	if (_NULL(sig_name = strsignal(ev->signum)))
		sig_name = "Unknown signal";

	W_DBG(WORKER, NULL, "signal ignored - %s received (%d)", sig_name, ev->signum);

	DBG_RETURN();
}




/***
 * NAME
 *   worker_runtime_cb -
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
static void worker_runtime_cb(struct ev_loop *loop __maybe_unused, struct ev_timer *ev __maybe_unused, int revents __maybe_unused)
{
	DBG_FUNC(NULL, "%p, %p, 0x%08x", loop, ev, revents);

	worker_stop(loop, "runtime exceeded");

	DBG_RETURN();
}


/***
 * NAME
 *   worker_accept_cb -
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
static void worker_accept_cb(struct ev_loop *loop __maybe_unused, struct ev_io *ev __maybe_unused, int revents __maybe_unused)
{
	struct worker *w;
	struct client *c;
	int            fd;

	w = prg.workers + (prg.clicount++ % cfg.num_workers);

	DBG_FUNC(w, "%p, %p, 0x%08x", loop, ev, revents);

	fd = accept(w->fd, NULL, NULL);
	if (_ERROR(fd)) {
		if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
			w_log(w, _E("Failed to accept client connection: %m"));

		DBG_RETURN();
	}

	W_DBG(WORKER, NULL,
	      "<%lu> New client connection accepted and assigned to worker %02d",
	      prg.clicount, w->id);

	if (_ERROR(socket_set_nonblocking(fd))) {
		w_log(NULL, _E("Failed to set client socket to non-blocking: %m"));
		(void)close(fd);

		DBG_RETURN();
	}
	else if (_ERROR(socket_set_keepalive(fd, 1, -1, -1, -1))) {
		w_log(NULL, _E("Failed to set KEEPALIVE on server socket: %m"));
		(void)close(fd);

		DBG_RETURN();
	}

	c = calloc(1, sizeof(*c));
	if (_NULL(c)) {
		w_log(w, _E("Failed to allocate memory for client state: %m"));
		(void)close(fd);

		DBG_RETURN();
	}

	c->fd             = fd;
	c->id             = prg.clicount;
	c->state          = SPOA_ST_CONNECTING;
	c->max_frame_size = cfg.max_frame_size;
	c->status_code    = SPOE_FRM_ERR_NONE;
	c->worker         = w;

	LIST_INIT(&(c->processing_frames));
	LIST_INIT(&(c->outgoing_frames));

	LIST_ADDQ(&(w->clients), &(c->by_worker));
	w->nbclients++;

	ev_io_init(&(c->ev_frame_rd), read_frame_cb, fd, EV_READ);
	ev_io_init(&(c->ev_frame_wr), write_frame_cb, fd, EV_WRITE);
	ev_io_start(w->ev_base, &(c->ev_frame_rd));
	ev_async_send(w->ev_base, &(w->ev_async));

	W_DBG(WORKER, NULL, "<%lu> New read event added to worker %02d", prg.clicount, w->id);

	DBG_RETURN();
}


/***
 * NAME
 *   worker_run_exit -
 *
 * ARGUMENTS
 *   fd         -
 *   ev_base    -
 *   ev_signals -
 *   nr_signals -
 *   ev_accept  -
 *   retval     -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int worker_run_exit(int fd, struct ev_loop *ev_base, struct worker_signal *ev_signals, int nr_signals, struct ev_io *ev_accept, int retval)
{
	int i;

	DBG_FUNC(NULL, "%d, %p, %p, %d, %p, %d", fd, ev_base, ev_signals, nr_signals, ev_accept, retval);

	if (ev_is_active(ev_accept) || ev_is_pending(ev_accept))
		ev_io_stop(ev_base, ev_accept);

	for (i = 0; i < nr_signals; i++)
		if (ev_is_active(&(ev_signals[i].signal)) || ev_is_pending(&(ev_signals[i].signal)))
			ev_signal_stop(ev_base, &(ev_signals[i].signal));

	if (_nNULL(ev_base) && !ev_is_default_loop(ev_base))
		ev_loop_destroy(ev_base);

	FD_CLOSE(fd);
	PTR_FREE(prg.workers);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   worker_run -
 *
 * ARGUMENTS
 *   This function takes no arguments.
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
int worker_run(void)
{
	struct worker_signal ev_signals[] = {
		{ .signum =  SIGHUP, .func = worker_signal_stop_cb   },
		{ .signum =  SIGINT, .func = worker_signal_stop_cb   },
		{ .signum = SIGPIPE, .func = worker_signal_ignore_cb },
		{ .signum = SIGTERM, .func = worker_signal_stop_cb   },
		{ .signum = SIGUSR1, .func = worker_signal_stop_cb   },
		{ .signum = SIGUSR2, .func = worker_signal_ignore_cb },
	};
	struct ev_timer  ev_runtime;
	struct ev_loop  *ev_base;
	struct ev_io     ev_accept;
	int              rc, i, fd = -1;

	DBG_FUNC(NULL, "");

	if (_ERROR(rlimit_setnofile())) {
		w_log(NULL, _F("Failed to set file descriptors limit: %m"));

		DBG_RETURN_INT(EX_SOFTWARE);
	}

	for (i = 0; i < TABLESIZE(ev_signals); i++)
		(void)memset(&(ev_signals[i].signal), 0, sizeof(ev_signals[0].signal));
	(void)memset(&ev_accept, 0, sizeof(ev_accept));

	ev_base = ev_default_loop(cfg.ev_backend);
	if (_NULL(ev_base)) {
		w_log(NULL, _F("Failed to initialize libev: %m"));

		DBG_RETURN_INT(EX_SOFTWARE);
	}

	W_DBG(WORKER, NULL, "libev: using backend '%s'", ev_backend_type(ev_base));

	fd = create_server_socket();
	if (_ERROR(fd)) {
		w_log(NULL, _F("Failed to create server socket"));

		DBG_RETURN_INT(worker_run_exit(fd, ev_base, ev_signals, TABLESIZE(ev_signals), &ev_accept, EX_SOFTWARE));
	}

	if (_ERROR(socket_set_nonblocking(fd))) {
		w_log(NULL, _F("Failed to set client socket to non-blocking: %m"));

		DBG_RETURN_INT(worker_run_exit(fd, ev_base, ev_signals, TABLESIZE(ev_signals), &ev_accept, EX_SOFTWARE));
	}

	prg.workers = calloc(cfg.num_workers, sizeof(*(prg.workers)));
	if (_NULL(prg.workers)) {
		w_log(NULL, _F("Failed to allocate memory for workers: %m"));

		DBG_RETURN_INT(worker_run_exit(fd, ev_base, ev_signals, TABLESIZE(ev_signals), &ev_accept, EX_SOFTWARE));
	}

	for (i = 0; i < cfg.num_workers; i++) {
		struct worker *w = prg.workers + i;

		w->id = i + 1;
		w->fd = fd;

		if (_nOK(pthread_create(&(w->thread), NULL, worker_thread, w)))
			w_log(NULL, _E("Failed to start thread for worker %02d: %m"), w->id);
	}

	ev_io_init(&ev_accept, worker_accept_cb, fd, EV_READ);
	ev_io_start(ev_base, &ev_accept);

	for (i = 0; i < TABLESIZE(ev_signals); i++) {
		ev_signal_init(&(ev_signals[i].signal), ev_signals[i].func, ev_signals[i].signum);
		ev_signal_start(ev_base, &(ev_signals[i].signal));
	}

	if (cfg.runtime_us > 0) {
		ev_timer_init(&ev_runtime, worker_runtime_cb, cfg.runtime_us / 1e6, 0.0);
		ev_timer_start(ev_base, &ev_runtime);
	}

	W_DBG(WORKER, NULL,
	      "Server is ready"
	      " [" STR_CAP_FRAGMENTATION "=%s - " STR_CAP_PIPELINING "=%s - " STR_CAP_ASYNC "=%s - debug=%s - max-frame-size=%u]",
	      STR_BOOL(cfg.cap_flags & FLAG_CAP_FRAGMENTATION),
	      STR_BOOL(cfg.cap_flags & FLAG_CAP_PIPELINING),
	      STR_BOOL(cfg.cap_flags & FLAG_CAP_ASYNC),
	      STR_BOOL(cfg.debug_level),
	      cfg.max_frame_size);

	(void)ev_run(ev_base, 0);

	for (i = 0; i < cfg.num_workers; i++) {
		struct worker *w = prg.workers + i;

		rc = pthread_join(w->thread, NULL);
		if (rc != 0)
			w_log(w, _E("Failed to join worker thread %02d: %s"), w->id, strerror(rc));

		W_DBG(WORKER, NULL, "Worker %02d: terminated (%d)", w->id, rc);
	}

	DBG_RETURN_INT(worker_run_exit(fd, ev_base, ev_signals, TABLESIZE(ev_signals), &ev_accept, EX_OK));
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
