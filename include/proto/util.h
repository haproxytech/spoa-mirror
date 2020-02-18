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
#ifndef _PROTO_UTIL_H
#define _PROTO_UTIL_H

void buffer_init(struct buffer *data);
void buffer_free(struct buffer *data);
struct buffer *buffer_alloc(size_t size, const void *src, ...);
void buffer_ptr_free(struct buffer **data);
ssize_t buffer_grow(struct buffer *data, const void *src, size_t n);
ssize_t buffer_grow_kv(struct buffer *data, const void *src, size_t n, ...);
const char *str_hex(const void *data, size_t size);
const char *str_ctrl(const void *data, size_t size);
void *mem_dup(const void *s, size_t size);
bool_t str_toull(const char *str, char **endptr, bool_t flag_end, int base, uint64_t *value, uint64_t val_min, uint64_t val_max);
bool_t str_toll(const char *str, char **endptr, bool_t flag_end, int base, int64_t *value, int64_t val_min, int64_t val_max);
const char *str_delay(uint64_t delay_us);
int getopt_shortopts(const struct option *longopts, char *shortopts, size_t size, uint8_t flags);
uint64_t parse_delay_us(const char *delay, uint64_t val_min, uint64_t val_max);
int parse_hostname(const char *hostname);
char *parse_url(const char *url);
uint64_t time_elapsed(const struct timeval *tv);
void c_log(const struct client *client, const char *format, ...)
	__fmt(printf, 2, 3);
void f_log(const struct spoe_frame *frame, const char *format, ...)
	__fmt(printf, 2, 3);
void w_log(const struct worker *worker, const char *format, ...)
	__fmt(printf, 2, 3);
int socket_set_nonblocking(int fd);
int socket_set_keepalive (int socket_fd, int alive, int idle, int intvl, int cnt);
int rlimit_setnofile (void);
void logfile_mark(const char *msg);
int logfile(const char *filename);
int pidfile(const char *filename, int *fd);
int daemonize(bool_t flag_chdir, bool_t flag_fdclose, int *fd, size_t n);

#endif /* _PROTO_UTIL_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
