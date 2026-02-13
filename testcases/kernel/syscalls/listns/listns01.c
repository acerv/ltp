// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify that listns() raises the correct errors for invalid input:
 *
 * - EFAULT: req points to inaccessible memory
 * - EFAULT: ns_ids points to inaccessible memory
 * - EINVAL: invalid flags
 * - EINVAL: insufficient ns_id_req.size
 * - EINVAL: non-zero ns_id_req.spare
 * - EINVAL: non-zero ns_id_req.spare2
 * - EINVAL: invalid ns_id_req.ns_type
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "lapi/namespaces.h"
#include "lapi/syscalls.h"

#define LISTSIZE 32

static ns_id_req *request;
static uint64_t ns_ids[LISTSIZE];

static struct tcase {
	int req_usage;
	uint32_t size;
	uint32_t spare;
	uint64_t ns_id;
	uint32_t ns_type;
	uint32_t spare2;
	uint64_t *ns_ids;
	size_t nr_ns_ids;
	unsigned int flags;
	int exp_errno;
	char *msg;
} tcases[] = {
	{
		.req_usage = 0,
		.ns_ids = ns_ids,
		.nr_ns_ids = LISTSIZE,
		.exp_errno = EFAULT,
		.msg = "request points to inaccessible memory",
	},
	{
		.req_usage = 1,
		.size = NS_ID_REQ_SIZE_VER0,
		.ns_ids = NULL,
		.nr_ns_ids = LISTSIZE,
		.exp_errno = EFAULT,
		.msg = "ns_ids points to inaccessible memory",
	},
	{
		.req_usage = 1,
		.size = NS_ID_REQ_SIZE_VER0,
		.ns_ids = ns_ids,
		.nr_ns_ids = LISTSIZE,
		.flags = -1,
		.exp_errno = EINVAL,
		.msg = "invalid flags",
	},
	{
		.req_usage = 1,
		.size = 0,
		.ns_ids = ns_ids,
		.nr_ns_ids = LISTSIZE,
		.exp_errno = EINVAL,
		.msg = "insufficient ns_id_req.size",
	},
	{
		.req_usage = 1,
		.size = NS_ID_REQ_SIZE_VER0,
		.spare = -1,
		.ns_ids = ns_ids,
		.nr_ns_ids = LISTSIZE,
		.exp_errno = EINVAL,
		.msg = "non-zero ns_id_req.spare",
	},
	{
		.req_usage = 1,
		.size = NS_ID_REQ_SIZE_VER0,
		.spare2 = -1,
		.ns_ids = ns_ids,
		.nr_ns_ids = LISTSIZE,
		.exp_errno = EINVAL,
		.msg = "non-zero ns_id_req.spare2",
	},
	{
		.req_usage = 1,
		.size = NS_ID_REQ_SIZE_VER0,
		.ns_type = -1,
		.ns_ids = ns_ids,
		.nr_ns_ids = LISTSIZE,
		.exp_errno = EINVAL,
		.msg = "invalid ns_id_req.ns_type",
	},
};

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];
	ns_id_req *req = NULL;

	memset(ns_ids, 0, sizeof(ns_ids));

	if (tc->req_usage) {
		req = request;
		memset(req, 0, NS_ID_REQ_SIZE_VER0);
		req->size = tc->size;
		req->spare = tc->spare;
		req->ns_id = tc->ns_id;
		req->ns_type = tc->ns_type;
		req->spare2 = tc->spare2;
	}

	TST_EXP_FAIL(tst_syscall(__NR_listns, req, tc->ns_ids,
		tc->nr_ns_ids, tc->flags), tc->exp_errno,
		"%s", tc->msg);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.min_kver = "6.19",
	.bufs = (struct tst_buffers []) {
		{ &request, .size = NS_ID_REQ_SIZE_VER0 },
		{},
	},
};
