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
#ifndef _COMMON_DEBUG_H
#define _COMMON_DEBUG_H

#define CHECK_ARG(a,f)           do { if (a) { f; } } while (0)
#define CHECK_ARG_ERRNO(a,e,f)   do { if (a) { errno = (e); f; } } while (0)
#define CHECK_ARG_NULL(a,f)      CHECK_ARG_ERRNO(_NULL(a), EFAULT, f)
#define CHECK_ARG_PATH(a,f)      do { if (_NULL(a)) { errno = EFAULT; f; } else if (*(a) == '\0') { errno = ENOENT; f; } } while (0)

#ifdef DEBUG
enum DBG_LEVEL_enum {
	DBG_LEVEL_FUNC = 0, /* Function debug level. */
	DBG_LEVEL_LOG,      /* Generic debug level 0. */
	DBG_LEVEL_NOTICE,   /* Generic debug level 1. */
	DBG_LEVEL_INFO,     /* Generic debug level 2. */
	DBG_LEVEL_DEBUG,    /* Generic debug level 3. */
	DBG_LEVEL_UTIL,     /* Util debug level. */
	DBG_LEVEL_WORKER,   /* Worker debug level. */
	DBG_LEVEL_SPOA,     /* SPOA debug level. */
	DBG_LEVEL_CURL,     /* cURL debug level. */
	DBG_LEVEL_ENABLED,  /* This have to be the last entry. */
};

#  define IFDEF_DBG(a, b)        a
#  define DBG_INDENT_STEP        2
#  define DBG_PARM(a, ...)       a, ##__VA_ARGS__
#  define C_DBG(l,C,f, ...)                                 \
	do {                                                \
		if (cfg.debug_level & (1 << DBG_LEVEL_##l)) \
			c_log((C), f, ##__VA_ARGS__);       \
	} while (0)
#  define F_DBG(l,F,f, ...)                                 \
	do {                                                \
		if (cfg.debug_level & (1 << DBG_LEVEL_##l)) \
			f_log((F), f, ##__VA_ARGS__);       \
	} while (0)
#  define W_DBG(l,W,f, ...)                                 \
	do {                                                \
		if (cfg.debug_level & (1 << DBG_LEVEL_##l)) \
			w_log((W), f, ##__VA_ARGS__);       \
	} while (0)
#  define DBG_FUNC(W,f, ...)                                                      \
	do {                                                                      \
		if (cfg.debug_level & (1 << DBG_LEVEL_ENABLED))                   \
			W_DBG(FUNC, (W), "%s(" f ") {", __func__, ##__VA_ARGS__); \
		dbg_w_ptr   = (W);                                                \
		dbg_indent += DBG_INDENT_STEP;                                    \
	} while (0)
#  define DBG_FUNC_END(f, ...)                                    \
	do {                                                      \
		dbg_indent -= DBG_INDENT_STEP;                    \
		if (cfg.debug_level & (1 << DBG_LEVEL_ENABLED))   \
			W_DBG(FUNC, dbg_w_ptr, f, ##__VA_ARGS__); \
	} while (0)
#  define DBG_RETURN()           do { DBG_FUNC_END("}"); return; } while (0)
#  define DBG_RETURN_EX(a,t,f)   do { t _r = (a); DBG_FUNC_END("} = " f, _r); return _r; } while (0)
#  define DBG_RETURN_INT(a)      DBG_RETURN_EX((a), int, "%d")
#  define DBG_RETURN_U64(a)      DBG_RETURN_EX((a), uint64_t, "%lu")
#  define DBG_RETURN_SIZE(a)     DBG_RETURN_EX((a), size_t, "%ld")
#  define DBG_RETURN_SSIZE(a)    DBG_RETURN_EX((a), ssize_t, "%lu")
#  define DBG_RETURN_PTR(a)      DBG_RETURN_EX((a), void *, "%p")
#  define DBG_RETURN_CPTR(a)     DBG_RETURN_EX((a), const void *, "%p")
#else
#  define IFDEF_DBG(a, b)        b
#  define DBG_PARM(...)
#  define C_DBG(...)             while (0)
#  define F_DBG(...)             while (0)
#  define W_DBG(...)             while (0)
#  define DBG_FUNC(...)          while (0)
#  define DBG_FUNC_END(f, ...)   while (0)
#  define DBG_RETURN()           return
#  define DBG_RETURN_EX(a,t,f)   return a
#  define DBG_RETURN_INT(a)      return a
#  define DBG_RETURN_U64(a)      return a
#  define DBG_RETURN_SIZE(a)     return a
#  define DBG_RETURN_SSIZE(a)    return a
#  define DBG_RETURN_PTR(a)      return a
#  define DBG_RETURN_CPTR(a)     return a
#endif /* DEBUG */

#endif /* _COMMON_DEBUG_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
