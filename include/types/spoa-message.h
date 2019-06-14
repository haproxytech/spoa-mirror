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

struct mirror {
	char        *out_address;    /* Outgoing connections interface (IP address). */
	int          out_port[2];    /* Outgoing connections port. */
	char        *url;            /* */
	char        *path;           /* */
	char        *method;         /* */
	int          request_method; /* */
	char        *version;        /* */
	struct list *hdrs;           /* */
	char        *body;           /* */
	size_t       body_head;      /* */
	size_t       body_size;      /* */
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
