/*
 * include/common/spoe.h
 * Macros, variables and structures for the SPOE filter.
 *
 * Copyright (C) 2017 HAProxy Technologies, Christopher Faulet <cfaulet@haproxy.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, version 2.1
 * exclusively.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _SPOE_TYPES_H
#define _SPOE_TYPES_H

/* Flags set on the SPOE frame */
#define SPOE_FRM_FL_FIN         0x00000001
#define SPOE_FRM_FL_ABRT        0x00000002

/* All supported SPOE actions */
enum spoe_action_type {
	SPOE_ACT_T_SET_VAR = 1,
	SPOE_ACT_T_UNSET_VAR,
	SPOE_ACT_TYPES,
};

/* Errors triggerd by SPOE applet */
#define SPOE_FRM_ERR_DEFINES   \
	SPOE_FRM_ERR_DEF(=  0, NONE,               "normal") \
	SPOE_FRM_ERR_DEF(    , IO,                 "I/O error") \
	SPOE_FRM_ERR_DEF(    , TOUT,               "a timeout occurred") \
	SPOE_FRM_ERR_DEF(    , TOO_BIG,            "frame is too big") \
	SPOE_FRM_ERR_DEF(    , INVALID,            "invalid frame received") \
	SPOE_FRM_ERR_DEF(    , NO_VSN,             "version value not found") \
	SPOE_FRM_ERR_DEF(    , NO_FRAME_SIZE,      "max-frame-size value not found") \
	SPOE_FRM_ERR_DEF(    , NO_CAP,             "capabilities value not found") \
	SPOE_FRM_ERR_DEF(    , BAD_VSN,            "unsupported version") \
	SPOE_FRM_ERR_DEF(    , BAD_FRAME_SIZE,     "max-frame-size too big or too small") \
	SPOE_FRM_ERR_DEF(    , FRAG_NOT_SUPPORTED, "fragmentation not supported") \
	SPOE_FRM_ERR_DEF(    , INTERLACED_FRAMES,  "invalid interlaced frames") \
	SPOE_FRM_ERR_DEF(    , FRAMEID_NOTFOUND,   "frame-id not found") \
	SPOE_FRM_ERR_DEF(    , RES,                "resource allocation error") \
	SPOE_FRM_ERR_DEF(= 99, UNKNOWN,            "an unknown error occurred")

#define SPOE_FRM_ERR_DEF(v,e,s)   SPOE_FRM_ERR_##e v,
enum spoe_frame_error {
	SPOE_FRM_ERR_DEFINES
	SPOE_FRM_ERRS,
};
#undef SPOE_FRM_ERR_DEF

/* Scopes used for variables set by agents. It is a way to be agnotic to vars
 * scope. */
enum spoe_vars_scope {
	SPOE_SCOPE_PROC = 0, /* <=> SCOPE_PROC  */
	SPOE_SCOPE_SESS,     /* <=> SCOPE_SESS */
	SPOE_SCOPE_TXN,      /* <=> SCOPE_TXN  */
	SPOE_SCOPE_REQ,      /* <=> SCOPE_REQ  */
	SPOE_SCOPE_RES,      /* <=> SCOPE_RES  */
};


enum spoe_frame_type {
	SPOE_FRM_T_UNSET = 0,

	/* Frames sent by HAProxy */
	SPOE_FRM_T_HAPROXY_HELLO = 1,
	SPOE_FRM_T_HAPROXY_DISCON,
	SPOE_FRM_T_HAPROXY_NOTIFY,

	/* Frames sent by the agents */
	SPOE_FRM_T_AGENT_HELLO = 101,
	SPOE_FRM_T_AGENT_DISCON,
	SPOE_FRM_T_AGENT_ACK
};

/* All supported data types */
enum spoe_data_type {
	SPOE_DATA_T_NULL = 0,
	SPOE_DATA_T_BOOL,
	SPOE_DATA_T_INT32,
	SPOE_DATA_T_UINT32,
	SPOE_DATA_T_INT64,
	SPOE_DATA_T_UINT64,
	SPOE_DATA_T_IPV4,
	SPOE_DATA_T_IPV6,
	SPOE_DATA_T_STR,
	SPOE_DATA_T_BIN,
	SPOE_DATA_TYPES
};

/* a memory block of arbitrary size, or a string */
struct chunk {
	char   *ptr;
	size_t  len;
};

/* all data types that may be encoded/decoded for each spoe_data_type */
union spoe_data {
	bool            boolean;
	int32_t         int32;
	uint32_t        uint32;
	int64_t         int64;
	uint64_t        uint64;
	struct in_addr  ipv4;
	struct in6_addr ipv6;
	struct chunk    chk;     /* types STR and BIN */
};

/* Masks to get data type or flags value */
#define SPOE_DATA_T_MASK  0x0F
#define SPOE_DATA_FL_MASK 0xF0

/* Flags to set Boolean values */
#define SPOE_DATA_FL_FALSE 0x00
#define SPOE_DATA_FL_TRUE  0x10

#endif /* _SPOE_TYPES_H */
