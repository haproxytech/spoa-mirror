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
#ifndef _COMMON_DEFINE_H
#define _COMMON_DEFINE_H

#ifdef USE_THREADS
#  define __THR                    __thread
#else
#  define __THR
#endif

#ifndef MIN
#  define MIN(a,b)                 (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#  define MAX(a,b)                 (((a) > (b)) ? (a) : (b))
#endif

#define ONCE(f)                    do { static __THR char __f = 1; if (__f) { __f = 0; f; } } while (0)

#define TABLESIZE_N(a,n)           ((int) (sizeof(a) / sizeof((a)[0])) - (n))
#define TABLESIZE_1(a)             TABLESIZE_N((a), 1)
#define TABLESIZE(a)               TABLESIZE_N((a), 0)

#define STRUCT_ADDR(t,n,e)         struct t *n = (void *)((uint8_t *)ev - offsetof(struct t, e))
#define DPTR_ARGS(a)               (a), _NULL(a) ? NULL : *(a)
#define PTR_SAFE(a,v)              (_NULL(a) ? (v) : (a))
#define PTR_FREE(a)                do { if (_nNULL(a)) { free(a); (a) = NULL; } } while (0)
#define STRUCT_ELEM(s,e,v)         (_NULL(s) ? (v) : (s)->e)
#define STRUCT_ELEM_SAFE(s,e,v)    (TEST_OR2(NULL, (s), (s)->e) ? (v) : (s)->e)
#define FD_CLOSE(a)                do { if ((a) >= 0) { (void)close(a); (a) = -1; } } while (0)

/* warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing] */
#define DEREF_PTR(t,n,m)           t *n = &(m)

#define BUFFER_NEXT(n)             do { __n = (__n + 1) % TABLESIZE(n); } while (0)
#define BUFFER_DEF(t,n,s)          static __THR t (n)[s]; static __THR size_t __n = 0; ONCE((void)memset((n), 0, sizeof(n))); BUFFER_NEXT(n)
#define BUFFER2_DEF(t,n,S,s)       static __THR t (n)[S][s]; static __THR size_t __n = 0; ONCE((void)memset((n), 0, sizeof(n))); BUFFER_NEXT(n)
#define BUFFER(n)                  ((n)[__n])
#define BUFFER_ADDRSIZE(n)         BUFFER(n), sizeof(BUFFER(n))

#define SIZEOF_N(a,n)              (sizeof(a) - (n))
#define SIZEOF_1(a)                SIZEOF_N((a), 1)

#define NIBBLE_TO_HEX(a)           ((a) + (((a) < 10) ? '0' : ('a' - 10)))
#define STR_SAFE(a)                (_NULL(a) ? "(null)" : (a))
#define STR_SIZE(a)                SIZEOF_1(a)
#define STR_ADDRSIZE(a)            (a), STR_SIZE(a)
#define STR_BOOL(a)                ((a) ? "true" : "false")

#define CLAMP_VALUE(v,a,b)         (((v) < (a)) ? (a) : (((v) > (b)) ? (b) : (v)))

#define IN_RANGE(v,a,b)            (((v) >= (a)) && ((v) <= (b)))

#define TEST_OR2(v,a,b)            (((v) == (a))                 || ((v) == (b)))
#define TEST_OR3(v,a,b,c)          (TEST_OR2((v), (a), (b))      || ((v) == (c)))
#define TEST_OR4(v,a,b,c,d)        (TEST_OR3((v), (a), (b), (c)) || ((v) == (d)))
#define TEST_NAND2(v,a,b)          (((v) != (a))                   && ((v) != (b)))
#define TEST_NAND3(v,a,b,c)        (TEST_NAND2((v), (a), (b))      && ((v) != (c)))
#define TEST_NAND4(v,a,b,c,d)      (TEST_NAND3((v), (a), (b), (c)) && ((v) != (d)))

#define TEST_ARRAY2(p,i,a,b)       (((p)[i] == (a))                      && ((p)[(i) + 1] == (b)))
#define TEST_ARRAY3(p,i,a,b,c)     (TEST_ARRAY2((p), (i), (a), (b))      && ((p)[(i) + 2] == (c)))
#define TEST_ARRAY4(p,i,a,b,c,d)   (TEST_ARRAY2((p), (i), (a), (b), (c)) && ((p)[(i) + 3] == (d)))

#define VAL_ARRAY2(p)              (p)[0], (p)[1]
#define VAL_ARRAY3(p)              VAL_ARRAY2(p), (p)[2]
#define VAL_ARRAY4(p)              VAL_ARRAY3(p), (p)[3]
#define VAL_ARRAY5(p)              VAL_ARRAY4(p), (p)[4]
#define VAL_ARRAY6(p)              VAL_ARRAY5(p), (p)[5]
#define VAL_ARRAY7(p)              VAL_ARRAY6(p), (p)[6]
#define VAL_ARRAY8(p)              VAL_ARRAY7(p), (p)[7]

#define TIMEVAL_INIT_S(s)          { (s), 0 }
#define TIMEVAL_INIT_MS(s)         { (s) / 1000, (s) % 1000 }
#define TIMEVAL_INIT_US(s)         { (s) / 1000000, (s) % 1000000 }
#define TIMEVAL_SET_S(t,s)         do { (t)->tv_sec = (s); (t)->tv_usec = 0; } while (0)
#define TIMEVAL_SET_MS(t,s)        do { (t)->tv_sec = (s) / 1000; (t)->tv_usec = (s) % 1000; } while (0)
#define TIMEVAL_SET_US(t,s)        do { (t)->tv_sec = (s) / 1000000; (t)->tv_usec = (s) % 1000000; } while (0)
#define TIMEVAL_S(t)               ((t)->tv_sec + ((t)->tv_usec + 500000) / 1000000)
#define TIMEVAL_MS(t)              ((t)->tv_sec * 1000ULL + ((t)->tv_usec + 500) / 1000)
#define TIMEVAL_US(t)              ((t)->tv_sec * 1000000ULL + (t)->tv_usec)
#define TIMEVAL_DIFF_MS(a,b)       (((a)->tv_sec - (b)->tv_sec) * 1000ULL + ((a)->tv_usec - (b)->tv_usec + 500) / 1000)
#define TIMEVAL_DIFF_US(a,b)       (((a)->tv_sec - (b)->tv_sec) * 1000000ULL + (a)->tv_usec - (b)->tv_usec)

#define STR_STRINGIFY(s)           #s
#define STR_VERSION2(M,m)          STR_STRINGIFY(M) "." STR_STRINGIFY(m)
#define STR_VERSION3(M,m,p)        STR_STRINGIFY(M) "." STR_STRINGIFY(m) "." STR_STRINGIFY(p)

#ifdef __linux__
#  define PRI_PIDT                 "d"
#  define PRI_PTHREADT             "lu"
#else
#  define PRI_PIDT                 "ld"
#  define PRI_PTHREADT             "u"
#endif

#ifdef __GNUC__
#  ifndef __always_inline
#     define __always_inline       __attribute__((always_inline)) inline
#  endif
#  define __fmt(a,b,c)             __attribute__((format(a, b, c)))
#  define __maybe_unused           __attribute__((unused))
#  ifndef __packed
#     define __packed              __attribute__((packed))
#  endif
#  define __noreturn               __attribute__((noreturn))
#else
#  ifndef __always_inline
#     define __always_inline       inline
#  endif
#  define __fmt(...)
#  define __maybe_unused
#  define __packed
#  define __noreturn
#endif

#define _NULL(a)                   ((a) == NULL)
#define _nNULL(a)                  ((a) != NULL)
#define _OK(a)                     ((a) == FUNC_RET_OK)
#define _nOK(a)                    ((a) != FUNC_RET_OK)
#define _ERROR(a)                  ((a) == FUNC_RET_ERROR)
#define _nERROR(a)                 ((a) != FUNC_RET_ERROR)

enum {
	FUNC_RET_ERROR = -1,
	FUNC_RET_OK,
};

#endif /* _COMMON_DEFINE_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vi: noexpandtab shiftwidth=8 tabstop=8
 */
