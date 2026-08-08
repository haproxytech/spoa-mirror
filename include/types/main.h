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
#ifndef _TYPES_MAIN_H
#define _TYPES_MAIN_H

#define DEFAULT_DEBUG_LEVEL          IFDEF_DBG((1 << DBG_LEVEL_FUNC) | (1 << DBG_LEVEL_SPOA), (1 << DBG_LEVEL_SPOA))
#define DEFAULT_MAX_FRAME_SIZE       16384
#define DEFAULT_NUM_WORKERS          10
#define DEFAULT_MONITOR_INTERVAL     5000000
#define DEFAULT_SERVER_ADDRESS       "0.0.0.0"
#define DEFAULT_SERVER_PORT          12345
#define DEFAULT_PROCESSING_DELAY     0
#define DEFAULT_CONNECTION_BACKLOG   10
#define DEFAULT_RUNTIME              -1

#define MIN_FRAME_SIZE               512

#define SPOP_VERSION                 "2.0"

#define STR_CAP_FRAGMENTATION        "fragmentation"
#define STR_CAP_PIPELINING           "pipelining"
#define STR_CAP_ASYNC                "async"
#define STR_CAP_NONE                 "<none>"

#define CAP_DEFINES                  \
	CAP_DEF(FRAGMENTATION, 0x01) \
	CAP_DEF(PIPELINING,    0x02) \
	CAP_DEF(ASYNC,         0x04)

#define CAP_DEF(a,b)   FLAG_CAP_##a = b,
enum FLAG_CAP_enum {
	CAP_DEFINES
};
#undef CAP_DEF

enum FLAG_OPT_enum {
	FLAG_OPT_HELP      = 0x01,
	FLAG_OPT_VERSION   = 0x02,
	FLAG_OPT_DAEMONIZE = 0x04,
};


/* The program configuration, mostly set from the command line. */
struct config_data {
#ifdef DEBUG
	uint32_t      debug_level;         /* The debug mode level. */
#endif
	uint8_t       opt_flags;           /* Options set on the command line. */
	unsigned int  max_frame_size;      /* The maximum frame size. */
	int           num_workers;         /* The number of the workers. */
	const char   *server_address;      /* The address to listen on. */
	int           server_port;         /* The port to listen on. */
	int           connection_backlog;  /* The connection backlog size. */
	uint64_t      processing_delay_us; /* The delay to process a message. */
	uint64_t      monitor_interval_us; /* The interval of the monitor messages. */
	int64_t       runtime_us;          /* The time the program runs (0 = unlimited). */
	uint8_t       cap_flags;           /* The enabled capabilities. */
	const char   *logfile;             /* The file all the messages are logged to. */
	bool_t        logfile_in_use;      /* Set when the log file is opened. */
	const char   *pidfile;             /* The file the process-id is written to. */
	int           pidfile_fd;          /* Descriptor of the opened pid file. */
	uint          ev_backend;          /* The libev backend type. */
#ifdef HAVE_LIBCURL
	char         *mir_url;             /* The URL used for the HTTP mirroring. */
	const char   *mir_interface;       /* Outgoing connections interface (IP address). */
	int           mir_port[2];         /* Outgoing connections port. */
	uint64_t      conn_timeout_us;     /* The maximum time allowed to connect to the mirror server. */
	uint64_t      timeout_us;          /* The maximum time allowed for a single transfer operation. */
#endif
};

/* The runtime data of the running program. */
struct program_data {
	const char     *name;       /* The program name. */
	struct timeval  start_time; /* The time the program started. */
	struct worker  *workers;    /* The workers of the program. */
	unsigned long   clicount;   /* The number of the accepted clients. */
};


extern struct config_data  cfg;
extern struct program_data prg;

#ifdef DEBUG
extern __THR const void *dbg_w_ptr;
extern __THR int         dbg_indent;
#endif

#endif /* _TYPES_MAIN_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
