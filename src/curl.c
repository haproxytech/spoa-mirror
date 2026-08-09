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


#ifdef DEBUG

/***
 * NAME
 *   mir_curl_slist_dump - write a cURL string list to the log
 *
 * ARGUMENTS
 *   list - cURL string list that is written
 *   msg  - text written in front of the list
 *
 * DESCRIPTION
 *   Write all the elements of the string list <list> to the log, prefixed with
 *   the text <msg>.  The function is compiled in the debug build only.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void mir_curl_slist_dump(const struct curl_slist *list, const char *msg)
{
	if (_NULL(list)) {
		W_DBG(DEBUG, NULL, "%s NULL list", msg);
	}
	else if (_NULL(list->data) && (_NULL(list->next))) {
		W_DBG(DEBUG, NULL, "%s %p:{ empty list }", msg, list);
	}
	else {
		W_DBG(DEBUG, NULL, "%s %p:{", msg, list);
		for ( ; _nNULL(list); list = list->next)
			W_DBG(DEBUG, NULL, "  %p:{ '%s' %p %p }", list, list->data, list->data, list->next);
		W_DBG(DEBUG, NULL, "}");
	}
}


/***
 * NAME
 *   mir_curl_debug_cb - CURLOPT_DEBUGFUNCTION callback function
 *
 * ARGUMENTS
 *   handle  - cURL easy handle of the transfer
 *   type    - kind of the reported data
 *   data    - data that is reported
 *   size    - size of the data, in bytes
 *   userptr - user data pointer, not used
 *
 * DESCRIPTION
 *   Write the data that the cURL library reports about a transfer to the log.
 *   The function is compiled in the debug build only.
 *
 * RETURN VALUE
 *   It always returns CURLE_OK (0).
 */
static int mir_curl_debug_cb(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr)
{
	DBG_FUNC(NULL, "%p, %u, %p, %zu, %p", handle, type, data, size, userptr);

	CURL_DBG("[%p]: %u <%s>", handle, type, str_ctrl(data, size));

	DBG_RETURN_INT(CURLE_OK);
}

#endif /* DEBUG */


/***
 * NAME
 *   mir_curl_handle_close - close a mirroring connection
 *
 * ARGUMENTS
 *   con - connection that is closed
 *
 * DESCRIPTION
 *   Remove the easy handle of the connection <con> from the multi handle and
 *   from the list of the connections that the multi handle serves, then release
 *   the list of the HTTP headers, the easy handle and the mirror data of the
 *   connection, and the connection itself.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void mir_curl_handle_close(struct curl_con *con)
{
	DBG_FUNC(NULL, "%p", con);

	if (_NULL(con))
		DBG_RETURN();

	CURL_DBG("Closing handle { %p %p \"%s\" %p }", con->easy, con->hdrs, con->error, con->curl);

	/* A connection that was never added is not on the list. */
	if (_nNULL(con->list.p) && _nNULL(con->list.n))
		LIST_DEL(&(con->list));

	if (_nNULL(con->curl) && _nNULL(con->curl->multi))
		(void)curl_multi_remove_handle(con->curl->multi, con->easy);

	if (_nNULL(con->hdrs))
		curl_slist_free_all(con->hdrs);

	if (_nNULL(con->easy))
		curl_easy_cleanup(con->easy);

	mir_ptr_free(&(con->mir));

	PTR_FREE(con);

	DBG_RETURN();
}


/***
 * NAME
 *   mir_curl_get_http_version - get the name of an HTTP version
 *
 * ARGUMENTS
 *   version - HTTP version as the cURL library reports it
 *
 * DESCRIPTION
 *   Look for the version <version> in the table that pairs every HTTP version
 *   that the cURL library can report with its name.
 *
 * RETURN VALUE
 *   It returns the name of the version <version>, or "?" if the version is not
 *   in the table.
 */
static const char *mir_curl_get_http_version(long version)
{
	/* The HTTP versions that the cURL library can report, and their names. */
	static struct {
		const char *str;   /* The name of the HTTP version. */
		long        value; /* The HTTP version as the library reports it. */
	} http_version[] = {
		{ "HTTP/???", CURL_HTTP_VERSION_NONE },
		{ "HTTP/1.0", CURL_HTTP_VERSION_1_0  },
		{ "HTTP/1.1", CURL_HTTP_VERSION_1_1  },
#if CURL_AT_LEAST_VERSION(7, 33, 0)
		{ "HTTP/2.0", CURL_HTTP_VERSION_2_0  },
#endif

		/* This has to be the last element. */
		{ "?", -1 }
	};
	int i;

	for (i = 0; i < TABLESIZE_1(http_version); i++)
		if (http_version[i].value == version)
			break;

	return http_version[i].str;
}


/***
 * NAME
 *   mir_curl_check_multi_info - check the completed transfers
 *
 * ARGUMENTS
 *   curl - cURL data of the worker
 *
 * DESCRIPTION
 *   Check for completed transfers, and remove their easy handles.  For every
 *   transfer that is done, the URL, the HTTP version, the response code, the
 *   number of the transferred bytes and the duration are written to the log,
 *   and its connection is then closed.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void mir_curl_check_multi_info(struct curl_data *curl)
{
	struct curl_con *con;
	const CURLMsg   *msg;
	int              msgs_in_queue;
	CURLcode         rc;

	DBG_FUNC(NULL, "%p", curl);

	CURL_DBG("Remaining: %d", curl->running_handles);

	while (_nNULL(msg = curl_multi_info_read(curl->multi, &msgs_in_queue))) {
		CURL_DBG("msg = { %d %p { %d } }, queue = %d", msg->msg, msg->easy_handle, msg->data.result, msgs_in_queue);

		if (msg->msg != CURLMSG_DONE)
			/* Do nothing. */;
		else if ((rc = curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &con)) != CURLE_OK)
			CURL_ERR_EASY("Failed to get private data", rc);
		else {
			CURL_v076100(curl_off_t, double) total_time = -1;
			CURL_v075500(curl_off_t, double) size_upload = -1;
			CURL_v075500(curl_off_t, double) size_download = -1;
			const char *url = NULL;
			long        response_code = -1, version = CURL_HTTP_VERSION_NONE;

			if ((rc = curl_easy_getinfo(msg->easy_handle, CURLINFO_EFFECTIVE_URL, &url)) != CURLE_OK)
				CURL_ERR_EASY("Failed to get effective URL", rc);
#if CURL_AT_LEAST_VERSION(7, 50, 0)
			if ((rc = curl_easy_getinfo(msg->easy_handle, CURLINFO_HTTP_VERSION, &version)) != CURLE_OK)
				CURL_ERR_EASY("Failed to get HTTP version", rc);
#endif
			if ((rc = curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &response_code)) != CURLE_OK)
				CURL_ERR_EASY("Failed to get response code", rc);
			if ((rc = curl_easy_getinfo(msg->easy_handle, CURL_v076100(CURLINFO_TOTAL_TIME_T, CURLINFO_TOTAL_TIME), &total_time)) != CURLE_OK)
				CURL_ERR_EASY("Failed to get transfer total time", rc);
			if ((rc = curl_easy_getinfo(msg->easy_handle, CURL_v075500(CURLINFO_SIZE_UPLOAD_T, CURLINFO_SIZE_UPLOAD), &size_upload)) != CURLE_OK)
				CURL_ERR_EASY("Failed to get number of uploaded bytes", rc);
			if ((rc = curl_easy_getinfo(msg->easy_handle, CURL_v075500(CURLINFO_SIZE_DOWNLOAD_T, CURLINFO_SIZE_DOWNLOAD), &size_download)) != CURLE_OK)
				CURL_ERR_EASY("Failed to get number of downloaded bytes", rc);

			w_log(NULL, "\"%s %s %s\" %ld " CURL_v075500("%ld/%ld", "%.0f/%.0f") " %.3f %s",
			      STRUCT_ELEM_SAFE(con->mir, method, "?"), url, mir_curl_get_http_version(version),
			      response_code, size_upload, size_download,
			      CURL_v076100(total_time / 1000.0, total_time * 1000.0),
			      (msg->data.result != CURLE_OK) ? con->error : "ok");

			CURL_DBG("Done: %s => (%d) %s", url, msg->data.result, con->error);

			mir_curl_handle_close(con);
		}
	}

	DBG_RETURN();
}


/***
 * NAME
 *   mir_curl_ev_socket_cb - libev socket action callback function
 *
 * ARGUMENTS
 *   loop    - event loop of the worker
 *   ev      - socket watcher of the transfer
 *   revents - received event flags
 *
 * DESCRIPTION
 *   Called by libev when we get action on a multi socket.  The action is passed
 *   to the cURL library and the completed transfers are then checked; the timer
 *   of the multi handle is stopped when no transfer is running any more.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void mir_curl_ev_socket_cb(struct ev_loop *loop __maybe_unused, struct ev_io *ev, int revents)
{
	struct curl_data *curl = (typeof(curl))(ev->data);
	CURLMcode         rcm;
	int               ev_bitmask;

	DBG_FUNC(NULL, "%p, %p, 0x%08x", loop, ev, revents);

	ev_bitmask  = (revents & EV_READ) ? CURL_POLL_IN : 0;
	ev_bitmask |= (revents & EV_WRITE) ? CURL_POLL_OUT : 0;

	if ((rcm = curl_multi_socket_action(curl->multi, ev->fd, ev_bitmask, &(curl->running_handles))) != CURLM_OK) {
		CURL_ERR_MULTI("Failed data transfer", rcm);
	} else {
		mir_curl_check_multi_info(curl);
		if(curl->running_handles <= 0) {
			CURL_DBG("last transfer done, kill timeout");

			ev_timer_stop(curl->ev_base, &(curl->ev_timer));
			ev_async_send(curl->ev_base, curl->ev_async);
		}
	}

	DBG_RETURN();
}


/***
 * NAME
 *   mir_curl_socket_set - set the data of a cURL socket
 *
 * ARGUMENTS
 *   socket - socket structure that is set
 *   easy   - cURL easy handle of the transfer
 *   s      - socket file descriptor
 *   what   - action that the cURL library waits for
 *   curl   - cURL data of the worker
 *
 * DESCRIPTION
 *   Assign information to a curl_sock structure.  The watcher that waits for
 *   the action <what> on the socket <s> is then started, so that the library is
 *   told when that action happens.
 *
 * RETURN VALUE
 *   It always returns 0.
 */
static int mir_curl_socket_set(struct curl_sock *socket, CURL *easy, curl_socket_t s, int what, struct curl_data *curl)
{
	int events;

	DBG_FUNC(NULL, "%p, %p, %d, %d, %p", socket, easy, s, what, curl);

	events  = (what & CURL_POLL_IN) ? EV_READ : 0;
	events |= (what & CURL_POLL_OUT) ? EV_WRITE : 0;

	socket->fd     = s;
	socket->easy   = easy;
	socket->action = what;
	socket->curl   = curl;

	if (ev_is_active(&(socket->ev_io)) || ev_is_pending(&(socket->ev_io)))
		ev_io_stop(curl->ev_base, &(socket->ev_io));

	ev_io_init(&(socket->ev_io), mir_curl_ev_socket_cb, socket->fd, events);
	socket->ev_io.data = curl;
	ev_io_start(curl->ev_base, &(socket->ev_io));
	ev_async_send(curl->ev_base, curl->ev_async);

	DBG_RETURN_INT(0);
}


/***
 * NAME
 *   mir_curl_socket_add - add a cURL socket
 *
 * ARGUMENTS
 *   easy - cURL easy handle of the transfer
 *   s    - socket file descriptor
 *   what - action that the cURL library waits for
 *   curl - cURL data of the worker
 *
 * DESCRIPTION
 *   Initialize a new curl_sock structure.  The structure is passed to the cURL
 *   library, which gives it back with every action that happens on the socket
 *   <s>.
 *
 * RETURN VALUE
 *   It returns a pointer to the allocated socket structure, or a NULL pointer
 *   in case of the error.
 */
static struct curl_sock *mir_curl_socket_add(CURL *easy, curl_socket_t s, int what, struct curl_data *curl)
{
	struct curl_sock *retptr;
	int               rc;
	CURLMcode         rcm;

	DBG_FUNC(NULL, "%p, %d, %d, %p", easy, s, what, curl);

	if (_NULL(retptr = calloc(1, sizeof(*retptr)))) {
		w_log(NULL, CURL_STR _E("Failed to allocate memory"));
	} else {
		if ((rc = mir_curl_socket_set(retptr, easy, s, what, curl)) != 0)
			/* Do nothing. */;
		else if ((rcm = curl_multi_assign(curl->multi, s, retptr)) != CURLM_OK)
			CURL_ERR_MULTI("Failed to set socket data", rcm);

		if ((rc != 0) || (rcm != CURLM_OK))
			PTR_FREE(retptr);
	}

	DBG_RETURN_PTR(retptr);
}


/***
 * NAME
 *   mir_curl_socket_remove - remove a cURL socket
 *
 * ARGUMENTS
 *   socket - socket structure that is released
 *   curl   - cURL data of the worker
 *
 * DESCRIPTION
 *   Clean up the curl_sock structure.  The watcher of the socket is stopped
 *   before the structure is released.
 *
 * RETURN VALUE
 *   It always returns 0.
 */
static int mir_curl_socket_remove(struct curl_sock *socket, struct curl_data *curl)
{
	DBG_FUNC(NULL, "%p, %p", socket, curl);

	if (_NULL(socket))
		DBG_RETURN_INT(0);

	if (ev_is_active(&(socket->ev_io)) || ev_is_pending(&(socket->ev_io)))
		ev_io_stop(curl->ev_base, &(socket->ev_io));
	ev_async_send(curl->ev_base, curl->ev_async);

	PTR_FREE(socket);

	DBG_RETURN_INT(0);
}


/***
 * NAME
 *   mir_curl_socket_cb - CURLMOPT_SOCKETFUNCTION callback function
 *
 * ARGUMENTS
 *   easy    - cURL easy handle of the transfer
 *   s       - socket file descriptor
 *   what    - action that the cURL library waits for
 *   userp   - cURL data of the worker
 *   socketp - socket structure of the socket <s>
 *
 * DESCRIPTION
 *   Follow what the cURL library asks for the socket <s>; the socket structure
 *   is released when the socket is not used any more, its data is set again
 *   when the structure exists, and a new structure is added otherwise.
 *
 * RETURN VALUE
 *   It returns 0 on success, or -1 if the socket structure cannot be added.
 */
static int mir_curl_socket_cb(CURL *easy, curl_socket_t s, int what, void *userp, void *socketp)
{
	struct curl_data *curl = (typeof(curl))userp;
	struct curl_sock *socket = (typeof(socket)) socketp;
	int               retval = 0;

	DBG_FUNC(NULL, "%p, %d, %d, %p, %p", easy, s, what, userp, socketp);

	if (what == CURL_POLL_REMOVE)
		retval = mir_curl_socket_remove(socket, curl);
	else if (_nNULL(socket))
		retval = mir_curl_socket_set(socket, easy, s, what, curl);
	else if (_NULL(socket = mir_curl_socket_add(easy, s, what, curl)))
		retval = -1;

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_ev_timer_cb - libev socket timer callback function
 *
 * ARGUMENTS
 *   loop    - event loop of the worker
 *   ev      - timer watcher of the multi handle
 *   revents - received event flags
 *
 * DESCRIPTION
 *   Called by libev when our timeout expires.  The cURL library is told that
 *   the timeout of the multi handle expired, and the completed transfers are
 *   then checked.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void mir_curl_ev_timer_cb(struct ev_loop *loop __maybe_unused, struct ev_timer *ev, int revents __maybe_unused)
{
	struct curl_data *curl = (typeof(curl))(ev->data);
	CURLMcode         rcm;

	DBG_FUNC(NULL, "%p, %p, 0x%08x", loop, ev, revents);

	if ((rcm = curl_multi_socket_action(curl->multi, CURL_SOCKET_TIMEOUT, 0, &(curl->running_handles))) != CURLM_OK)
		CURL_ERR_MULTI("Failed data transfer", rcm);

	mir_curl_check_multi_info(curl);

	DBG_RETURN();
}


/***
 * NAME
 *   mir_curl_timer_cb - CURLMOPT_TIMERFUNCTION callback function
 *
 * ARGUMENTS
 *   multi      - cURL multi handle
 *   timeout_ms - timeout in milliseconds
 *   userp      - cURL data of the worker
 *
 * DESCRIPTION
 *   Update the event timer after curl_multi library calls.  The timer wakes the
 *   cURL library up after <timeout_ms> milliseconds; a negative timeout only
 *   stops the timer, and a zero one is turned into a single millisecond.
 *
 * RETURN VALUE
 *   It always returns CURLM_OK (0).
 */
static int mir_curl_timer_cb(CURLM *multi __maybe_unused, long timeout_ms, void *userp)
{
	struct curl_data *curl = (typeof(curl))userp;

	DBG_FUNC(NULL, "%p, %ld, %p", multi, timeout_ms, userp);

	ev_timer_stop(curl->ev_base, &(curl->ev_timer));

	/***
	 * If the value of argument timeout_ms is less than 0, then the timer
	 * should only be stopped; otherwise a timeout is set.
	 */
	if (timeout_ms >= 0) {
		/***
		 * If timeout is set to 0, we will delay calling the callback
		 * function mir_curl_ev_timer_cb() by 1 millisecond.
		 */
		if (timeout_ms == 0)
			timeout_ms = 1;

		ev_timer_init(&(curl->ev_timer), mir_curl_ev_timer_cb, timeout_ms / 1000.0, 0.0);
		ev_timer_start(curl->ev_base, &(curl->ev_timer));
	}

	ev_async_send(curl->ev_base, curl->ev_async);

	DBG_RETURN_INT(CURLM_OK);
}


/***
 * NAME
 *   mir_curl_set_headers - set the HTTP headers of a transfer
 *
 * ARGUMENTS
 *   con - connection that is set up
 *   mir - mirror data of the message
 *
 * DESCRIPTION
 *   Build the cURL list of the HTTP headers out of the headers of the mirror
 *   data <mir>, and set that list, together with the HTTP request method, on
 *   the easy handle of the connection <con>.
 *
 * RETURN VALUE
 *   It returns CURLE_OK (0) on success, or the error that the cURL library
 *   reported.
 */
static CURLcode mir_curl_set_headers(struct curl_con *con, const struct mirror *mir)
{
	struct buffer     *hdr, *hdr_back;
	struct curl_slist *slist;
	CURLcode           retval = CURLE_OK;

	DBG_FUNC(NULL, "%p, %p", con, mir);

	if (_NULL(con) || _NULL(mir))
		DBG_RETURN_INT(retval);

	list_for_each_entry_safe(hdr, hdr_back, &(mir->hdrs), list) {
		slist = curl_slist_append(con->hdrs, (const char *)hdr->ptr);
		if (_NULL(slist))
			DBG_RETURN_INT(CURLE_OUT_OF_MEMORY);

		con->hdrs = slist;
	}

#ifdef DEBUG
	mir_curl_slist_dump(con->hdrs, "HTTP headers");
#endif

	if ((retval == CURLE_OK) && _nNULL(mir->method))
		if ((retval = curl_easy_setopt(con->easy, CURLOPT_CUSTOMREQUEST, mir->method)) != CURLE_OK)
			CURL_ERR_EASY("Failed to set HTTP request method", retval);

	if ((retval == CURLE_OK) && _nNULL(con->hdrs))
		if ((retval = curl_easy_setopt(con->easy, CURLOPT_HTTPHEADER, con->hdrs)) != CURLE_OK)
			CURL_ERR_EASY("Failed to set HTTP headers", retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_init - initialize the cURL mirroring
 *
 * ARGUMENTS
 *   loop - event loop of the worker
 *   ev   - async watcher of the worker
 *   curl - cURL data that is initialized
 *
 * DESCRIPTION
 *   Initialize the cURL data <curl> of a worker; the multi handle is created,
 *   and the callback functions with which the library asks for the socket and
 *   the timer actions are set.  Everything that was allocated is released again
 *   when any of these steps fails.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) on success, FUNC_RET_ERROR (-1) otherwise.
 */
int mir_curl_init(struct ev_loop *loop, struct ev_async *ev, struct curl_data *curl)
{
#ifndef USE_THREADS
	CURLcode  rc;
#endif
	CURLMcode rcm;
	int       retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "%p, %p", loop, curl);

	if (_NULL(loop) || _NULL(curl))
		DBG_RETURN_INT(retval);

	(void)memset(curl, 0, sizeof(*curl));
	LIST_INIT(&(curl->cons));

	curl->ev_base  = loop;
	curl->ev_async = ev;

	ev_timer_init(&(curl->ev_timer), mir_curl_ev_timer_cb, 0.0, 0.0);
	curl->ev_timer.data = curl;

#ifndef USE_THREADS
	if ((rc = curl_global_init(CURL_GLOBAL_DEFAULT)) != CURLE_OK)
		w_log(NULL, CURL_STR _E("Failed to initialize library"));
	else
#endif
	if (_NULL(curl->multi = curl_multi_init()))
		w_log(NULL, CURL_STR _E("Failed to initialize multi handle"));
	else if ((rcm = curl_multi_setopt(curl->multi, CURLMOPT_SOCKETFUNCTION, mir_curl_socket_cb)) != CURLM_OK)
		CURL_ERR_MULTI("Failed to add socket callback function", rcm);
	else if ((rcm = curl_multi_setopt(curl->multi, CURLMOPT_SOCKETDATA, curl)) != CURLM_OK)
		CURL_ERR_MULTI("Failed to set socket callback function data", rcm);
	else if ((rcm = curl_multi_setopt(curl->multi, CURLMOPT_TIMERFUNCTION, mir_curl_timer_cb)) != CURLM_OK)
		CURL_ERR_MULTI("Failed to add timer callback function", rcm);
	else if ((rcm = curl_multi_setopt(curl->multi, CURLMOPT_TIMERDATA, curl)) != CURLM_OK)
		CURL_ERR_MULTI("Failed to set timer callback function data", rcm);
	else
		DBG_RETURN_INT(FUNC_RET_OK);

	if (_ERROR(retval))
		mir_curl_close(curl);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_close - release the cURL mirroring
 *
 * ARGUMENTS
 *   curl - cURL data that is released
 *
 * DESCRIPTION
 *   Close all the connections that are still added to the multi handle of the
 *   cURL data <curl>, which aborts the transfers that did not finish, then
 *   release the multi handle, stop its timer and clear the whole structure.
 *   The easy handles have to be removed before the multi handle is released,
 *   so the connections cannot be left to it.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void mir_curl_close(struct curl_data *curl)
{
	struct curl_con *con, *con_back;

	DBG_FUNC(NULL, "%p", curl);

	if (_NULL(curl))
		DBG_RETURN();

	list_for_each_entry_safe(con, con_back, &(curl->cons), list)
		mir_curl_handle_close(con);

	if (_nNULL(curl->multi))
		(void)curl_multi_cleanup(curl->multi);

	if (ev_is_active(&(curl->ev_timer)) || ev_is_pending(&(curl->ev_timer)))
		ev_timer_stop(curl->ev_base, &(curl->ev_timer));
	ev_async_send(curl->ev_base, curl->ev_async);

#ifndef USE_THREADS
	curl_global_cleanup();
#endif

	(void)memset(curl, 0, sizeof(*curl));

	DBG_RETURN();
}


/***
 * NAME
 *   mir_curl_read_cb - CURLOPT_READFUNCTION callback function
 *
 * ARGUMENTS
 *   buffer   - buffer in which the data is written
 *   size     - size of one item, in bytes
 *   nitems   - number of the items that fit into the buffer
 *   instream - connection whose request body is sent
 *
 * DESCRIPTION
 *   Copy the next part of the body of the mirrored request into the buffer
 *   <buffer>, and remember how much of the body is already sent.
 *
 * RETURN VALUE
 *   It returns the number of the copied bytes, or 0 when the whole body is
 *   sent.
 */
static size_t mir_curl_read_cb(void *buffer __maybe_unused, size_t size, size_t nitems, void *instream)
{
	size_t retval = 0;
	struct curl_con *con = (typeof(con))instream;

	DBG_FUNC(NULL, "%p, %zu, %zu, %p", buffer, size, nitems, instream);

	if (_NULL(con) || _NULL(con->mir) || _NULL(con->mir->body)) {
		DBG_RETURN_SIZE(retval);
	}
	else if (con->mir->body_head < con->mir->body_size) {
		retval = MIN(size * nitems, con->mir->body_size - con->mir->body_head);
		(void)memcpy(buffer, con->mir->body + con->mir->body_head, retval);

		CURL_DBG("%zu+%zu/%zu byte(s) sent", retval, con->mir->body_head, con->mir->body_size);

		con->mir->body_head += retval;
	}

	DBG_RETURN_SIZE(retval);
}


/***
 * NAME
 *   mir_curl_write_cb - CURLOPT_WRITEFUNCTION callback function
 *
 * ARGUMENTS
 *   buffer    - buffer with the received data
 *   size      - size of one item, in bytes
 *   nitems    - number of the received items
 *   outstream - user data pointer, not used
 *
 * DESCRIPTION
 *   Discard the data that the mirror server answered, telling the cURL library
 *   that all of it is taken.
 *
 * RETURN VALUE
 *   It returns the number of the discarded bytes, that is <size> multiplied by
 *   <nitems>.
 */
static size_t mir_curl_write_cb(void *buffer __maybe_unused, size_t size, size_t nitems, void *outstream __maybe_unused)
{
	size_t retval = size * nitems;

	DBG_FUNC(NULL, "%p, %zu, %zu, %p", buffer, size, nitems, outstream);

	DBG_RETURN_SIZE(retval);
}


/***
 * NAME
 *   mir_curl_xferinfo_cb - CURLOPT_XFERINFOFUNCTION/CURLOPT_PROGRESSFUNCTION callback function
 *
 * ARGUMENTS
 *   clientp - connection whose progress is reported
 *   dltotal - number of the bytes that are expected to be received
 *   dlnow   - number of the bytes that are received so far
 *   ultotal - number of the bytes that are expected to be sent
 *   ulnow   - number of the bytes that are sent so far
 *
 * DESCRIPTION
 *   Write the progress of the transfer of the connection <clientp> to the log.
 *   The older cURL libraries report the sizes as floating point numbers, hence
 *   the two versions of the function.
 *
 * RETURN VALUE
 *   It always returns 0.
 */
#if CURL_AT_LEAST_VERSION(7, 32, 0)

static int mir_curl_xferinfo_cb(void *clientp, curl_off_t dltotal __maybe_unused, curl_off_t dlnow __maybe_unused, curl_off_t ultotal __maybe_unused, curl_off_t ulnow __maybe_unused)
{
	struct curl_con *con = (typeof(con))clientp;

	DBG_FUNC(NULL, "%p, %"PRId64", %"PRId64", %"PRId64", %"PRId64, clientp, dltotal, dlnow, ultotal, ulnow);

	CURL_DBG("Progress: %s (%"PRId64"/%"PRId64" %"PRId64"/%"PRId64")", STRUCT_ELEM_SAFE(con->mir, url, "?"), dlnow, dltotal, ulnow, ultotal);

	DBG_RETURN_INT(0);
}

#else

static int mir_curl_xferinfo_cb(void *clientp, double dltotal __maybe_unused, double dlnow __maybe_unused, double ultotal __maybe_unused, double ulnow __maybe_unused)
{
	struct curl_con *con = (typeof(con))clientp;

	DBG_FUNC(NULL, "%p, %f, %f, %f, %f", clientp, dltotal, dlnow, ultotal, ulnow);

	CURL_DBG("Progress: %s (%.0f/%.0f %.0f/%.0f)", STRUCT_ELEM_SAFE(con->mir, url, "?"), dlnow, dltotal, ulnow, ultotal);

	DBG_RETURN_INT(0);
}

#endif /* CURL_AT_LEAST_VERSION(7, 32, 0) */


/***
 * NAME
 *   mir_curl_add_keepalive - set the keepalive of a transfer
 *
 * ARGUMENTS
 *   con        - connection that is set up
 *   flag_alive - whether the keepalive is enabled on the connection
 *   idle       - idle time in seconds before the first keepalive probe
 *   intvl      - time in seconds between two keepalive probes
 *
 * DESCRIPTION
 *   Set the TCP keepalive on the easy handle of the connection <con>.  Negative
 *   values of <idle> and <intvl> leave the related settings as they are, and
 *   neither of them is set when <flag_alive> is not set.
 *
 * RETURN VALUE
 *   It returns CURLE_OK (0) on success, the error reported by the cURL library,
 *   or CURLE_BAD_FUNCTION_ARGUMENT if <con> is a NULL pointer.
 */
static CURLcode mir_curl_add_keepalive(struct curl_con *con, bool_t flag_alive, long idle, long intvl)
{
	CURLcode retval = CURLE_BAD_FUNCTION_ARGUMENT;

	DBG_FUNC(NULL, "%p, %hhu, %ld, %ld", con, flag_alive, idle, intvl);

	if (_NULL(con))
		DBG_RETURN_INT(retval);

	if ((retval = curl_easy_setopt(con->easy, CURLOPT_TCP_KEEPALIVE, (long)flag_alive)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set TCP keepalive", retval);
	else if (!flag_alive)
		/* Do nothing. */;
	else if ((idle >= 0) && (retval = curl_easy_setopt(con->easy, CURLOPT_TCP_KEEPIDLE, idle)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set TCP keepidle", retval);
	else if ((intvl >= 0) && (retval = curl_easy_setopt(con->easy, CURLOPT_TCP_KEEPINTVL, intvl)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set TCP keepintvl", retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_add_post - set up an HTTP POST request
 *
 * ARGUMENTS
 *   con - connection that is set up
 *   mir - mirror data of the message
 *
 * DESCRIPTION
 *   Set the options that the easy handle of the connection <con> needs to send
 *   the body of a POST request; the size of the body and the callback function
 *   that reads it.  Nothing is set for any other request method.
 *
 * RETURN VALUE
 *   It returns CURLE_OK (0) on success, the error reported by the cURL library,
 *   or CURLE_BAD_FUNCTION_ARGUMENT if an argument is a NULL pointer.
 */
static CURLcode mir_curl_add_post(struct curl_con *con, const struct mirror *mir)
{
	CURLcode retval = CURLE_BAD_FUNCTION_ARGUMENT;

	DBG_FUNC(NULL, "%p, %p", con, mir);

	if (_NULL(con) || _NULL(mir))
		DBG_RETURN_INT(retval);

	if (mir->request_method != CURL_HTTP_METHOD_POST)
		retval = CURLE_OK;
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_HTTP_CONTENT_DECODING, 0L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set HTTP content decoding", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_HTTP_TRANSFER_DECODING, 0L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set HTTP transfer decoding", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_POST, 1L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to init HTTP POST data", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)mir->body_size)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set HTTP POST data size", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_READFUNCTION, mir_curl_read_cb)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set read callback function", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_READDATA, con)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set read callback function data", retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_add_put - set up an HTTP PUT request
 *
 * ARGUMENTS
 *   con - connection that is set up
 *   mir - mirror data of the message
 *
 * DESCRIPTION
 *   Set the options that the easy handle of the connection <con> needs to send
 *   the body of a PUT request; the uploading is switched on, the size of the
 *   body is set, and so is the callback function that reads the body.  Nothing
 *   is set for any other request method.
 *
 * RETURN VALUE
 *   It returns CURLE_OK (0) on success, the error reported by the cURL library,
 *   or CURLE_BAD_FUNCTION_ARGUMENT if an argument is a NULL pointer.
 */
static CURLcode mir_curl_add_put(struct curl_con *con, const struct mirror *mir)
{
	CURLcode retval = CURLE_BAD_FUNCTION_ARGUMENT;

	DBG_FUNC(NULL, "%p, %p", con, mir);

	if (_NULL(con) || _NULL(mir))
		DBG_RETURN_INT(retval);

	if (mir->request_method != CURL_HTTP_METHOD_PUT)
		retval = CURLE_OK;
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_HTTP_CONTENT_DECODING, 0L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set HTTP content decoding", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_HTTP_TRANSFER_DECODING, 0L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set HTTP transfer decoding", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_READFUNCTION, mir_curl_read_cb)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set read callback function", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_UPLOAD, 1L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to enable uploading", retval);
#if !CURL_AT_LEAST_VERSION(7, 12, 1)
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_PUT, 1L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to init HTTP PUT data", retval);
#endif
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_READDATA, con)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set read callback function data", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_INFILESIZE_LARGE, (curl_off_t)mir->body_size)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set HTTP PUT data size", retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_add_out - set the outgoing connection options
 *
 * ARGUMENTS
 *   con - connection that is set up
 *   mir - mirror data of the message
 *
 * DESCRIPTION
 *   Set the interface and the range of the local ports that the connection
 *   <con> uses for the mirrored request, as configured for the program.
 *
 * RETURN VALUE
 *   It returns CURLE_OK (0) on success, or the error that the cURL library
 *   reported.
 */
static CURLcode mir_curl_add_out(struct curl_con *con, const struct mirror *mir)
{
	CURLcode retval = CURLE_OK;

	DBG_FUNC(NULL, "%p, %p", con, mir);

	if (_NULL(con) || _NULL(mir))
		DBG_RETURN_INT(retval);

	if (_nNULL(cfg.mir_interface))
		if ((retval = curl_easy_setopt(con->easy, CURLOPT_INTERFACE, cfg.mir_interface)) != CURLE_OK)
			CURL_ERR_EASY("Failed to set outgoing connections interface", retval);

	if ((retval != CURLE_OK) || (cfg.mir_port[0] == 0))
		/* Do nothing. */;
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_LOCALPORT, (long)cfg.mir_port[0])) != CURLE_OK)
		CURL_ERR_EASY("Failed to set outgoing connections port", retval);
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_LOCALPORTRANGE, (long)cfg.mir_port[1])) != CURLE_OK)
		CURL_ERR_EASY("Failed to set outgoing connections port range", retval);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_add_cert - set the SSL options of a transfer
 *
 * ARGUMENTS
 *   con - connection that is set up
 *   mir - mirror data of the message
 *
 * DESCRIPTION
 *   Switch the verification of the peer certificate and of the host name off
 *   when the mirror URL uses the 'https' scheme, so that the mirror server may
 *   use a self-signed certificate as well.
 *
 * RETURN VALUE
 *   It returns CURLE_OK (0) on success, the error reported by the cURL library,
 *   or CURLE_BAD_FUNCTION_ARGUMENT if an argument is a NULL pointer.
 */
static CURLcode mir_curl_add_cert(struct curl_con *con, const struct mirror *mir)
{
	CURLcode retval = CURLE_BAD_FUNCTION_ARGUMENT;

	DBG_FUNC(NULL, "%p, %p", con, mir);

	if (_NULL(con) || _NULL(mir))
		DBG_RETURN_INT(retval);

	if (strncasecmp(mir->url, STR_ADDRSIZE(STR_HTTPS_PFX)) != 0) {
		retval = CURLE_OK;
	} else {
		CURL_DBG("disabling SSL peer/host verification");

		if ((retval = curl_easy_setopt(con->easy, CURLOPT_SSL_VERIFYPEER, 0L)) != CURLE_OK)
			CURL_ERR_EASY("Failed to disable SSL peer verification", retval);
		else if ((retval = curl_easy_setopt(con->easy, CURLOPT_SSL_VERIFYHOST, 0L)) != CURLE_OK)
			CURL_ERR_EASY("Failed to disable SSL host verification", retval);
	}

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_add_url - set the URL of a transfer
 *
 * ARGUMENTS
 *   con - connection that is set up
 *   mir - mirror data of the message
 *
 * DESCRIPTION
 *   Set the URL and the request target of the mirrored request on the easy
 *   handle of the connection <con>.  The HTTP version 2.0 is selected when the
 *   mirrored request announced it and the cURL library supports it.
 *
 * RETURN VALUE
 *   It returns CURLE_OK (0) on success, the error reported by the cURL library,
 *   or CURLE_BAD_FUNCTION_ARGUMENT if an argument is a NULL pointer.
 */
static CURLcode mir_curl_add_url(struct curl_con *con, const struct mirror *mir)
{
	CURLcode retval = CURLE_BAD_FUNCTION_ARGUMENT;

	DBG_FUNC(NULL, "%p, %p", con, mir);

	if (_NULL(con) || _NULL(mir))
		DBG_RETURN_INT(retval);

#if CURL_AT_LEAST_VERSION(7, 33, 0)
	if (_NULL(mir->version) || (strcasecmp(mir->version, "2.0") != 0))
		retval = CURLE_OK;
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2_0)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set HTTP version", retval);
#else
	retval = CURLE_OK;
#endif

	if (retval != CURLE_OK)
		/* Do nothing. */;
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_URL, mir->url)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set URL", retval);
#if CURL_AT_LEAST_VERSION(7, 55, 0)
	/*
	 * WARNING: without this option, support for absolute-form request
	 * target cannot be used.
	 */
	else if ((retval = curl_easy_setopt(con->easy, CURLOPT_REQUEST_TARGET, mir->path)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set request target", retval);
#endif

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   mir_curl_add - add a mirrored request
 *
 * ARGUMENTS
 *   curl - cURL data of the worker
 *   mir  - mirror data of the message
 *
 * DESCRIPTION
 *   Create a new easy handle, and add it to the global curl_multi.  All the
 *   options of the handle are set before that; the URL, the HTTP headers, the
 *   callback functions, the timeouts and the keepalive, and the multi handle
 *   <curl> then runs the transfer.  The connection is closed again when any of
 *   these steps fails.  The mirror data <mir> is taken over by the connection
 *   only when the easy handle is really added, so that the caller releases the
 *   data in every other case.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) on success, FUNC_RET_ERROR (-1) otherwise.
 */
int mir_curl_add(struct curl_data *curl, struct mirror *mir)
{
	struct curl_con *con;
	CURLcode         rc;
	CURLMcode        rcm;
	int              retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "%p, %p", curl, mir);

	if (_NULL(curl) || _NULL(mir))
		DBG_RETURN_INT(retval);

	CURL_DBG("Adding mirror { \"%s\" \"%s\" \"%s\" %d \"%s\" { %p %p } %p %zu/%zu }", mir->url, mir->path, mir->method, mir->request_method, mir->version, mir->hdrs.p, mir->hdrs.n, mir->body, mir->body_head, mir->body_size);

	if (_NULL(con = calloc(1, sizeof(*con))))
		w_log(NULL, CURL_STR _E("Failed to allocate memory"));
	else if (_NULL(con->easy = curl_easy_init()))
		w_log(NULL, CURL_STR _E("Failed to initialize easy handle"));
	else if ((rc = mir_curl_add_out(con, mir)) != CURLE_OK)
		/* Do nothing. */;
	else if ((rc = mir_curl_add_cert(con, mir)) != CURLE_OK)
		/* Do nothing. */;
	else if ((rc = mir_curl_add_url(con, mir)) != CURLE_OK)
		/* Do nothing. */;
	else if ((rc = mir_curl_set_headers(con, mir)) != CURLE_OK)
		/* Do nothing. */;
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_WRITEFUNCTION, mir_curl_write_cb)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set write callback function", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_WRITEDATA, con)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set write callback function data", rc);
#ifdef DEBUG
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_DEBUGFUNCTION, mir_curl_debug_cb)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set debug function", rc);
#endif
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_VERBOSE, IFDEF_DBG(1L, 0L))) != CURLE_OK)
		CURL_ERR_EASY("Failed to set verbosity", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_ERRORBUFFER, con->error)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set error messages buffer", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_PRIVATE, con)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set private data pointer", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_NOPROGRESS, 0L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to switch on the progress meter", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURL_v073200(CURLOPT_XFERINFOFUNCTION, CURLOPT_PROGRESSFUNCTION), mir_curl_xferinfo_cb)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set transfer callback function", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURL_v073200(CURLOPT_XFERINFODATA, CURLOPT_PROGRESSDATA), con)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set transfer callback function data", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_LOW_SPEED_LIMIT, CURL_LS_LIMIT)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set low speed limit", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_LOW_SPEED_TIME, CURL_LS_TIME)) != CURLE_OK)
		CURL_ERR_EASY("Failed to set low speed time", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_NOSIGNAL, 1L)) != CURLE_OK)
		CURL_ERR_EASY("Failed to disable signals", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_CONNECTTIMEOUT_MS, (long)(cfg.conn_timeout_us / UINT64_C(1000)))) != CURLE_OK)
		CURL_ERR_EASY("Failed to set connect timeout", rc);
	else if ((rc = curl_easy_setopt(con->easy, CURLOPT_TIMEOUT_MS, (long)(cfg.timeout_us / UINT64_C(1000)))) != CURLE_OK)
		CURL_ERR_EASY("Failed to set read timeout", rc);
	else if ((rc = mir_curl_add_keepalive(con, 1, CURL_KEEPIDLE_TIME, CURL_KEEPINTVL_TIME)) != CURLE_OK)
		/* Do nothing. */;
	else if ((rc = mir_curl_add_post(con, mir)) != CURLE_OK)
		/* Do nothing. */;
	else if ((rc = mir_curl_add_put(con, mir)) == CURLE_OK) {
		CURL_DBG("Adding easy %p to multi %p (%s)", con->easy, curl->multi, mir->url);

		con->curl = curl;

		if ((rcm = curl_multi_add_handle(curl->multi, con->easy)) != CURLM_OK) {
			CURL_ERR_MULTI("Failed to add easy handle", rcm);
		} else {
			con->mir = mir;
			LIST_ADDQ(&(curl->cons), &(con->list));

			retval = FUNC_RET_OK;
		}
	}

	if (_ERROR(retval))
		mir_curl_handle_close(con);

	DBG_RETURN_INT(retval);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
