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


struct config_data cfg = {
#ifdef DEBUG
	.debug_level         = DEFAULT_DEBUG_LEVEL,
#endif
	.max_frame_size      = DEFAULT_MAX_FRAME_SIZE,
	.num_workers         = DEFAULT_NUM_WORKERS,
	.server_address      = DEFAULT_SERVER_ADDRESS,
	.server_port         = DEFAULT_SERVER_PORT,
	.connection_backlog  = DEFAULT_CONNECTION_BACKLOG,
	.processing_delay_us = DEFAULT_PROCESSING_DELAY,
	.monitor_interval_us = DEFAULT_MONITOR_INTERVAL,
	.runtime_us          = DEFAULT_RUNTIME,
	.pidfile_fd          = -1,
	.ev_backend          = EVFLAG_AUTO,
};
struct program_data prg;

#ifdef DEBUG
__THR const void *dbg_w_ptr  = NULL;
__THR int         dbg_indent = 0;
#endif


/***
 * NAME
 *   usage -
 *
 * ARGUMENTS
 *   program_name -
 *   flag_verbose -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
static void usage(const char *program_name, bool_t flag_verbose)
{
	(void)printf("\nUsage: %s { -h --help }\n", program_name);
	(void)printf("       %s { -V --version }\n", program_name);
	(void)printf("       %s { -r --runtime=TIME } [OPTION]...\n\n", program_name);

	if (flag_verbose) {
		(void)printf("Options are:\n");
		(void)printf("  -a, --address=NAME              Specify the address to listen on (default: \"%s\").\n", DEFAULT_SERVER_ADDRESS);
		(void)printf("  -B, --libev-backend=TYPE        Specify the libev backend type (default: AUTO).\n");
		(void)printf("  -b, --connection-backlog=VALUE  Specify the connection backlog size (default: %d).\n", DEFAULT_CONNECTION_BACKLOG);
		(void)printf("  -c, --capability=NAME           Enable the support of the specified capability.\n");
		(void)printf("  -D, --daemonize                 Run this program as a daemon.\n");
#ifdef DEBUG
		(void)printf("  -d, --debug=LEVEL               Enable and specify the debug mode level (default: %d).\n", DEFAULT_DEBUG_LEVEL);
#endif
		(void)printf("  -F, --pidfile=FILE              Specifies a file to write the process-id to.\n");
		(void)printf("  -h, --help                      Show this text.\n");
		(void)printf("  -i, --monitor-interval=TIME     Set the monitor interval (default: %s).\n", str_delay(DEFAULT_MONITOR_INTERVAL));
		(void)printf("  -l, --logfile=[MODE:]FILE       Log all messages to logfile (default: stdout/stderr).\n");
		(void)printf("  -m, --max-frame-size=VALUE      Specify the maximum frame size (default: %d bytes).\n", DEFAULT_MAX_FRAME_SIZE);
		(void)printf("  -n, --num-workers=VALUE         Specify the number of workers (default: %d).\n", DEFAULT_NUM_WORKERS);
		(void)printf("  -p, --port=VALUE                Specify the port to listen on (default: %d).\n", DEFAULT_SERVER_PORT);
		(void)printf("  -r, --runtime=TIME              Run this program for the specified time (0 = unlimited).\n");
		(void)printf("  -t, --processing-delay=TIME     Set a delay to process a message (default: %s).\n", str_delay(DEFAULT_PROCESSING_DELAY));
#ifdef HAVE_LIBCURL
		(void)printf("  -u, --mirror-url=URL            Specify the URL for the HTTP mirroring.\n");
		(void)printf("  -I, --mirror-interface=NAME     Specify the interface/address for outgoing connections.\n");
		(void)printf("  -P, --mirror-local-port=VALUE   Specify the local port range for outgoing connections.\n");
#endif
		(void)printf("  -V, --version                   Show program version.\n\n");
		(void)printf("Supported libev backends: %s.\n\n", ev_backends_supported());
		(void)printf("Supported capabilities: " STR_CAP_FRAGMENTATION ", " STR_CAP_PIPELINING ", " STR_CAP_ASYNC ".\n\n");
		(void)printf("Allowed logging file opening modes: a, w.  The 'a' mode allows openning or\n");
		(void)printf("creating file for writing at end-of-file.  The 'w' mode allows truncating\n");
		(void)printf("the file to zero length or creating a new file.  If a capital letter is used\n");
		(void)printf("for the mode, then line buffering is used when writing to the log file.\n\n");
		(void)printf("The time delay/interval is specified in milliseconds by default, but can be\n");
		(void)printf("in any other unit if the number is suffixed by a unit (us, ms, s, m, h, d).\n\n");
		(void)printf("Copyright 2018-2020 HAProxy Technologies\n");
		(void)printf("SPDX-License-Identifier: GPL-2.0-or-later\n\n");
	} else {
		(void)printf("For help type: %s -h\n\n", program_name);
	}
}


/***
 * NAME
 *   getopt_set_capability -
 *
 * ARGUMENTS
 *   name -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int getopt_set_capability(const char *name)
{
#define CAP_DEF(a,b)   { #a, b },
	static const struct {
		const char *name;
		uint8_t     flag;
	} capabilities[] = { CAP_DEFINES };
#undef CAP_DEF
	int i, retval = FUNC_RET_OK;

	DBG_FUNC(NULL, "\"%s\"", name);

	for (i = 0; i < TABLESIZE(capabilities); i++)
		if (strcasecmp(capabilities[i].name, name) == 0)
			break;

	if (i < TABLESIZE(capabilities)) {
		cfg.cap_flags |= capabilities[i].flag;
	} else {
		(void)fprintf(stderr, "ERROR: unsupported capability '%s'\n", name);

		retval = FUNC_RET_ERROR;
	}

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   getopt_set_ev_backend -
 *
 * ARGUMENTS
 *   type -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int getopt_set_ev_backend(const char *type)
{
#define LIBEV_BACKEND_DEF(v,s)   { s, EVBACKEND_##v },
	static const struct {
		const char *str;
		uint        type;
	} backends[] = { LIBEV_BACKEND_DEFINES };
#undef LIBEV_BACKEND_DEF
	int i, retval = FUNC_RET_OK;

	DBG_FUNC(NULL, "\"%s\"", type);

	for (i = 0; i < TABLESIZE(backends); i++)
		if (strcasecmp(backends[i].str, type) == 0)
			break;

	if (i < TABLESIZE(backends)) {
		cfg.ev_backend |= backends[i].type;
	} else {
		(void)fprintf(stderr, "ERROR: invalid libev backend '%s'\n", type);

		retval = FUNC_RET_ERROR;
	}

	DBG_RETURN_INT(retval);
}


#ifdef DEBUG

/***
 * NAME
 *   getopt_set_debug_level -
 *
 * ARGUMENTS
 *   value       -
 *   debug_level -
 *   val_min     -
 *   val_max     -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int getopt_set_debug_level(const char *value, uint32_t *debug_level, int val_min, int val_max)
{
	int64_t level;
	int     retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "\"%s\", %p, %d, %d", value, debug_level, val_min, val_max);

	if (TEST_OR2(NULL, value, debug_level))
		DBG_RETURN_INT(retval);

	if (*value == '\0') {
		(void)fprintf(stderr, "ERROR: debug level not defined\n");
	}
	else if (str_toll(value, NULL, 1, 10, &level, val_min, val_max)) {
		*debug_level = ((level == -1) ? val_max : level) | (1 << DBG_LEVEL_ENABLED);

		retval = FUNC_RET_OK;
	}
	else {
		(void)fprintf(stderr, "ERROR: invalid debug level (allowed range [%d, %d]): '%s'\n", val_min, val_max, value);
	}

	DBG_RETURN_INT(retval);
}

#endif /* DEBUG */


/***
 * NAME
 *   getopt_set_time -
 *
 * ARGUMENTS
 *   delay   -
 *   time_us -
 *   val_min -
 *   val_max -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int getopt_set_time(const char *delay, uint64_t *time_us, uint64_t val_min, uint64_t val_max)
{
	uint64_t delay_us;
	int      retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "\"%s\", %p, %"PRIu64", %"PRIu64, delay, time_us, val_min, val_max);

	if (TEST_OR2(NULL, delay, time_us))
		DBG_RETURN_INT(retval);

	if (*delay != '\0') {
		delay_us = parse_delay_us(delay, val_min, val_max);

		if (errno == ERANGE)
			(void)fprintf(stderr, "ERROR: wrong amount of time (allowed range [%s, %s]): '%s'\n", str_delay(val_min), str_delay(val_max), delay);
		else if (errno)
			(void)fprintf(stderr, "ERROR: invalid time format: '%s'\n", delay);
		else
			*time_us = delay_us;

		retval = errno ? FUNC_RET_ERROR : FUNC_RET_OK;
	} else {
		(void)fprintf(stderr, "ERROR: time not defined\n");
	}

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   getopt_set_ports -
 *
 * ARGUMENTS
 *   ports -
 *   range -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int getopt_set_ports(const char *ports, int *range)
{
	char    *endptr = NULL;
	int64_t  value[2] = { 0, 0 };
	int      retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "\"%s\", %p", ports, range);

	if (TEST_OR2(NULL, ports, range))
		DBG_RETURN_INT(retval);

	if (*ports != '\0') {
		if (!str_toll(ports, &endptr, 0, 10, value, 0, 65535))
			(void)fprintf(stderr, "ERROR: invalid port range: '%s'\n", ports);
		else if (_NULL(endptr) || (endptr[0] == '\0'))
			retval = FUNC_RET_OK;
		else if (endptr[0] != '-')
			(void)fprintf(stderr, "ERROR: invalid port range: '%s'\n", ports);
		else if (!str_toll(endptr + 1, &endptr, 1, 10, value + 1, 0, 65535))
			(void)fprintf(stderr, "ERROR: invalid port range: '%s'\n", ports);
		else if (value[1] < value[0])
			(void)fprintf(stderr, "ERROR: invalid port range: '%s'\n", ports);
		else
			retval = FUNC_RET_OK;
	} else {
		(void)fprintf(stderr, "ERROR: port range is not defined\n");
	}

	/* If everything is fine, set the port range. */
	if (_OK(retval)) {
		range[0] = value[0];
		range[1] = (value[1] > 0) ? (value[1] - value[0] + 1) : 1;

		W_DBG(NOTICE, NULL, "port range set to { %d, %d }", range[0], range[1]);
	}

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   main -
 *
 * ARGUMENTS
 *   argv -
 *   argc -
 *   envp -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
int main(int argc, char **argv, char **envp __maybe_unused)
{
	static const struct option longopts[] = {
		{ "address",            required_argument, NULL, 'a' },
		{ "libev-backend",      required_argument, NULL, 'B' },
		{ "connection-backlog", required_argument, NULL, 'b' },
		{ "capability",         required_argument, NULL, 'c' },
		{ "daemonize",          no_argument,       NULL, 'D' },
		{ "debug",              required_argument, NULL, 'd' },
		{ "pidfile",            required_argument, NULL, 'F' },
		{ "help",               no_argument,       NULL, 'h' },
		{ "monitor-interval",   required_argument, NULL, 'i' },
		{ "logfile",            required_argument, NULL, 'l' },
		{ "max-frame-size",     required_argument, NULL, 'm' },
		{ "num-workers",        required_argument, NULL, 'n' },
		{ "port",               required_argument, NULL, 'p' },
		{ "runtime",            required_argument, NULL, 'r' },
		{ "processing-delay",   required_argument, NULL, 't' },
#ifdef HAVE_LIBCURL
		{ "mirror-url",         required_argument, NULL, 'u' },
		{ "mirror-interface",   required_argument, NULL, 'I' },
		{ "mirror-local-port",  required_argument, NULL, 'P' },
#endif
		{ "version",            no_argument,       NULL, 'V' },
		{ NULL,                 0,                 NULL, 0   }
	};
	char        shortopts[TABLESIZE(longopts) * 2 + 1];
	int         c, longopts_idx = -1, retval = EX_OK;
	bool_t      flag_error = 0;
#ifdef HAVE_LIBCURL
	const char *mir_url = NULL;
#  ifdef USE_THREADS
	CURLcode    rc = CURLE_FAILED_INIT;
#  endif
#endif

	(void)gettimeofday(&(prg.start_time), NULL);

	prg.name = basename(argv[0]);

	DBG_FUNC(NULL, "%d, %p:%p, %p", argc, DPTR_ARGS(argv), envp);

	(void)getopt_shortopts(longopts, shortopts, sizeof(shortopts), 0);

	while ((c = getopt_long(argc, argv, shortopts, longopts, &longopts_idx)) != EOF) {
		if (c == 'a')
			cfg.server_address = optarg;
		else if (c == 'B')
			flag_error |= _OK(getopt_set_ev_backend(optarg)) ? 0 : 1;
		else if (c == 'b')
			cfg.connection_backlog = atoi(optarg);
		else if (c == 'c')
			flag_error |= _OK(getopt_set_capability(optarg)) ? 0 : 1;
		else if (c == 'D')
			cfg.opt_flags |= FLAG_OPT_DAEMONIZE;
#ifdef DEBUG
		else if (c == 'd')
			flag_error |= _OK(getopt_set_debug_level(optarg, &(cfg.debug_level), -1, (1 << DBG_LEVEL_ENABLED) - 1)) ? 0 : 1;
#else
		else if (c == 'd')
			(void)fprintf(stderr, "WARNING: the program is not configured to run in debug mode, option '%c' ignored\n", c);
#endif
		else if (c == 'F')
			cfg.pidfile = optarg;
		else if (c == 'h')
			cfg.opt_flags |= FLAG_OPT_HELP;
		else if (c == 'i')
			flag_error |= _OK(getopt_set_time(optarg, &(cfg.monitor_interval_us), TIMEINT_S(1), TIMEINT_S(3600))) ? 0 : 1;
		else if (c == 'l')
			cfg.logfile = optarg;
		else if (c == 'm')
			cfg.max_frame_size = atoi(optarg);
		else if (c == 'n')
			cfg.num_workers = atoi(optarg);
		else if (c == 'p')
			cfg.server_port = atoi(optarg);
		else if (c == 'r')
			flag_error |= _OK(getopt_set_time(optarg, (uint64_t *)&(cfg.runtime_us), 0, TIMEINT_S(86400 * 7))) ? 0 : 1;
		else if (c == 't')
			flag_error |= _OK(getopt_set_time(optarg, &(cfg.processing_delay_us), 0, TIMEINT_S(1))) ? 0 : 1;
#ifdef HAVE_LIBCURL
		else if (c == 'u')
			mir_url = optarg;
		else if (c == 'I')
			cfg.mir_interface = optarg;
		else if (c == 'P')
			flag_error |= _OK(getopt_set_ports(optarg, cfg.mir_port)) ? 0 : 1;
#endif
		else if (c == 'V')
			cfg.opt_flags |= FLAG_OPT_VERSION;
		else
			flag_error = 1;
	}

	if (cfg.opt_flags & FLAG_OPT_HELP) {
		usage(prg.name, 1);
	}
	else if (cfg.opt_flags & FLAG_OPT_VERSION) {
		(void)printf("\n%s v%s [build %d] by %s, %s\n\n", prg.name, PACKAGE_VERSION, PACKAGE_BUILD, PACKAGE_AUTHOR, __DATE__);
	}
	else {
		if (cfg.runtime_us < 0) {
			(void)fprintf(stderr, "ERROR: runtime value not set\n");
			flag_error = 1;
		}

		if (cfg.max_frame_size < MIN_FRAME_SIZE) {
			(void)fprintf(stderr, "ERROR: max-frame-size cannot be less than %d\n", MIN_FRAME_SIZE);
			flag_error = 1;
		}

		if (!IN_RANGE(cfg.num_workers, 1, 1000)) {
			(void)fprintf(stderr, "ERROR: invalid number of workers '%d'\n", cfg.num_workers);
			flag_error = 1;
		}

		if (!IN_RANGE(cfg.server_port, 1, 65535)) {
			(void)fprintf(stderr, "ERROR: invalid port '%d'\n", cfg.server_port);
			flag_error = 1;
		}

		if (flag_error)
			usage(prg.name, 0);
	}

	if (flag_error || (cfg.opt_flags & (FLAG_OPT_HELP | FLAG_OPT_VERSION)))
		DBG_RETURN_INT(flag_error ? EX_USAGE : EX_OK);

#ifdef HAVE_LIBCURL
	if (_nNULL(mir_url) && _NULL(cfg.mir_url = parse_url(mir_url))) {
		(void)fprintf(stderr, "ERROR: Unable to set URL '%s'\n", mir_url);
		flag_error = 1;
	}
#  ifdef USE_THREADS
	else if ((rc = curl_global_init(CURL_GLOBAL_DEFAULT)) != CURLE_OK) {
		(void)fprintf(stderr, CURL_STR "Failed to initialize library: %d\n", rc);
		flag_error = 1;
	}
#  endif
#endif

	/* Opening the pidfile. */
	if (!flag_error && (retval == EX_OK))
		if (_nNULL(cfg.pidfile))
			retval = pidfile(cfg.pidfile, &(cfg.pidfile_fd));

	if (!flag_error && (retval == EX_OK))
		if (_nNULL(cfg.logfile)) {
			retval = logfile(cfg.logfile);
			cfg.logfile_in_use = _nERROR(retval);
		}

	if (!flag_error && (retval == EX_OK))
		if (cfg.opt_flags & FLAG_OPT_DAEMONIZE)
			retval = daemonize(1, !cfg.logfile_in_use, &(cfg.pidfile_fd), 1);

	/* Writing PID into the pidfile. */
	if (!flag_error && (retval == EX_OK))
		if (cfg.pidfile_fd >= 0)
			retval = pidfile(NULL, &(cfg.pidfile_fd));

	if (!flag_error && (retval == EX_OK))
		retval = worker_run();

#ifdef HAVE_LIBCURL
#  ifdef USE_THREADS
	if (rc == CURLE_OK)
		curl_global_cleanup();
#  endif

	PTR_FREE(cfg.mir_url);
#endif

	/* Closing the pidfile. */
	if (cfg.pidfile_fd >= 0)
		retval = pidfile(cfg.pidfile, &(cfg.pidfile_fd));

	if (cfg.logfile_in_use)
		logfile_mark("stop ");

	DBG_RETURN_INT(flag_error ? EX_USAGE : retval);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
