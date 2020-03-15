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


/***
 * NAME
 *   spoa_msg_iprep -
 *
 * ARGUMENTS
 *   frame    -
 *   buf      -
 *   end      -
 *   ip_score -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
int spoa_msg_iprep(struct spoe_frame *frame, const char **buf, const char *end, int *ip_score)
{
	union spoe_data      data;
	enum spoe_data_type  type;
	char                 addr[MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
	const char          *ptr = *buf, *str;
	uint64_t             len;
	uint8_t              nbargs;
	int                  retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %p, %p", frame, DPTR_ARGS(buf), end, ip_score);

	if (_ERROR(spoe_decode(frame, &ptr, end,
	                       SPOE_DEC_UINT8, &nbargs,
	                       SPOE_DEC_STR0, &str, &len,   /* arg name */
	                       SPOE_DEC_DATA, &data, &type, /* arg value */
	                       SPOE_DEC_END))) {
		retval = FUNC_RET_ERROR;
	}
	else if (nbargs != 1) {
		return 0;
	}
	else if (type == SPOE_DATA_T_IPV4) {
		if (_nNULL(inet_ntop(AF_INET, &(data.ipv4), addr, INET_ADDRSTRLEN)))
			retval = random() % 101;

		F_DBG(SPOA, frame, "IPv4 score for %.*s is %d", INET_ADDRSTRLEN, addr, retval);
	}
	else if (type == SPOE_DATA_T_IPV6) {
		if (_nNULL(inet_ntop(AF_INET6, &(data.ipv6), addr, INET6_ADDRSTRLEN)))
			retval = random() % 101;

		F_DBG(SPOA, frame, "IPv6 score for %.*s is %d", INET6_ADDRSTRLEN, addr, retval);
	}
	else {
		return 0;
	}

	*ip_score = retval;

	SPOE_BUFFER_ADVANCE(retval);

	return retval;
}


/***
 * NAME
 *   spoa_msg_iprep_action -
 *
 * ARGUMENTS
 *   frame    -
 *   buf      -
 *   ip_score -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void spoa_msg_iprep_action(struct spoe_frame *frame, char **buf, int ip_score)
{
	DBG_FUNC(FW_PTR, "%p, %p:%p, %d", frame, DPTR_ARGS(buf), ip_score);

	F_DBG(SPOA, frame, "Add action: set variable ip_score=%d", ip_score);

	(void)spoe_encode(frame, buf,
	                  SPOE_ENC_UINT8, SPOE_ACT_T_SET_VAR, /* Action type */
	                  SPOE_ENC_UINT8, 3,                  /* Number of args */
	                  SPOE_ENC_UINT8, SPOE_SCOPE_SESS,    /* Arg 1: the scope */
	                  SPOE_ENC_STR, "ip_score", 8,        /* Arg 2: variable name */
	                  SPOE_ENC_UINT8, SPOE_DATA_T_UINT32, /* Arg 3: variable type */
	                  SPOE_ENC_VARINT, ip_score,          /*        variable value */
	                  SPOE_ENC_END);
}


/***
 * NAME
 *   spoa_msg_test -
 *
 * ARGUMENTS
 *   frame -
 *   buf   -
 *   end   -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
int spoa_msg_test(struct spoe_frame *frame, const char **buf, const char *end)
{
	union spoe_data      data;
	enum spoe_data_type  type;
	char                 addr[MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
	const char          *ptr = *buf, *str;
	uint64_t             len;
	uint8_t              nbargs;
	int                  i, retval = FUNC_RET_OK;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %p", frame, DPTR_ARGS(buf), end);

	retval = spoe_decode(frame, &ptr, end, SPOE_DEC_UINT8, &nbargs, SPOE_DEC_END);
	if (_nERROR(retval))
		F_DBG(SPOA, frame, "%hhu arg(s) expected", nbargs);

	for (i = 0; _nERROR(retval) && (i < nbargs); i++) {
		retval = spoe_decode(frame, &ptr, end,
		                     SPOE_DEC_STR0, &str, &len,   /* arg name */
		                     SPOE_DEC_DATA, &data, &type, /* arg value */
		                     SPOE_DEC_END);
		if (_ERROR(retval)) {
			break;
		}
		else if (type == SPOE_DATA_T_NULL) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu:", i, (int)len, str, type);
		}
		else if (type == SPOE_DATA_T_BOOL) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: %02hhx", i, (int)len, str, type, data.boolean);
		}
		else if (type == SPOE_DATA_T_INT32) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: %d", i, (int)len, str, type, data.int32);
		}
		else if (type == SPOE_DATA_T_UINT32) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: %08x", i, (int)len, str, type, data.uint32);
		}
		else if (type == SPOE_DATA_T_INT64) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: %"PRId64, i, (int)len, str, type, data.int64);
		}
		else if (type == SPOE_DATA_T_UINT64) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: %016"PRIx64, i, (int)len, str, type, data.uint64);
		}
		else if (type == SPOE_DATA_T_IPV4) {
			if (_nNULL(inet_ntop(AF_INET, &(data.ipv4), addr, INET_ADDRSTRLEN)))
				retval = FUNC_RET_ERROR;

			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: \"%s\"", i, (int)len, str, type, addr);
		}
		else if (type == SPOE_DATA_T_IPV6) {
			if (_nNULL(inet_ntop(AF_INET6, &(data.ipv6), addr, INET6_ADDRSTRLEN)))
				retval = FUNC_RET_ERROR;

			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: \"%s\"", i, (int)len, str, type, addr);
		}
		else if (type == SPOE_DATA_T_STR) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: \"%.*s\"", i, (int)len, str, type, (int)data.chk.len, data.chk.ptr);
		}
		else if (type == SPOE_DATA_T_BIN) {
			F_DBG(SPOA, frame, "test[%d] name='%.*s' type=%hhu: <%s> <%s>", i, (int)len, str, type, str_hex(data.chk.ptr, data.chk.len), str_ctrl(data.chk.ptr, data.chk.len));
		}
		else {
			f_log(frame, _E("test[%d] name='%.*s': Invalid argument data type: %hhu"), i, (int)len, str, type);

			retval = FUNC_RET_ERROR;
		}
	}

	SPOE_BUFFER_ADVANCE(retval);

	return retval;
}


/***
 * NAME
 *   spoa_msg_arg_hdrs -
 *
 * ARGUMENTS
 *   frame -
 *   buf   -
 *   end   -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
static struct list *spoa_msg_arg_hdrs(struct spoe_frame *frame, const char *buf, const char *end)
{
	struct buffer *hdr = NULL, *hdr_back;
	struct list   *retptr = NULL;
	const char    *str;
	uint64_t       len;
	int            i, rc;

	DBG_FUNC(FW_PTR, "%p, %p, %p", frame, buf, end);

	if (_NULL(retptr = calloc(1, sizeof(*retptr))))
		return retptr;

	LIST_INIT(retptr);

	/* Build the HTTP headers. */
	for (i = 0; buf < end; i++) {
		rc = spoe_decode(frame, &buf, end, SPOE_DEC_STR0, &str, &len, SPOE_DEC_END);
		if (_ERROR(rc))
			break;

		F_DBG(SPOA, frame, "str[%d]: <%.*s>", i, (int)len, str);

		if (i & 1) {
			if (_NULL(str)) {
				/* HTTP header has no value. */
				if (_ERROR(rc = buffer_grow(hdr, ";\0", 2)))
					break;
			}
			else if (_ERROR(rc = buffer_grow_va(hdr, ": ", 2, str, len, "\0", 1, NULL))) {
				break;
			}

			F_DBG(SPOA, frame, "header[%d]: <%.*s>", i / 2, (int)hdr->len, hdr->ptr);
			LIST_ADDQ(retptr, &(hdr->list));
		}
		else if (_NULL(str)) {
			if (buf != end) {
				/* HTTP header has no name. */
				f_log(frame, _E("HTTP header defined without a name"));

				rc = FUNC_RET_ERROR;
			}

			break;
		}
		else if (_NULL(hdr = buffer_alloc(cfg.max_frame_size, str, len, NULL))) {
			break;
		}
	}

	/* In the case of a fault, the allocated memory is released. */
	if (_ERROR(rc) || _NULL(hdr)) {
		buffer_ptr_free(&hdr);

		list_for_each_entry_safe(hdr, hdr_back, retptr, list)
			buffer_ptr_free(&hdr);

		PTR_FREE(retptr);
	}

	return retptr;
}


/***
 * NAME
 *   spoa_msg_mirror -
 *
 * ARGUMENTS
 *   frame -
 *   buf   -
 *   end   -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
int spoa_msg_mirror(struct spoe_frame *frame, const char **buf, const char *end)
{
	union spoe_data      data;
	enum spoe_data_type  type;
	struct mirror       *mir;
	const char          *ptr = *buf, *str;
	uint64_t             len;
	uint8_t              nbargs;
	int                  i, retval = FUNC_RET_ERROR;

	DBG_FUNC(FW_PTR, "%p, %p:%p, %p", frame, DPTR_ARGS(buf), end);

	if (_NULL(mir = calloc(1, sizeof(*mir)))) {
		f_log(frame, _E("Failed to allocate memory"));

		return retval;
	}

	retval = spoe_decode(frame, &ptr, end, SPOE_DEC_UINT8, &nbargs, SPOE_DEC_END);
	if (_nERROR(retval))
		F_DBG(SPOA, frame, "%hhu arg(s) expected", nbargs);

	for (i = 0; _nERROR(retval) && (i < nbargs); i++) {
		retval = spoe_decode(frame, &ptr, end,
		                     SPOE_DEC_STR0, &str, &len,   /* arg name */
		                     SPOE_DEC_DATA, &data, &type, /* arg value */
		                     SPOE_DEC_END);
		if (_ERROR(retval)) {
			break;
		}
		else if (type == SPOE_DATA_T_STR) {
			char **mir_ptr;

			F_DBG(SPOA, frame, "mirror[%d] name='%.*s' type=%hhu: \"%.*s\"", i, (int)len, str, type, (int)data.chk.len, data.chk.ptr);

			if ((len == STR_SIZE(SPOE_MSG_ARG_METHOD)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_ARG_METHOD)) == 0))
				mir_ptr = &(mir->method);
			else if ((len == STR_SIZE(SPOE_MSG_ARG_PATH)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_ARG_PATH)) == 0))
				mir_ptr = &(mir->path);
			else if ((len == STR_SIZE(SPOE_MSG_ARG_VER)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_ARG_VER)) == 0))
				mir_ptr = &(mir->version);
			else {
				f_log(frame, _W("Unknown argument, ignored: '%.*s'"), (int)len, str);

				continue;
			}

			if (_nNULL(*mir_ptr)) {
				f_log(frame, _W("Duplicated argument, ignored: '%.*s'"), (int)len, str);
			}
			else if (_NULL(*mir_ptr = mem_dup(data.chk.ptr, data.chk.len))) {
				f_log(frame, _E("Failed to allocate memory for headers"));

				retval = FUNC_RET_ERROR;
			}
		}
		else if (type == SPOE_DATA_T_BIN) {
			F_DBG(SPOA, frame, "mirror[%d] name='%.*s' type=%hhu: <%s> <%s>", i, (int)len, str, type, str_hex(data.chk.ptr, data.chk.len), str_ctrl(data.chk.ptr, data.chk.len));

			if ((len == STR_SIZE(SPOE_MSG_ARG_HDRS)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_ARG_HDRS)) == 0)) {
				if (_nNULL(mir->hdrs))
					f_log(frame, _W("Duplicated argument, ignored: '%.*s'"), (int)len, str);
				else if (_NULL(mir->hdrs = spoa_msg_arg_hdrs(frame, data.chk.ptr, data.chk.ptr + data.chk.len - 1)))
					retval = FUNC_RET_ERROR;
			}
			else if ((len == STR_SIZE(SPOE_MSG_ARG_BODY)) && (memcmp(str, STR_ADDRSIZE(SPOE_MSG_ARG_BODY)) == 0)) {
				if (_nNULL(mir->body)) {
					f_log(frame, _W("Duplicated argument, ignored: '%.*s'"), (int)len, str);
				}
				else if (_nNULL(data.chk.ptr) && (data.chk.len != 0)) {
					mir->body_size = data.chk.len;

					if (_NULL(mir->body = mem_dup(data.chk.ptr, data.chk.len))) {
						f_log(frame, _E("Failed to allocate memory for body"));

						retval = FUNC_RET_ERROR;
					}
				}
			}
			else {
				f_log(frame, _W("Unknown argument, ignored: '%.*s'"), (int)len, str);
			}
		}
		else {
			f_log(frame, _E("mirror[%d] name='%.*s': Invalid argument data type: %hhu"), i, (int)len, str, type);

			retval = FUNC_RET_ERROR;
		}
	}

#ifdef HAVE_LIBCURL
	if (_nERROR(retval) && _nNULL(cfg.mir_url)) {
		int url_len, rc;

		retval = FUNC_RET_ERROR;

		if (_nNULL(mir->path))
			url_len = strlen(cfg.mir_url) + strlen(mir->path) + 1;

		if (_NULL(mir->path))
			f_log(frame, _E("HTTP path not set"));
		else if (_NULL(mir->method))
			f_log(frame, _E("HTTP request method not set"));
		else if (_NULL(mir->version))
			f_log(frame, _E("HTTP version not set"));
		else if (_NULL(mir->hdrs))
			f_log(frame, _E("HTTP headers not set"));
		else if (_NULL(mir->url = malloc(url_len)))
			f_log(frame, _E("Failed to allocate memory"));
		else if (((rc = snprintf(mir->url, url_len, "%s%s", cfg.mir_url, mir->path)) >= url_len) || (rc < 0))
			f_log(frame, (rc < 0) ? _E("Failed to construct URL: %m") : _E("URL too long"));
		else {
#define CURL_HTTP_METHOD_DEF(a)   { #a, TABLESIZE_1(#a) },
			static const struct {
				const char *name;
				size_t      len;
			} http_method[] = { CURL_HTTP_METHOD_DEFINES };
#undef CURL_HTTP_METHOD_DEF

			for (i = 0; i < TABLESIZE(http_method); i++)
				if (strncasecmp(mir->method, http_method[i].name, http_method[i].len) == 0) {
					mir->request_method = i;

					break;
				}

			if (i < TABLESIZE(http_method))
				retval = mir_curl_add(&(FW_PTR->curl), mir);
			else
				f_log(frame, _E("Invalid HTTP request method"));
		}
	}

	if (_ERROR(retval) || _NULL(cfg.mir_url))
		mir_ptr_free(&mir);

#else

	mir_ptr_free(&mir);
#endif /* HAVE_LIBCURL */

	SPOE_BUFFER_ADVANCE(retval);

	return retval;
}


/***
 * NAME
 *   mir_ptr_free -
 *
 * ARGUMENTS
 *   data -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void mir_ptr_free(struct mirror **data)
{
	struct buffer *hdr = NULL, *hdr_back;

	DBG_FUNC(NULL, "%p:%p", DPTR_ARGS(data));

	if (_NULL(data) || _NULL(*data))
		return;

	W_DBG(NOTICE, NULL, "  freeing mirror { \"%s\" \"%s\" \"%s\" %d \"%s\" %p %p %zu/%zu }", (*data)->url, (*data)->path, (*data)->method, (*data)->request_method, (*data)->version, (*data)->hdrs, (*data)->body, (*data)->body_head, (*data)->body_size);

	PTR_FREE((*data)->out_address);
	PTR_FREE((*data)->url);
	PTR_FREE((*data)->path);
	PTR_FREE((*data)->method);
	PTR_FREE((*data)->version);

	list_for_each_entry_safe(hdr, hdr_back, (*data)->hdrs, list)
		buffer_ptr_free(&hdr);

	PTR_FREE((*data)->hdrs);
	PTR_FREE((*data)->body);
	PTR_FREE(*data);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
