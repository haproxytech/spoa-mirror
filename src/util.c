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
 *   mem_dup - duplicate memory data
 *
 * ARGUMENTS
 *   s    - pointer to the data that is duplicated
 *   size - size of the data, in bytes
 *
 * DESCRIPTION
 *   Duplicate the first <size> bytes of the data pointed to by <s> into newly
 *   allocated memory.  One byte more than requested is allocated and the copy
 *   is terminated with the null character, so it can be used as a string too.
 *
 * RETURN VALUE
 *   It returns a pointer to the duplicated data, or a NULL pointer in case of
 *   the error.  The returned memory has to be released using free().
 */
void *mem_dup(const void *s, size_t size)
{
	void *retptr = NULL;

	DBG_FUNC(NULL, "%p, %zu", s, size);

	if (_NULL(retptr = malloc(size + 1)))
		DBG_RETURN_PTR(retptr);

	(void)memcpy(retptr, s, size);
	((uint8_t *)retptr)[size] = '\0';

	DBG_RETURN_PTR(retptr);
}


/***
 * NAME
 *   buffer_init - initialize a buffer
 *
 * ARGUMENTS
 *   data - pointer to the buffer structure
 *
 * DESCRIPTION
 *   Initialize the buffer <data>.  All fields of the structure are cleared and
 *   its list is set to an empty one, without releasing anything, so only a
 *   buffer whose data area is not allocated may be initialized in this way.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void buffer_init(struct buffer *data)
{
	DBG_FUNC(NULL, "%p", data);

	if (_NULL(data))
		DBG_RETURN();

	W_DBG(NOTICE, NULL, "initializing buffer { { %p %p } %p %zu/%zu }", data->list.p, data->list.n, data->ptr, data->len, data->size);

	(void)memset(data, 0, sizeof(*data));
	LIST_INIT(&(data->list));

	DBG_RETURN();
}


/***
 * NAME
 *   buffer_free - release the data of a buffer
 *
 * ARGUMENTS
 *   data - pointer to the buffer structure
 *
 * DESCRIPTION
 *   Release the data area allocated in the buffer <data>, remove the buffer
 *   from the list it is on and initialize the buffer structure again.  The
 *   buffer structure itself is not released.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void buffer_free(struct buffer *data)
{
	DBG_FUNC(NULL, "%p", data);

	if (_NULL(data))
		DBG_RETURN();

	W_DBG(NOTICE, NULL, "freeing buffer { { %p %p } %p %zu/%zu }", data->list.p, data->list.n, data->ptr, data->len, data->size);

	if (_nNULL(data->list.p) && _nNULL(data->list.n))
		LIST_DEL(&(data->list));
	if (data->size > 0)
		PTR_FREE(data->ptr);
	(void)memset(data, 0, sizeof(*data));
	LIST_INIT(&(data->list));

	DBG_RETURN();
}


/***
 * NAME
 *   buffer_alloc - allocate a buffer
 *
 * ARGUMENTS
 *   size - size of the buffer data area, in bytes
 *   src  - pointer to the data that is added to the buffer
 *
 * DESCRIPTION
 *   Allocate a buffer structure together with its data area of <size> bytes,
 *   then add the data pointed to by <src> to it.  The pointer <src> is followed
 *   by the number of the bytes that are added, and any number of such pairs may
 *   be given, the last pointer being NULL.  The whole buffer is released if any
 *   of the data cannot be added to it.
 *
 * RETURN VALUE
 *   It returns a pointer to the allocated buffer, or a NULL pointer in case of
 *   the error.  The buffer has to be released using buffer_ptr_free().
 */
struct buffer *buffer_alloc(size_t size, const void *src, ...)
{
	va_list        ap;
	struct buffer *retptr;

	DBG_FUNC(NULL, "%zu, %p, ...", size, src);

	if (_NULL(retptr = calloc(1, sizeof(*retptr)))) {
		/* Do nothing. */
	}
	else if ((size > 0) && _NULL(retptr->ptr = calloc(1, size))) {
		PTR_FREE(retptr);
	}
	else {
		LIST_INIT(&(retptr->list));
		retptr->size = size;

		va_start(ap, src);
		for ( ; _nNULL(src); src = va_arg(ap, typeof(src))) {
			size_t n = va_arg(ap, typeof(n));

			if (_ERROR(buffer_grow(retptr, src, n))) {
				buffer_ptr_free(&retptr);

				break;
			}
		}
		va_end(ap);
	}

	DBG_RETURN_PTR(retptr);
}


/***
 * NAME
 *   buffer_ptr_free - release a buffer
 *
 * ARGUMENTS
 *   data - pointer to the buffer structure pointer
 *
 * DESCRIPTION
 *   Release the buffer <*data> together with the data area allocated in it, and
 *   set the pointer <*data> to a NULL pointer.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void buffer_ptr_free(struct buffer **data)
{
	DBG_FUNC(NULL, "%p:%p", DPTR_ARGS(data));

	if (_NULL(data) || _NULL(*data))
		DBG_RETURN();

	buffer_free(*data);
	PTR_FREE(*data);

	DBG_RETURN();
}


/***
 * NAME
 *   buffer_grow - add data to a buffer
 *
 * ARGUMENTS
 *   data - pointer to the buffer structure
 *   src  - pointer to the data that is added to the buffer
 *   n    - number of the data bytes that are added to the buffer
 *
 * DESCRIPTION
 *   Add <n> bytes of the data pointed to by <src> to the end of the buffer
 *   <data>.  If there is not enough free space in the buffer, the data area is
 *   enlarged by <n> bytes rounded up to a multiple of 32.  When <src> is a NULL
 *   pointer, the buffer is only enlarged and the added space is cleared.
 *
 * RETURN VALUE
 *   It returns the length of the data in the buffer, or FUNC_RET_ERROR (-1) in
 *   case of the error.
 */
ssize_t buffer_grow(struct buffer *data, const void *src, size_t n)
{
	uint8_t *ptr;
	size_t   size = ALIGN_VALUE(n, 5);
	int      retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "%p, %p, %zu", data, src, n);

	if (_NULL(data))
		DBG_RETURN_SSIZE(retval);

	if (_NULL(data->ptr))
		buffer_init(data);

	if (n == 0) {
		retval = data->len;
	}
	else if (_nNULL(data->ptr) && ((data->size - data->len) >= n)) {
		if (_nNULL(src)) {
			/* Copying src data to buffer. */
			(void)memcpy(data->ptr + data->len, src, n);

			data->len += n;
		}

		retval = data->len;
	}
	else if (_NULL(ptr = realloc(data->ptr, data->size + size))) {
		w_log(NULL, _E("Failed to allocate data: %m"));
	}
	else {
		W_DBG(NOTICE, NULL, "reallocating buffer { %p %zu/%zu } -> { %p %zu/%zu }",
		      data->ptr, data->len, data->size, ptr, data->len + (_NULL(src) ? 0 : n), data->size + size);

		if (_NULL(src)) {
			/* Clearing allocated buffer. */
			(void)memset(ptr + data->size, 0, size);

			data->size += size;
		} else {
			/* Copying src data to buffer. */
			(void)memcpy(ptr + data->len, src, n);

			data->len  += n;
			data->size += size;
		}

		data->ptr = ptr;

		retval = data->len;
	}

	DBG_RETURN_SSIZE(retval);
}


/***
 * NAME
 *   buffer_grow_va - add more data blocks to a buffer
 *
 * ARGUMENTS
 *   data - pointer to the buffer structure
 *   src  - pointer to the data that is added to the buffer
 *   n    - number of the data bytes that are added to the buffer
 *
 * DESCRIPTION
 *   Add the data given as pointer/size pairs to the end of the buffer <data>,
 *   in the same way as buffer_grow() does.  Any number of the pairs may be
 *   given after <data>, the last pointer being NULL.
 *
 * RETURN VALUE
 *   It returns the length of the data in the buffer, or FUNC_RET_ERROR (-1) in
 *   case of the error.
 */
ssize_t buffer_grow_va(struct buffer *data, const void *src, size_t n, ...)
{
	va_list ap;
	ssize_t retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "%p, %p, %zu, ...", data, src, n);

	if (_NULL(data))
		DBG_RETURN_SSIZE(retval);

	va_start(ap, n);
	while (_nNULL(src) && (n > 0)) {
		if (_ERROR(retval = buffer_grow(data, src, n)))
			break;

		if (_nNULL(src = va_arg(ap, typeof(src))))
			n = va_arg(ap, typeof(n));
	}
	va_end(ap);

	DBG_RETURN_SSIZE(retval);
}


/***
 * NAME
 *   str_hex -
 *
 * ARGUMENTS
 *   data -
 *   size -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
const char *str_hex(const void *data, size_t size)
{
	static __THR char  retbuf[BUFSIZ];
	const uint8_t     *ptr = data;
	size_t             i;

	if (_NULL(data))
		return "(null)";
	else if (size == 0)
		return "()";

	for (i = 0, size <<= 1; (i < SIZEOF_N(retbuf, 2)) && (i < size); ptr++) {
		retbuf[i++] = NIBBLE_TO_HEX(*ptr >> 4);
		retbuf[i++] = NIBBLE_TO_HEX(*ptr & 0x0f);
	}

	retbuf[i] = '\0';

	return retbuf;
}


/***
 * NAME
 *   str_ctrl -
 *
 * ARGUMENTS
 *   data -
 *   size -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
const char *str_ctrl(const void *data, size_t size)
{
	static __THR char  retbuf[BUFSIZ];
	const uint8_t     *ptr = data;
	size_t             i, n = 0;

	if (_NULL(data))
		return "(null)";
	else if (size == 0)
		return "()";

	for (i = 0; (n < SIZEOF_N(retbuf, 1)) && (i < size); i++)
		retbuf[n++] = IN_RANGE(ptr[i], 0x20, 0x7e) ? ptr[i] : '.';

	retbuf[n] = '\0';

	return retbuf;
}


/***
 * NAME
 *   str_toull -
 *
 * ARGUMENTS
 *   str      -
 *   endptr   -
 *   flag_end -
 *   base     -
 *   value    -
 *   val_min  -
 *   val_max  -
 *
 * DESCRIPTION
 *   -
 *
 * RETURN VALUE
 *   -
 */
bool_t str_toull(const char *str, char **endptr, bool_t flag_end, int base, uint64_t *value, uint64_t val_min, uint64_t val_max)
{
	char   *ptr = NULL;
	bool_t  retval = false;

	if (_NULL(str) || _NULL(value))
		return retval;

	errno = 0;

	*value = strtoull(str, PTR_SAFE(endptr, &ptr), base);
	if ((*str == '\0') || (flag_end && (**PTR_SAFE(endptr, &ptr) != '\0')))
		w_log(NULL, _E("Not a number: '%s'"), str);
	else if ((*value == ULLONG_MAX) && (errno == ERANGE))
		w_log(NULL, _E("Value out of range: %s"), str);
	else if ((val_min <= val_max) && !IN_RANGE(*value, val_min, val_max))
		w_log(NULL, _E("Value out of range [%"PRIu64", %"PRIu64"]: %s"), val_min, val_max, str);
	else if ((*value == 0) && (errno == EINVAL))
		w_log(NULL, _E("Invalid value: %s"), str);
	else
		retval = true;

	return retval;
}


/***
 * NAME
 *   str_toll - convert a string to a signed integer
 *
 * ARGUMENTS
 *   str      - string that is converted
 *   endptr   - pointer to the first character after the number
 *   flag_end - whether the whole string has to be a number
 *   base     - base of the conversion, or 0 to take it from the string
 *   value    - pointer to the converted value
 *   val_min  - lowest allowed value
 *   val_max  - highest allowed value
 *
 * DESCRIPTION
 *   Convert the string <str> into a signed integer value and save the result in
 *   <*value>.  If <endptr> is not a NULL pointer, the address of the first
 *   character that does not belong to the number is stored there.  When the
 *   flag <flag_end> is set, the whole string has to be a number.  The result is
 *   checked against the range [<val_min>, <val_max>], but only if <val_min> is
 *   not greater than <val_max>.  Every error found is logged.
 *
 * RETURN VALUE
 *   It returns true if the string is converted, false otherwise.
 */
bool_t str_toll(const char *str, char **endptr, bool_t flag_end, int base, int64_t *value, int64_t val_min, int64_t val_max)
{
	char   *ptr = NULL;
	bool_t  retval = false;

	if (_NULL(str) || _NULL(value))
		return retval;

	errno = 0;

	*value = strtoll(str, PTR_SAFE(endptr, &ptr), base);
	if ((*str == '\0') || (flag_end && (**PTR_SAFE(endptr, &ptr) != '\0')))
		w_log(NULL, _E("Not a number: '%s'"), str);
	else if ((*value == LLONG_MAX) && (errno == ERANGE))
		w_log(NULL, _E("Value out of range: %s"), str);
	else if ((val_min <= val_max) && !IN_RANGE(*value, val_min, val_max))
		w_log(NULL, _E("Value out of range [%"PRIi64", %"PRIi64"]: %s"), val_min, val_max, str);
	else if ((*value == 0) && (errno == EINVAL))
		w_log(NULL, _E("Invalid value: %s"), str);
	else
		retval = true;

	return retval;
}


/***
 * NAME
 *   str_delay - convert a time interval to a string
 *
 * ARGUMENTS
 *   delay_us - time interval in microseconds
 *
 * DESCRIPTION
 *   Write the time interval <delay_us> into a static buffer, using the largest
 *   unit (seconds, milliseconds or microseconds) in which the interval can be
 *   shown.  Two buffers are used in turn, so the results of two calls may be
 *   used in the same expression.
 *
 * RETURN VALUE
 *   It returns a pointer to the written string.
 */
const char *str_delay(uint64_t delay_us)
{
	BUFFER2_DEF(char, retbuf, 2, 32);

	DBG_FUNC(NULL, "%"PRIu64, delay_us);

	if (delay_us > 1000000)
		(void)snprintf(BUFFER_ADDRSIZE(retbuf), "%.2fs", delay_us / 1000000.0);
	else if (delay_us > 1000)
		(void)snprintf(BUFFER_ADDRSIZE(retbuf), "%.2fms", delay_us / 1000.0);
	else if (delay_us > 0)
		(void)snprintf(BUFFER_ADDRSIZE(retbuf), "%"PRIu64"us", delay_us);
	else
		DBG_RETURN_CPTR("0");

	DBG_RETURN_CPTR(BUFFER(retbuf));
}


/***
 * NAME
 *   getopt_shortopts - compose the short options string
 *
 * ARGUMENTS
 *   longopts  - array of the long options, ended by a zeroed element
 *   shortopts - buffer in which the short options string is written
 *   size      - size of the buffer <shortopts>, in bytes
 *   flags     - flags that select the way the options are parsed
 *
 * DESCRIPTION
 *   Compose the short options string from the long options array <longopts>,
 *   adding a colon after every option that requires an argument.  The flags
 *   FLAG_GETOPT_POSIXLY_CORRECT and FLAG_GETOPT_NONOPTION_ARG add the character
 *   '+' or '-' at the beginning of the string, and the character ':' is added
 *   by the flag FLAG_GETOPT_DIST_ERRORS.
 *
 * RETURN VALUE
 *   It returns the number of the options written, or FUNC_RET_ERROR (-1) if an
 *   error occurred, in which case errno is set.
 */
int getopt_shortopts(const struct option *longopts, char *shortopts, size_t size, uint8_t flags)
{
	size_t i, n = 0, size_1 = size - 1;
	int    retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "%p, %p, %zu, 0x%02hhx", longopts, shortopts, size, flags);

	CHECK_ARG_NULL(longopts, return retval);
	CHECK_ARG_NULL(shortopts, return retval);
	CHECK_ARG_ERRNO(!IN_RANGE(size, 1, BUFSIZ), EINVAL, return retval);

	if ((flags & (FLAG_GETOPT_POSIXLY_CORRECT | FLAG_GETOPT_NONOPTION_ARG)) && (n < size_1))
		shortopts[n++] = (flags & FLAG_GETOPT_POSIXLY_CORRECT) ? '+' : '-';

	if ((flags & FLAG_GETOPT_DIST_ERRORS) && (n < size_1))
		shortopts[n++] = ':';

	for (i = 0; (n < size_1) && _nNULL(longopts[i].name); i++) {
		shortopts[n++] = longopts[i].val;

		if (longopts[i].has_arg == required_argument) {
			if (n < size_1)
				shortopts[n++] = ':';
			else
				break;
		}
	}

	if (_NULL(longopts[i].name)) {
		shortopts[n] = '\0';

		retval = i;
	} else {
		shortopts[MIN(n, size_1)] = '\0';

		errno = EINVAL;
	}

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   parse_delay_us - parse a time interval
 *
 * ARGUMENTS
 *   delay   - string with the time interval that is parsed
 *   val_min - lowest allowed time interval, in microseconds
 *   val_max - highest allowed time interval, in microseconds
 *
 * DESCRIPTION
 *   Convert the time interval written in the string <delay> to microseconds.
 *   The number may be followed by one of the units 'us', 'ms', 's', 'm', 'h' or
 *   'd'; when the unit is not given, milliseconds are used.  The result is
 *   checked against the range [<val_min>, <val_max>].
 *
 * RETURN VALUE
 *   It returns the time interval in microseconds, or ULLONG_MAX if an error
 *   occurred, in which case errno is set.
 */
uint64_t parse_delay_us(const char *delay, uint64_t val_min, uint64_t val_max)
{
	char     *endptr = NULL;
	uint64_t  retval;

	DBG_FUNC(NULL, "\"%s\", %"PRIu64", %"PRIu64, delay, val_min, val_max);

	if (!str_toull(delay, &endptr, 0, 10, &retval, 1, 0))
		/* Do nothing. */;
	else if (TEST_ARRAY3(endptr, 0, 'u', 's', '\0'))
		/* microseconds - no conversion */;
	else if ((endptr[0] == '\0') || TEST_ARRAY3(endptr, 0, 'm', 's', '\0'))
		PARSE_DELAY_US(1000);               /* No unit or milliseconds */
	else if (TEST_ARRAY2(endptr, 0, 's', '\0'))
		PARSE_DELAY_US(UINT64_C(1000000));         /* seconds */
	else if (TEST_ARRAY2(endptr, 0, 'm', '\0'))
		PARSE_DELAY_US(UINT64_C(1000000) * 60);    /* minutes */
	else if (TEST_ARRAY2(endptr, 0, 'h', '\0'))
		PARSE_DELAY_US(UINT64_C(1000000) * 3600);  /* hours */
	else if (TEST_ARRAY2(endptr, 0, 'd', '\0'))
		PARSE_DELAY_US(UINT64_C(1000000) * 86400); /* days */
	else
		errno = EINVAL;                     /* Invalid unit. */

	if (errno) {
		retval = ULLONG_MAX;
	}
	else if (!IN_RANGE(retval, val_min, val_max)) {
		errno  = ERANGE;
		retval = ULLONG_MAX;
	}

	DBG_RETURN_U64(retval);
}


/***
 * NAME
 *   parse_hostname - check a host name
 *
 * ARGUMENTS
 *   hostname - host name that is checked
 *
 * DESCRIPTION
 *   Check whether the host name <hostname> can be resolved, using a connection
 *   based protocol and allowing both the IPv4 and the IPv6 address family.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) if the host name <hostname> is resolved, and
 *   FUNC_RET_ERROR (-1) if it is not.
 */
int parse_hostname(const char *hostname)
{
	static const struct addrinfo hints =
	{
		.ai_flags    = AI_PASSIVE | AI_CANONNAME, /* Set the official name of the host */
		.ai_family   = AF_UNSPEC,                 /* Allow IPv4 or IPv6 */
		.ai_socktype = SOCK_STREAM,               /* Use connection-based protocol (TCP) */
		.ai_protocol = 0                          /* Any protocol */
	};
	struct addrinfo *res = NULL;
	int              rc, retval = FUNC_RET_OK;

	DBG_FUNC(NULL, "\"%s\"", hostname);

	if (_nOK(rc = getaddrinfo(hostname, NULL, &hints, &res))) {
		w_log(NULL, _E("Failed to get address info for '%s': %s"), hostname, gai_strerror(rc));

		retval = FUNC_RET_ERROR;
	}

	freeaddrinfo(res);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   parse_url - check and duplicate a URL
 *
 * ARGUMENTS
 *   url - URL that is parsed
 *
 * DESCRIPTION
 *   Check the URL <url> and duplicate it.  Only the schemes 'http' and 'https'
 *   are allowed, all trailing '/' characters are removed from the copy, and the
 *   host name and the port number, if it is given, are checked.
 *
 * RETURN VALUE
 *   It returns a pointer to the duplicated URL, or a NULL pointer in case of
 *   the error.  The returned memory has to be released using free().
 */
char *parse_url(const char *url)
{
	char     *retptr = NULL, *host, *port = NULL, *ptr, path;
	uint64_t  rc;
	size_t    len;
	bool_t    flag_https = 0;

	DBG_FUNC(NULL, "\"%s\"", url);

	if (_NULL(url))
		DBG_RETURN_PTR(retptr);

	len = strlen(url);

	if (strncasecmp(url, STR_ADDRSIZE(STR_HTTP_PFX)) == 0) {
		/* Do nothing. */
	}
	else if (strncasecmp(url, STR_ADDRSIZE(STR_HTTPS_PFX)) == 0) {
		flag_https = 1;
	}
	else {
		w_log(NULL, _E("Invalid URL scheme '%s'"), url);

		DBG_RETURN_PTR(retptr);
	}

	if (_NULL(retptr = mem_dup(url, len))) {
		w_log(NULL, _E("Failed to allocate data: %m"));

		DBG_RETURN_PTR(retptr);
	}

	/*
	 * The HTTP URL scheme is used to designate Internet resources
	 * accessible using HTTP (HyperText Transfer Protocol).
	 *
	 * An HTTP URL takes the form: http://<host>:<port>/<path>?<searchpart>
	 *
	 * <host> is the fully qualified domain name of a network host, or its
	 * IP address.  <port> is the port number to connect to.  If :<port>
	 * is omitted, the port defaults to 80.  <path> is an HTTP selector,
	 * and <searchpart> is a query string.  The <path> is optional, as
	 * is the <searchpart> and its preceding "?".  If neither <path> nor
	 * <searchpart> is present, the "/" may also be omitted.
	 *
	 * Within the <path> and <searchpart> components, "/", ";", "?" are
	 * reserved.  The "/" character may be used within HTTP to designate a
	 * hierarchical structure.
	 */
	host = retptr + (flag_https ? STR_SIZE(STR_HTTPS_PFX) : STR_SIZE(STR_HTTP_PFX));

	/* Remove all the trailing '/' characters from the URL. */
	for (ptr = retptr + len - 1; *ptr == '/'; *(ptr--) = '\0');

	/* Find the port number if it is defined. */
	for (ptr = host; TEST_NAND3(*ptr, '\0', ':', '/'); ptr++);
	if (*ptr == ':') {
		*(ptr++) = '\0';
		for (port = ptr; TEST_NAND2(*ptr, '\0', '/'); ptr++);
	}

	/*
	 * Save the beginning of the <path> part and terminate the end of
	 * <host> or <port> part.
	 */
	path = *ptr;
	*ptr = '\0';

	W_DBG(UTIL, NULL, "host: \"%s\", port: \"%s\", path: '%c'", host, port, path);

	if (_ERROR(parse_hostname(host))) {
		w_log(NULL, _E("Invalid hostname '%s'"), host);

		PTR_FREE(retptr);
	}
	else if (_nNULL(port)) {
		if (!str_toull(port, NULL, 1, 10, &rc, 0, 65535)) {
			w_log(NULL, _E("Invalid port number '%s'"), port);

			PTR_FREE(retptr);
		} else {
			/* Return the <host> terminator. */
			*(port - 1) = ':';
		}
	}

	if (_nNULL(retptr)) {
		/* Return the first character of the <path>. */
		*ptr = path;

		W_DBG(UTIL, NULL, "URL: \"%s\"", retptr);
	}

	DBG_RETURN_PTR(retptr);
}


/***
 * NAME
 *   thread_id - get the identifier of the calling worker
 *
 * ARGUMENTS
 *   This function takes no arguments.
 *
 * DESCRIPTION
 *   Look for the calling thread in the array of the worker threads, where the
 *   first worker has the identifier 1.  The result is used as the identifier of
 *   the worker in the log lines for which the worker is not known.
 *
 * RETURN VALUE
 *   It returns the identifier of the calling worker thread, or 0 if the thread
 *   is not one of the workers.
 */
static __always_inline int thread_id(void)
{
	pthread_t id;
	int       i, retval = 0;

	if (_NULL(prg.workers))
		return retval;

	id = pthread_self();

	for (i = 0; (retval == 0) && (i < cfg.num_workers); i++)
		if (pthread_equal((prg.workers + i)->thread, id))
			retval = i + 1;

	return retval;
}


/***
 * NAME
 *   time_elapsed - measure the elapsed time
 *
 * ARGUMENTS
 *   tv - point in time from which the interval is measured
 *
 * DESCRIPTION
 *   Compute the time interval between the current time and the time <tv>.  The
 *   current time is read using gettimeofday().
 *
 * RETURN VALUE
 *   It returns the number of the microseconds elapsed since the time <tv>, or
 *   the current time in microseconds if <tv> is a NULL pointer.
 */
uint64_t time_elapsed(const struct timeval *tv)
{
	struct timeval now;

	(void)gettimeofday(&now, NULL);

	return _NULL(tv) ? TIMEVAL_US(&now) : TIMEVAL_DIFF_US(&now, tv);
}


/***
 * NAME
 *   c_log - write a client log message
 *
 * ARGUMENTS
 *   client - client for which the message is written
 *   format - printf(3) style format of the message
 *
 * DESCRIPTION
 *   Write the message <format> to the standard output.  The message is prefixed
 *   with the identifier of the worker that serves the client <client>, the
 *   program runtime, and the identifier and the file descriptor of the client.
 *   If the client or its worker is not known, the identifier of the calling
 *   thread is used and the client data are left out.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void c_log(const struct client *client, const char *format, ...)
{
	va_list ap;
	char    fmt[BUFSIZ];
	double  runtime;

	runtime = LOG_RUNTIME(time_elapsed(&(prg.start_time)), time_elapsed(NULL));

	if (_NULL(client) || _NULL(CW_PTR))
		(void)snprintf(fmt, sizeof(fmt), LOG_FMT LOG_FMT_INDENT "%s\n",
		               thread_id(), runtime, LOG_INDENT format);
	else
		(void)snprintf(fmt, sizeof(fmt), LOG_FMT LOG_FMT_INDENT "<%lu:%d> %s\n",
		               CW_PTR->id, runtime, LOG_INDENT client->id, client->fd, format);

	va_start(ap, format);
	(void)vfprintf(stdout, fmt, ap);
	va_end(ap);
}


/***
 * NAME
 *   f_log - write a frame log message
 *
 * ARGUMENTS
 *   frame  - frame for which the message is written
 *   format - printf(3) style format of the message
 *
 * DESCRIPTION
 *   Write the message <format> to the standard output.  The message is prefixed
 *   with the identifier of the worker of the frame <frame>, the program runtime
 *   and the identifier of the client of the frame.  If the worker is not known,
 *   the identifier of the calling thread is used, and the client identifier is
 *   left out if the frame or its client is not known.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void f_log(const struct spoe_frame *frame, const char *format, ...)
{
	va_list ap;
	char    fmt[BUFSIZ];
	double  runtime;

	runtime = LOG_RUNTIME(time_elapsed(&(prg.start_time)), time_elapsed(NULL));

	if (_NULL(frame) || _NULL(FC_PTR))
		(void)snprintf(fmt, sizeof(fmt), LOG_FMT LOG_FMT_INDENT "%s\n",
		               thread_id(), runtime, LOG_INDENT format);
	else
		(void)snprintf(fmt, sizeof(fmt), LOG_FMT LOG_FMT_INDENT "<%lu> %s\n",
		               STRUCT_ELEM(FW_PTR, id, thread_id()),
		               runtime, LOG_INDENT FC_PTR->id, format);

	va_start(ap, format);
	(void)vfprintf(stdout, fmt, ap);
	va_end(ap);
}


/***
 * NAME
 *   w_log - write a worker log message
 *
 * ARGUMENTS
 *   worker - worker for which the message is written
 *   format - printf(3) style format of the message
 *
 * DESCRIPTION
 *   Write the message <format> to the standard output.  The message is prefixed
 *   with the identifier of the worker <worker> and the program runtime.  The
 *   identifier of the calling thread is used if <worker> is a NULL pointer.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void w_log(const struct worker *worker, const char *format, ...)
{
	va_list ap;
	char    fmt[BUFSIZ];

	(void)snprintf(fmt, sizeof(fmt), LOG_FMT LOG_FMT_INDENT "%s\n",
	               STRUCT_ELEM(worker, id, thread_id()),
		       LOG_RUNTIME(time_elapsed(&(prg.start_time)), time_elapsed(NULL)),
	               LOG_INDENT format);

	va_start(ap, format);
	(void)vfprintf(stdout, fmt, ap);
	va_end(ap);
}


/***
 * NAME
 *   socket_set_nonblocking - set the non-blocking mode of a socket
 *
 * ARGUMENTS
 *   fd - file descriptor that is set
 *
 * DESCRIPTION
 *   Add the flag O_NONBLOCK to the flags of the file descriptor <fd>, so that
 *   the operations on it do not block.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) on success, FUNC_RET_ERROR (-1) otherwise.
 */
int socket_set_nonblocking(int fd)
{
	int flags, retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "%d", fd);

	flags = fcntl(fd, F_GETFL, NULL);
	if (_ERROR(flags))
		w_log(NULL, _E("Failed to get file descriptor %d flags: %m"), fd);
	else if (_ERROR(fcntl(fd, F_SETFL, flags | O_NONBLOCK)))
		w_log(NULL, _E("Failed to set non-blocking file descriptor %d: %m"), fd);
	else
		retval = FUNC_RET_OK;

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   socket_set_keepalive - set the keepalive of a socket
 *
 * ARGUMENTS
 *   socket_fd - socket file descriptor
 *   alive     - whether the keepalive is enabled on the socket
 *   idle      - idle time in seconds before the first keepalive probe
 *   intvl     - time in seconds between two keepalive probes
 *   cnt       - number of the probes before the connection is dropped
 *
 * DESCRIPTION
 *   Set the keepalive of the socket <socket_fd>, so that the idle connections
 *   are checked and dropped when the other side stops to answer.  A negative
 *   value of <idle>, <intvl> or <cnt> leaves the related setting unchanged, and
 *   the same is done for a value of <alive> that is neither 0 nor 1.  Not all
 *   of these settings can be changed on every operating system.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) if all the settings are applied, FUNC_RET_ERROR
 *   (-1) otherwise.
 */
int socket_set_keepalive(int socket_fd, int alive, int idle, int intvl, int cnt)
{
	int retval = FUNC_RET_OK;

	DBG_FUNC(NULL, "%d, %d, %d, %d, %d", socket_fd, alive, idle, intvl, cnt);

#ifdef __linux__
	if (idle >= 0)
		retval = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));

	if (_nERROR(retval) && (intvl >= 0))
		retval = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));

	if (_nERROR(retval) && (cnt >= 0))
		retval = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));

#elif defined(__sun)

#  ifdef TCP_KEEPALIVE_THRESHOLD
	if (idle >= 0)
	{
		int probe = idle * 1000;

		/* The initial probe interval in milliseconds. */
		retval = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPALIVE_THRESHOLD, &probe, sizeof(probe));
	}
#  endif

#  ifdef TCP_KEEPALIVE_ABORT_THRESHOLD
	if (_nERROR(retval) && (intvl >= 0) && (cnt >= 0))
	{
		int abort_th = intvl * 1000 * cnt;

		/* The abort interval in milliseconds. */
		retval = setsockopt(socket_fd, IPPROTO_TCP, TCP_KEEPALIVE_ABORT_THRESHOLD, &abort_th, sizeof(abort_th));
	}
#  endif
#endif /* __linux__ || __sun */

	if (_nERROR(retval) && IN_RANGE(alive, 0, 1))
		retval = setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof(alive));

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   rlimit_setnofile - raise the limit of the open files
 *
 * ARGUMENTS
 *   This function takes no arguments.
 *
 * DESCRIPTION
 *   Raise the soft RLIMIT_NOFILE limit, the number of the file descriptors that
 *   the process may have open, to the hard limit, so that the program can serve
 *   as many connections as the system allows.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) on success, FUNC_RET_ERROR (-1) otherwise.
 */
int rlimit_setnofile(void)
{
	struct rlimit nofile;
	int           retval;

	DBG_FUNC(NULL, "");

	if (_ERROR(retval = getrlimit(RLIMIT_NOFILE, &nofile)))
		DBG_RETURN_INT(retval);

	nofile.rlim_cur = nofile.rlim_max;

	W_DBG(UTIL, NULL, "setting RLIMIT_NOFILE to %lu", nofile.rlim_cur);

	retval = setrlimit(RLIMIT_NOFILE, &nofile);

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   logfile_mark - write a marker line to the log
 *
 * ARGUMENTS
 *   msg - message that is written in the marker line
 *
 * DESCRIPTION
 *   Write a marker line with the message <msg> and the current local time to
 *   the log, showing where the program run starts and where it ends.
 *
 * RETURN VALUE
 *   This function does not return a value.
 */
void logfile_mark(const char *msg)
{
	struct tm res;
	time_t    time_s;
	char      tmbuf[32];

	(void)time(&time_s);
	(void)strftime (tmbuf, sizeof (tmbuf), "%F %T", localtime_r (&time_s, &res));
	w_log(NULL, "--- %s --- %s -------------------------", msg, tmbuf);
}


/***
 * NAME
 *   logfile - open the log file
 *
 * ARGUMENTS
 *   filename - name of the log file, with the optional mode prefix
 *
 * DESCRIPTION
 *   Open the log file <filename> and redirect both the standard output and the
 *   standard error output to it.  The name may start with the mode prefix 'a:'
 *   or 'w:', which selects whether the data is appended to the file or the file
 *   is truncated; the same prefix written in the upper case additionally sets
 *   the line buffered output.  A file whose name has no prefix must not exist.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) on success, FUNC_RET_ERROR (-1) otherwise.
 */
int logfile(const char *filename)
{
	bool_t flag_iolbf = 0;
	int    fd, flags = O_WRONLY | O_CREAT, retval = FUNC_RET_ERROR;

	DBG_FUNC(NULL, "\"%s\"", filename);

	if (filename[1] == ':') {
		if (TEST_OR2(filename[0], 'a', 'A')) {
			flags      |= O_APPEND;
			flag_iolbf  = filename[0] == 'A';
		}
		else if (TEST_OR2(filename[0], 'w', 'W')) {
			flags      |= O_TRUNC;
			flag_iolbf  = filename[0] == 'W';
		}
		else {
			(void)fprintf(stderr, "ERROR: invalid logfile mode '%c'\n", filename[0]);

			DBG_RETURN_INT(retval);
		}

		filename += 2;
	} else {
		flags |= O_EXCL;
	}

	fd = open(filename, flags, 0644);
	if (_ERROR(fd))
		(void)fprintf(stderr, "ERROR: unable to open logfile: %m\n");
	else if (_ERROR (dup2 (fd, STDOUT_FILENO)))
		(void)fprintf(stderr, "ERROR: unable to redirect stdout to logfile: %m\n");
	else if (_ERROR (dup2 (fd, STDERR_FILENO)))
		(void)fprintf(stderr, "ERROR: unable to redirect stderr to logfile: %m\n");
	else
		retval = FUNC_RET_OK;

	FD_CLOSE(fd);

	if (_nERROR(retval)) {
		static char linebuf[2][BUFSIZ];

		if (flag_iolbf && _ERROR(setvbuf(stdout, linebuf[0], _IOLBF, sizeof(linebuf[0]))))
			(void)fprintf(stderr, "WARNING: unable to set stdout buffering mode: %m\n");
		if (flag_iolbf && _ERROR(setvbuf(stderr, linebuf[1], _IOLBF, sizeof(linebuf[1]))))
			(void)fprintf(stderr, "WARNING: unable to set stderr buffering mode: %m\n");

		logfile_mark("start");
	}

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   pidfile - handle the pid file
 *
 * ARGUMENTS
 *   filename - name of the pid file, or NULL to write the process identifier
 *   fd       - pointer to the file descriptor of the pid file
 *
 * DESCRIPTION
 *   Handle the pid file, in the way that depends on the arguments.  If the name
 *   <filename> is given and the descriptor <*fd> is negative, then the file is
 *   created, it must not exist before, and its descriptor is saved in <*fd>.
 *   If <filename> is a NULL pointer, the identifier of the process is written
 *   to the already open file, and if both the name and an open descriptor are
 *   given, the file is closed and removed.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) on success, FUNC_RET_ERROR (-1) otherwise.
 */
int pidfile(const char *filename, int *fd)
{
	char buffer[16];
	int  n, retval = FUNC_RET_OK;

	DBG_FUNC(NULL, "\"%s\", %p", filename, fd);

	if (_NULL(filename)) {
		if (*fd < 0)
			DBG_RETURN_INT(retval);

		n = snprintf(buffer, sizeof(buffer), "%"PRI_PIDT"\n", getpid());
		if (IN_RANGE(n, 0, (int)SIZEOF_N(buffer, 1))) {
			retval = _ERROR(write(*fd, buffer, n)) ? FUNC_RET_ERROR : FUNC_RET_OK;
			if (_ERROR(retval))
				(void)fprintf(stderr, "ERROR: unable to write PID to pidfile: %m\n");
		} else {
			(void)fprintf(stderr, "ERROR: unable to write PID to pidfile: buffer too small\n");

			retval = FUNC_RET_ERROR;
		}
	}
	else if (*fd >= 0) {
		FD_CLOSE(*fd);

		if (_ERROR(unlink(filename)))
			retval = FUNC_RET_ERROR;
	}
	else {
		*fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0644);
		if (_ERROR(*fd)) {
			(void)fprintf(stderr, "ERROR: unable to create pidfile: %m\n");

			retval = FUNC_RET_ERROR;
		}
	}

	DBG_RETURN_INT(retval);
}


/***
 * NAME
 *   daemonize - move the program to the background
 *
 * ARGUMENTS
 *   flag_chdir   - whether the working directory is changed to the root one
 *   flag_fdclose - whether the standard descriptors are redirected
 *   fd           - array of the file descriptors that are left open
 *   n            - number of the elements of the array <fd>
 *
 * DESCRIPTION
 *   Move the program to the background.  The process forks twice and becomes
 *   the leader of a new session, detached from the controlling terminal; both
 *   parents exit, so the function returns in the second child only.  The file
 *   mode mask is cleared and, if the flag <flag_chdir> is set, the working
 *   directory is changed to the root directory.  All open file descriptors are
 *   then closed, except the standard ones and those listed in the array <fd>.
 *   If the flag <flag_fdclose> is set, the standard input, the standard output
 *   and the standard error output are redirected to /dev/null.
 *
 * RETURN VALUE
 *   It returns FUNC_RET_OK (0) on success, FUNC_RET_ERROR (-1) otherwise.
 */
int daemonize(bool_t flag_chdir, bool_t flag_fdclose, int *fd, size_t n)
{
#ifdef DEBUG
	pid_t  pid[3];
#endif
	pid_t  pidf;
	size_t j;
	int    i;

	DBG_FUNC(NULL, "%hhu, %hhu, %p, %zu", flag_chdir, flag_fdclose, fd, n);

#ifdef DEBUG
	pid[0] = getpid();
#endif

	/* Fork, parent terminates, child-1 continues. */
	if (_ERROR(pidf = fork()))
		DBG_RETURN_INT(FUNC_RET_ERROR);
	else if (pidf > 0)
		_exit(0);

	/* Become session leader. */
	if (_ERROR(setsid()))
		DBG_RETURN_INT(FUNC_RET_ERROR);

#ifdef DEBUG
	pid[1] = getpid();
#endif

	/* Ignore signals and fork again, child-1 terminates, child-2 continues. */
	(void)signal(SIGCHLD, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	if (_ERROR(pidf = fork()))
		DBG_RETURN_INT(FUNC_RET_ERROR);
	else if (pidf > 0)
		_exit(0);

#ifdef DEBUG
	pid[2] = getpid();
#endif

	(void)umask(0);

	/* Change working directory. */
	if (flag_chdir && _ERROR(chdir("/")))
		DBG_RETURN_INT(FUNC_RET_ERROR);

	/*
	 * Close off file descriptors; except stdin, stdout, stderr and those
	 * listed in the array fd[] (if it's not a NULL pointer).
	 */
	for (i = sysconf(_SC_OPEN_MAX) - 1; i >= 0; i--) {
		if (TEST_OR3(i, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO))
			continue;
		else if (_nNULL(fd) && (n > 0)) {
			for (j = 0; (j < n) && (fd[j] != i); j++);
			if (j < n) {
				W_DBG(UTIL, NULL, "fd %d skipped", i);

				continue;
			}
		}

		if (_OK(close(i)))
			W_DBG(UTIL, NULL, "fd %d closed", i);
	}

	if (flag_fdclose) {
		/* Close off stdin, stdout and stderr. */
		if (_ERROR(close(STDIN_FILENO)))
			W_DBG(UTIL, NULL, "cannot close fd %d: %m", STDIN_FILENO);
		if (_ERROR(close(STDERR_FILENO)))
			W_DBG(UTIL, NULL, "cannot close fd %d: %m", STDERR_FILENO);
		if (_ERROR(close(STDOUT_FILENO)))
			W_DBG(UTIL, NULL, "cannot close fd %d: %m", STDOUT_FILENO);

		/* Redirect stdin, stdout and stderr to /dev/null. */
		(void)open("/dev/null", O_RDONLY);
		(void)open("/dev/null", O_RDWR);
		(void)open("/dev/null", O_RDWR);
	} else {
		W_DBG(UTIL, NULL, "pids: %"PRI_PIDT" -> %"PRI_PIDT" -> %"PRI_PIDT, VAL_ARRAY3(pid));
	}

	DBG_RETURN_INT(FUNC_RET_OK);
}

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
