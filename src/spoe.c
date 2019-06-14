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
#include "include.h"


/***
 * NAME
 *   spoe_frm_err_reasons -
 *
 * ARGUMENTS
 *   status_code -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
const char *spoe_frm_err_reasons(int status_code)
{
#define SPOE_FRM_ERR_DEF(v,e,s)   { s, SPOE_FRM_ERR_##e },
	static const struct {
		const char *str;
		int code;
	} reasons[] = { SPOE_FRM_ERR_DEFINES };
#undef SPOE_FRM_ERR_DEF
	int i;

	for (i = 0; i < TABLESIZE(reasons); i++)
		if (reasons[i].code == status_code)
			break;

	/* If status_code is invalid, return message for SPOE_FRM_ERR_UNKNOWN code. */
	return (i < TABLESIZE(reasons)) ? reasons[i].str : reasons[TABLESIZE_1(reasons)].str;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
