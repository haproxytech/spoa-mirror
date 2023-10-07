/***
 * Copyright 2023 HAProxy Technologies
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
#include "decode-data.h"
#include "util.h"


struct config_data cfg = {
#ifdef DEBUG
	.debug_level = DEFAULT_DEBUG_LEVEL
#endif
};
struct program_data prg;
struct _prg_data    _prg;

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
	(void)printf("       %s [OPTION]...\n\n", program_name);

	if (flag_verbose) {
		(void)printf("Options are:\n");
		(void)printf("  -f data  display data as decoded frame.\n");
		(void)printf("  -s data  Display data as strings.\n");
		(void)printf("  -h       Show this text.\n");
		(void)printf("  -V       Show program version.\n\n");
		(void)printf("Copyright 2023 HAProxy Technologies\n");
		(void)printf("SPDX-License-Identifier: GPL-2.0-or-later\n\n");
	} else {
		(void)printf("For help type: %s -h\n\n", program_name);
	}
}


/***
 * NAME
 *   cb_spoe_dec_str -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int cb_spoe_dec_str(struct spoe_frame *frame __maybe_unused, void *arg1, void *arg2 __maybe_unused)
{
	const char *str = arg1;
	int         len = *(uint64_t *)arg2;

	(void)printf(" value='%.*s' STR\n", len, str);

	return FUNC_RET_OK;
}


/***
 * NAME
 *   cb_spoe_dec_uint8 -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int cb_spoe_dec_uint8(struct spoe_frame *frame __maybe_unused, void *arg1, void *arg2 __maybe_unused)
{
	uint8_t size = *(uint8_t *)arg1;

	(void)printf(" value=%hhu UINT8\n", size);

	return FUNC_RET_OK;
}


/***
 * NAME
 *   cb_spoe_dec_varint -
 *
 * ARGUMENTS
 *   frame -
 *   arg1  -
 *   arg2  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int cb_spoe_dec_varint(struct spoe_frame *frame __maybe_unused, void *arg1, void *arg2 __maybe_unused)
{
	uint64_t size = *(uint64_t *)arg1;

	(void)printf(" value=%"PRIu64" VARINT\n", size);

	return FUNC_RET_OK;
}


/***
 * NAME
 *   hex2uint8 -
 *
 * ARGUMENTS
 *   data   -
 *   len    -
 *   buffer -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static ssize_t hex2uint8(const char *data, size_t len, uint8_t **buffer)
{
	size_t  i, j, buflen = (len >> 1) + 1;
	ssize_t retval = FUNC_RET_ERROR;

	if ((data == NULL) || (buffer == NULL))
		return retval;

	if (*buffer == NULL) {
		*buffer = calloc(1, buflen);
		if (*buffer == NULL)
			return retval;
	}

	for (i = 0; i < len; i += (j > 0) ? (j * 2) : 2) {
		for (j = 0; j < buflen; j++) {
			if (STR_IDX(i, j, 0) == len) {
				/* End of data. */
				retval = j;

				break;
			}
			else if (STR_IDX(i, j, 1) == len) {
				/* Single byte hex data must have two digits. */
				(void)printf("0x%04lx: %.1s -- invalid hex value\n", i >> 1, data + STR_IDX(i, j, 0));

				break;
			}
			else {
				/* All printable data will be copied to the buffer. */
				(*buffer)[j] = STR_HEX(data[STR_IDX(i, j, 0)], data[STR_IDX(i, j, 1)]);
			}
		}
	}

	return retval;
}


/***
 * NAME
 *   decode_bin -
 *
 * ARGUMENTS
 *   data -
 *   len  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int decode_bin(const char *data, size_t len)
{
	char   buffer[BUFSIZ];
	size_t i, j;
	int    ch = 0;

	if (data == NULL)
		return -1;
	else if (len == 0)
		return 0;

	for (i = 0; i < len; i += (j > 0) ? (j * 2) : 2) {
		for (j = 0; j < (sizeof(buffer) - 1); j++) {
			if (STR_IDX(i, j, 0) == len) {
				/* End of data. */
				break;
			}
			else if (STR_IDX(i, j, 1) == len) {
				/* Single byte hex data must have two digits. */
				(void)printf("0x%04x: %.1s -- invalid hex value\n", (int)(i >> 1), data + STR_IDX(i, j, 0));

				return -1;
			}
			else {
				/* All printable data will be copied to the buffer. */
				ch = STR_HEX(data[STR_IDX(i, j, 0)], data[STR_IDX(i, j, 1)]);

				if (!isprint(ch))
					break;

				buffer[j] = ch;
			}
		}

		/* Buffer or one byte printing. */
		if (j > 0)
			(void)printf("0x%04x: %.*s\n", (int)(i >> 1), (int)j, buffer);
		else
			(void)printf("0x%04x: 0x%02x %d\n", (int)(i >> 1), ch, ch);
	}

	return 0;
}


/***
 * NAME
 *   decode_frame -
 *
 * ARGUMENTS
 *   data -
 *   len  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static int decode_frame(const char *data, size_t len)
{
	static const struct {
		const char           *msg;
		enum spoe_frame_type  type;
	} frame_type[] = {
		{ "UNSET",              SPOE_FRM_T_UNSET          },
		{ "HAPROXY-HELLO",      SPOE_FRM_T_HAPROXY_HELLO  },
		{ "HAPROXY-DISCONNECT", SPOE_FRM_T_HAPROXY_DISCON },
		{ "HAPROXY-NOTIFY",     SPOE_FRM_T_HAPROXY_NOTIFY },
		{ "AGENT-HELLO",        SPOE_FRM_T_AGENT_HELLO    },
		{ "AGENT-DISCONNECT",   SPOE_FRM_T_AGENT_DISCON   },
		{ "AGENT-ACK",          SPOE_FRM_T_AGENT_ACK      }
	};
	uint8_t           *buffer = NULL;
	ssize_t            buflen;
	struct spoe_frame  frame;
	const char        *ptr, *end;
	int                i, rc, retval = FUNC_RET_ERROR;

	if (data == NULL)
		return retval;
	else if (len == 0)
		return FUNC_RET_OK;

	(void)printf("%.*s\n", (int)len, data);

	buflen = hex2uint8(data, len, &buffer);
	if (_ERROR(buflen))
		return retval;

	(void)memset(&frame, 0, sizeof(frame));
	SPOE_FRAME_BUFFER_SET(&frame, (typeof(frame.buf))buffer, 0, buflen, 0);
	LIST_INIT(&(frame.list));

	for (i = 0; i < TABLESIZE(frame_type); i++)
		if (frame_type[i].type == (enum spoe_frame_type)*(frame.buf))
			break;

	/* Invalid frame type. */
	if (i >= TABLESIZE(frame_type))
		return retval;

	flag_log_nl = 1;

	rc = spoe_decode_frame(frame_type[i].msg, &frame, frame_type[i].type, FUNC_RET_ERROR, SPOE_DEC_END);
	if (_ERROR(rc))
		return retval;

	flag_log_nl = 0;

	ptr = frame.buf + rc;
	end = frame.buf + frame.len;

	while (_nERROR(rc) && (ptr < end)) {
		if (frame_type[i].type == SPOE_FRM_T_UNSET) {
			/* Fragmented frame. */
			rc = FUNC_RET_OK;

			break;
		}
		else if (frame_type[i].type == SPOE_FRM_T_HAPROXY_HELLO) {
			rc = spoe_decode_kv(&frame, &ptr, end,
			                    SPOE_DEC_STR, STR_ADDRSIZE("supported-versions"), cb_spoe_dec_str,
			                    SPOE_DEC_VARINT, STR_ADDRSIZE("max-frame-size"), cb_spoe_dec_varint,
			                    SPOE_DEC_UINT8, STR_ADDRSIZE("healthcheck"), cb_spoe_dec_uint8,
			                    SPOE_DEC_STR, STR_ADDRSIZE("capabilities"), cb_spoe_dec_str,
			                    SPOE_DEC_STR, STR_ADDRSIZE("engine-id"), cb_spoe_dec_str,
			                    SPOE_DEC_END);
		}
		else if (frame_type[i].type == SPOE_FRM_T_HAPROXY_DISCON) {
			rc = spoe_decode_kv(&frame, &ptr, end,
			                    SPOE_DEC_VARINT, STR_ADDRSIZE("status-code"), cb_spoe_dec_varint,
			                    SPOE_DEC_STR, STR_ADDRSIZE("message"), cb_spoe_dec_str,
			                    SPOE_DEC_END);
		}
		else if (frame_type[i].type == SPOE_FRM_T_HAPROXY_NOTIFY) {
			union spoe_data      value_data;
			enum spoe_data_type  value_type;
			const char          *name_str;
			uint64_t             name_len;
			uint8_t              nbargs;
			char                 addr[MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
			int                  n;

			/* Decode the message name. */
			rc = spoe_decode(&frame, &ptr, end, SPOE_DEC_STR0, &name_str, &name_len, SPOE_DEC_END);
			if (_ERROR(rc) || _NULL(name_str))
				break;

			(void)printf("Message: '%.*s'\n", (int)name_len, name_str);

			/* Decode the argument count. */
			rc = spoe_decode(&frame, &ptr, end, SPOE_DEC_UINT8, &nbargs, SPOE_DEC_END);
			if (_ERROR(rc))
				break;

			/* Decode individual arguments. */
			for (n = 0; _nERROR(rc) && (n < nbargs); n++) {
				rc = spoe_decode(&frame, &ptr, end,
				                 SPOE_DEC_STR0, &name_str, &name_len,     /* arg name */
				                 SPOE_DEC_DATA, &value_data, &value_type, /* arg value */
				                 SPOE_DEC_END);
				if (_ERROR(rc)) {
					break;
				}
				else if (value_type == SPOE_DATA_T_NULL) {
					(void)printf("  name='%.*s' value=NULL NULL\n", (int)name_len, name_str);
				}
				else if (value_type == SPOE_DATA_T_BOOL) {
					(void)printf("  name='%.*s' value=%02hhx BOOL\n", (int)name_len, name_str, value_data.boolean);
				}
				else if (value_type == SPOE_DATA_T_INT32) {
					(void)printf("  name='%.*s' value=%d INT32\n", (int)name_len, name_str, value_data.int32);
				}
				else if (value_type == SPOE_DATA_T_UINT32) {
					(void)printf("  name='%.*s' value=%08x UINT32\n", (int)name_len, name_str, value_data.uint32);
				}
				else if (value_type == SPOE_DATA_T_INT64) {
					(void)printf("  name='%.*s' value=%"PRId64" INT64\n", (int)name_len, name_str, value_data.int64);
				}
				else if (value_type == SPOE_DATA_T_UINT64) {
					(void)printf("  name='%.*s' value=%016"PRIx64" UINT64\n", (int)name_len, name_str, value_data.uint64);
				}
				else if (value_type == SPOE_DATA_T_IPV4) {
					if (_NULL(inet_ntop(AF_INET, &(value_data.ipv4), addr, INET_ADDRSTRLEN)))
						rc = FUNC_RET_ERROR;

					(void)printf("  name='%.*s' value='%s' IPV4\n", (int)name_len, name_str, _ERROR(rc) ? "invalid" : addr);
				}
				else if (value_type == SPOE_DATA_T_IPV6) {
					if (_NULL(inet_ntop(AF_INET6, &(value_data.ipv6), addr, INET6_ADDRSTRLEN)))
						rc = FUNC_RET_ERROR;

					(void)printf("  name='%.*s' value='%s' IPV6\n", (int)name_len, name_str, _ERROR(rc) ? "invalid" : addr);
				}
				else if (value_type == SPOE_DATA_T_STR) {
					(void)printf("  name='%.*s' value='%.*s' STR\n", (int)name_len, name_str, (int)value_data.chk.len, value_data.chk.ptr);
				}
				else if (value_type == SPOE_DATA_T_BIN) {
					(void)printf("  name='%.*s' value=<%s> <%s> BIN\n", (int)name_len, name_str, str_hex(value_data.chk.ptr, value_data.chk.len), str_ctrl(value_data.chk.ptr, value_data.chk.len));
				}
				else {
					(void)printf("  name='%.*s' value=Invalid data type: %hhu\n", (int)name_len, name_str, value_type);

					rc = FUNC_RET_ERROR;
				}
			}
		}
		else if (frame_type[i].type == SPOE_FRM_T_AGENT_HELLO) {
			rc = spoe_decode_kv(&frame, &ptr, end,
			                    SPOE_DEC_STR, STR_ADDRSIZE("version"), cb_spoe_dec_str,
			                    SPOE_DEC_VARINT, STR_ADDRSIZE("max-frame-size"), cb_spoe_dec_varint,
			                    SPOE_DEC_STR, STR_ADDRSIZE("capabilities"), cb_spoe_dec_str,
			                    SPOE_DEC_END);
		}
		else if (frame_type[i].type == SPOE_FRM_T_AGENT_DISCON) {
			rc = spoe_decode_kv(&frame, &ptr, end,
			                    SPOE_DEC_VARINT, STR_ADDRSIZE("status-code"), cb_spoe_dec_varint,
			                    SPOE_DEC_STR, STR_ADDRSIZE("message"), cb_spoe_dec_str,
			                    SPOE_DEC_END);
		}
		else if (frame_type[i].type == SPOE_FRM_T_AGENT_ACK) {
			/* This frame does not have any extra data. */
			rc = FUNC_RET_OK;

			break;
		}
		else {
			/* Error. */
			rc = FUNC_RET_ERROR;
		}
	}

	if (_nERROR(rc)) {
		(void)printf("%sThe frame is %scompletely decoded.\n", (ptr == end) ? "" : "WARNING: ", (ptr == end) ? "" : "not ");

		retval = FUNC_RET_OK;
	}

	return retval;
}


/***
 * NAME
 *   main -
 *
 * ARGUMENTS
 *   argv -
 *   argc -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
int main(int argc, char **argv)
{
	bool_t flag_error = 0;
	int    c, retval = EX_OK;

	_prg.name = basename(argv[0]);

	while ((c = getopt(argc, argv, ":f:s:hV")) != EOF) {
		if (c == 'f')
			_prg.frame_data = optarg;
		else if (c == 's')
			_prg.bin_data = optarg;
		else if (c == 'h')
			_prg.opt_flags |= FLAG_OPT_HELP;
		else if (c == 'V')
			_prg.opt_flags |= FLAG_OPT_VERSION;
		else
			flag_error = 1;
	}

	if (_prg.opt_flags & FLAG_OPT_HELP) {
		usage(_prg.name, 1);
	}
	else if (_prg.opt_flags & FLAG_OPT_VERSION) {
		(void)printf("\n%s v%s [build %d] by %s, %s\n\n", _prg.name, PACKAGE_VERSION, PACKAGE_BUILD, PACKAGE_AUTHOR, __DATE__);
	}
	else if ((_prg.bin_data == NULL) && (_prg.frame_data == NULL)) {
		flag_error = 1;

		usage(_prg.name, 0);
	}

	if (flag_error || (_prg.opt_flags & (FLAG_OPT_HELP | FLAG_OPT_VERSION)))
		return flag_error ? EX_USAGE : EX_OK;

	if (_prg.bin_data != NULL)
		retval = decode_bin(_prg.bin_data, strlen(_prg.bin_data));
	else
		retval = decode_frame(_prg.frame_data, strlen(_prg.frame_data));

	return retval;
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
