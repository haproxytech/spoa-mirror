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


struct config_data {
#ifdef DEBUG
	uint32_t      debug_level;
#endif
	uint8_t       opt_flags;
	unsigned int  max_frame_size;
	int           num_workers;
	const char   *server_address;
	int           server_port;
	int           connection_backlog;
	uint64_t      processing_delay_us;
	uint64_t      monitor_interval_us;
	int64_t       runtime_us;
	uint8_t       cap_flags;
	const char   *logfile;
	bool_t        logfile_in_use;
	const char   *pidfile;
	int           pidfile_fd;
	uint          ev_backend;
#ifdef HAVE_LIBCURL
	char         *mir_url;
	const char   *mir_address;
	int           mir_port[2];
#endif
};

struct program_data {
	const char     *name;
	struct timeval  start_time;
	struct worker  *workers;
	unsigned long   clicount;
};


extern struct config_data  cfg;
extern struct program_data prg;

#endif /* _TYPES_MAIN_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
