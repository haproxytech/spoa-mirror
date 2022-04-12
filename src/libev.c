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
#include "include.h"


/***
 * NAME
 *   ev_backend_name -
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
const char *ev_backend_name(uint type)
{
#define LIBEV_BACKEND_DEF(v,s)   { s, EVBACKEND_##v },
	static const struct {
		const char *name;
		uint        type;
	} backends[] = { LIBEV_BACKEND_DEFINES };
#undef LIBEV_BACKEND_DEF
	int         i;
	const char *retptr = "unknown";

	DBG_FUNC(NULL, "%u", type);

	for (i = 0; i < TABLESIZE(backends); i++)
		if (backends[i].type == type) {
			retptr = backends[i].name;

			break;
		}

	DBG_RETURN_CPTR(retptr);
}


/***
 * NAME
 *   ev_backend_type -
 *
 * ARGUMENTS
 *   loop -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
const char *ev_backend_type(struct ev_loop *loop)
{
	const char *retptr = "invalid";

	DBG_FUNC(NULL, "%p", loop);

	if (_nNULL(loop))
		retptr = ev_backend_name(ev_backend(loop));

	DBG_RETURN_CPTR(retptr);
}


/***
 * NAME
 *   ev_backends_supported -
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
const char *ev_backends_supported(void)
{
	static __THR char retbuf[BUFSIZ];
	uint              i, ev_backends;

	DBG_FUNC(NULL, "");

	(void)memset(retbuf, 0, sizeof(retbuf));

	ev_backends = ev_supported_backends();

	for (i = 1; i <= EVBACKEND_ALL; i <<= 1)
		if (ev_backends & i) {
			if (*retbuf != '\0')
				(void)strncat(retbuf, ", ", SIZEOF_1(retbuf));

			(void)strncat(retbuf, ev_backend_name(i), SIZEOF_1(retbuf));
		}

	DBG_RETURN_CPTR(retbuf);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
