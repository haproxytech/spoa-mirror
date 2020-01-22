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
#ifndef _INCLUDE_H
#define _INCLUDE_H

#define USE_LOG_STARTTIME

#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sysexits.h>

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif
#ifdef HAVE_STDDEF_H
#  include <stddef.h>
#endif
#ifdef HAVE_STRING_H
#  include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_LIBGEN_H
#  include <libgen.h>
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif
#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#endif
#ifdef HAVE_LIBPTHREAD
#  include <pthread.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <netinet/tcp.h>

#ifdef HAVE_LIBCURL
#  include <curl/curl.h>
#endif

#ifdef HAVE_LIBEV
#  include <ev.h>
#endif

#include "common/debug.h"
#include "common/define.h"
#include "common/mini-clist.h"
#include "common/spoe.h"
#include "common/version.h"

#include "types/util.h"
#ifdef HAVE_LIBCURL
#  include "types/curl.h"
#endif
#include "types/libev.h"
#include "types/main.h"
#include "types/spoa-message.h"
#include "types/spoa.h"
#include "types/spoe-decode.h"
#include "types/spoe-encode.h"
#include "types/spoe.h"
#include "types/tcp.h"
#include "types/worker.h"

#ifdef HAVE_LIBCURL
#  include "proto/curl.h"
#endif
#include "proto/libev.h"
#include "proto/spoa-message.h"
#include "proto/spoa.h"
#include "proto/spoe-decode.h"
#include "proto/spoe-encode.h"
#include "proto/spoe.h"
#include "proto/spop-ack.h"
#include "proto/spop-disconnect.h"
#include "proto/spop-hello.h"
#include "proto/spop-notify.h"
#include "proto/spop-unset.h"
#include "proto/tcp.h"
#include "proto/util.h"
#include "proto/worker.h"

#endif /* _INCLUDE_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
