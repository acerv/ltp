/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef LAPI_NAMESPACES_H
#define LAPI_NAMESPACES_H

#include <stdint.h>
#include "lapi/syscalls.h"

#ifndef NS_ID_REQ_SIZE_VER0
# define NS_ID_REQ_SIZE_VER0 32
#endif

#ifndef LISTNS_CURRENT_USER
# define LISTNS_CURRENT_USER 0xffffffffffffffff
#endif

#ifndef MNT_NS
# define MNT_NS (1ULL << 17)
#endif

#ifndef UTS_NS
# define UTS_NS (1ULL << 26)
#endif

#ifndef IPC_NS
# define IPC_NS (1ULL << 27)
#endif

#ifndef USER_NS
# define USER_NS (1ULL << 28)
#endif

#ifndef PID_NS
# define PID_NS (1ULL << 29)
#endif

#ifndef NET_NS
# define NET_NS (1ULL << 30)
#endif

struct ns_id_req_fallback {
	uint32_t size;
	uint32_t spare;
	uint64_t ns_id;
	uint32_t ns_type;
	uint32_t spare2;
	uint64_t user_ns_id;
};

#ifndef HAVE_STRUCT_NS_ID_REQ
typedef struct ns_id_req_fallback ns_id_req;
#else
typedef struct ns_id_req ns_id_req;
#endif

#endif /* LAPI_NAMESPACES_H */
