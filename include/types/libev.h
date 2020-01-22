/***
 * Copyright 2020 HAProxy Technologies
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
#ifndef _TYPES_LIBEV_H
#define _TYPES_LIBEV_H

#if ((EV_VERSION_MAJOR >= 4) && (EV_VERSION_MINOR >= 29))
#  define LIBEV_BACKEND_DEFINES_EX              \
	LIBEV_BACKEND_DEF(LINUXAIO, "linuxaio") \
	LIBEV_BACKEND_DEF(IOURING,  "iouring")
#elif ((EV_VERSION_MAJOR >= 4) && (EV_VERSION_MINOR >= 27))
#  define LIBEV_BACKEND_DEFINES_EX              \
	LIBEV_BACKEND_DEF(LINUXAIO, "linuxaio")
#else
#  define LIBEV_BACKEND_DEFINES_EX
#endif
#define LIBEV_BACKEND_DEFINES                   \
	LIBEV_BACKEND_DEF(SELECT,   "select")   \
	LIBEV_BACKEND_DEF(POLL,     "poll")     \
	LIBEV_BACKEND_DEF(EPOLL,    "epoll")    \
	LIBEV_BACKEND_DEF(KQUEUE,   "kqueue")   \
	LIBEV_BACKEND_DEF(DEVPOLL,  "devpoll")  \
	LIBEV_BACKEND_DEF(PORT,     "port")     \
	LIBEV_BACKEND_DEFINES_EX

#endif /* _TYPES_LIBEV_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
