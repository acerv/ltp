/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef LISTNS_H
#define LISTNS_H

#define _GNU_SOURCE

#include "tst_test.h"
#include "lapi/namespaces.h"

static inline ssize_t listns(uint64_t ns_id, uint32_t ns_type,
			     uint64_t list[], size_t num, unsigned int flags)
{
	ns_id_req req = {
		.size = NS_ID_REQ_SIZE_VER0,
		.ns_id = ns_id,
		.ns_type = ns_type,
	};

	return tst_syscall(__NR_listns, &req, list, num, flags);
}

#endif
