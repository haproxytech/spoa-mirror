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
#ifndef _TYPES_CURL_H
#define _TYPES_CURL_H

#define CURL_STR              "cURL: "
#define CURL_ERR_EASY(a,b)    w_log(NULL, _E(CURL_STR a ": %s (%u)"), curl_easy_strerror(b), (b))
#define CURL_ERR_MULTI(a,b)   w_log(NULL, _E(CURL_STR a ": %s (%d)"), curl_multi_strerror(b), (b))
#define CURL_DBG(a, ...)      W_DBG(2, NULL, "  " CURL_STR a, ##__VA_ARGS__)

/* Time-out connect operations after this amount of milliseconds. */
#define CURL_CON_TMOUT        20000
#define CURL_CON_TMOUT_MIN    1
#define CURL_CON_TMOUT_MAX    60000

/* Time-out the read operation after this amount of milliseconds. */
#define CURL_TMOUT            10000
#define CURL_TMOUT_MIN        1
#define CURL_TMOUT_MAX        60000

/*
 * If the download receives less than "low speed limit" bytes/second during
 * "low speed time" seconds, the operations is aborted.
 */
#define CURL_LS_LIMIT         0L
#define CURL_LS_TIME          0L

#define CURL_KEEPIDLE_TIME    10
#define CURL_KEEPINTVL_TIME   10

/*
 * +---------+-------------------------------------------------+----------+
 * | Method  | Description                                     | RFC 7231 |
 * +---------+-------------------------------------------------+----------+
 * | GET     | Transfer a current representation of the target | Sc 4.3.1 |
 * |         | resource.                                       |          |
 * | HEAD    | Same as GET, but only transfer the status line  | Sc 4.3.2 |
 * |         | and header section.                             |          |
 * | POST    | Perform resource-specific processing on the     | Sc 4.3.3 |
 * |         | request payload.                                |          |
 * | PUT     | Replace all current representations of the      | Sc 4.3.4 |
 * |         | target resource with the request payload.       |          |
 * | DELETE  | Remove all current representations of the       | Sc 4.3.5 |
 * |         | target resource.                                |          |
 * | CONNECT | Establish a tunnel to the server identified by  | Sc 4.3.6 |
 * |         | the target resource.                            |          |
 * | OPTIONS | Describe the communication options for the      | Sc 4.3.7 |
 * |         | target resource.                                |          |
 * | TRACE   | Perform a message loop-back test along the path | Sc 4.3.8 |
 * |         | to the target resource.                         |          |
 * | PATCH   | Apply a set of changes described in the request | RFC 5789 |
 * |         | entity.                                         |          |
 * +---------+-------------------------------------------------+----------+
 */
#define CURL_HTTP_METHOD_DEFINES      \
	CURL_HTTP_METHOD_DEF(GET)     \
	CURL_HTTP_METHOD_DEF(HEAD)    \
	CURL_HTTP_METHOD_DEF(POST)    \
	CURL_HTTP_METHOD_DEF(PUT)     \
	CURL_HTTP_METHOD_DEF(DELETE)  \
	CURL_HTTP_METHOD_DEF(CONNECT) \
	CURL_HTTP_METHOD_DEF(OPTIONS) \
	CURL_HTTP_METHOD_DEF(TRACE)   \
	CURL_HTTP_METHOD_DEF(PATCH)

enum CURL_HTTP_METHOD_enum
{
#define CURL_HTTP_METHOD_DEF(a)   CURL_HTTP_METHOD_##a,
	CURL_HTTP_METHOD_DEFINES
#undef CURL_HTTP_METHOD_DEF
};

#ifndef CURL_AT_LEAST_VERSION
#  define CURL_AT_LEAST_VERSION(x,y,z)   0
#endif
#if CURL_AT_LEAST_VERSION(7, 32, 0)
#  define CURL_v073200(a,b)   a
#else
#  define CURL_v073200(a,b)   b
#endif
#if CURL_AT_LEAST_VERSION(7, 55, 0)
#  define CURL_v075500(a,b)   a
#else
#  define CURL_v075500(a,b)   b
#endif
#if CURL_AT_LEAST_VERSION(7, 61, 0)
#  define CURL_v076100(a,b)   a
#else
#  define CURL_v076100(a,b)   b
#endif

struct curl_data {
	struct ev_loop  *ev_base;         /* */
	struct ev_async *ev_async;        /* */
	struct ev_timer  ev_timer;        /* */
	CURLM           *multi;           /* cURL multi handle. */
	int              running_handles; /* The number of running easy handles within the multi handle. */
};

struct curl_con {
	CURL              *easy;                   /* cURL easy handle. */
	struct curl_slist *hdrs;                   /* A linked list of HTTP headers. */
	char               error[CURL_ERROR_SIZE]; /* Buffer to receive error messages in. */
	struct curl_data  *curl;                   /* */
	struct mirror     *mir;                    /* */
};

/* Information associated with a specific socket. */
struct curl_sock {
	curl_socket_t     fd;     /* */
	CURL             *easy;   /* */
	int               action; /* Set, but not used. */
	struct ev_io      ev_io;  /* */
	struct curl_data *curl;   /* */
};

#endif /* _TYPES_CURL_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
