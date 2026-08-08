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
#ifndef _TYPES_SPOA_MESSAGE_H
#define _TYPES_SPOA_MESSAGE_H

#define SPOE_MSG_IPREP         "check-client-ip"
#define SPOE_MSG_IPREP_UNSET   -42

#define SPOE_MSG_TEST          "test"

#define SPOE_MSG_MIRROR        "mirror"

#define SPOE_MSG_ARG_BODY      "arg_body"
#define SPOE_MSG_ARG_HDRS      "arg_hdrs"
#define SPOE_MSG_ARG_METHOD    "arg_method"
#define SPOE_MSG_ARG_PATH      "arg_path"
#define SPOE_MSG_ARG_VER       "arg_ver"

/* The HTTP request that is mirrored to the mirror server. */
struct mirror {
	char        *url;            /* The URL of the mirrored request. */
	char        *path;           /* The path of the HTTP request. */
	char        *method;         /* The HTTP request method. */
	int          request_method; /* The index of the HTTP request method. */
	char        *version;        /* The HTTP version. */
	struct list  hdrs;           /* The headers of the HTTP request. */
	char        *body;           /* The body of the HTTP request. */
	size_t       body_head;      /* The number of the body bytes already sent. */
	size_t       body_size;      /* The size of the body. */
};

#endif /* _TYPES_SPOA_MESSAGE_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
