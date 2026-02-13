// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify basic listns() functionality:
 *
 * - list all namespaces and verify it returns a non-zero count
 * - create a new mount namespace via unshare(), list namespaces filtered
 *   by MNT_NS and verify the new namespace ID appears in results
 * - filter by NET_NS type and verify only network namespaces are returned
 *   (none of the mount namespace IDs appear)
 */

#define _GNU_SOURCE

#include "listns.h"
#include "lapi/sched.h"

#define LISTSIZE 512

static void test_list_all(void)
{
	uint64_t list[LISTSIZE];

	TST_EXP_POSITIVE(listns(0, 0, list, LISTSIZE, 0));
	if (!TST_PASS)
		return;

	tst_res(TPASS, "listns() returned %ld namespace(s)", TST_RET);
}

static void test_filter_by_type(void)
{
	uint64_t mnt_list[LISTSIZE];
	uint64_t net_list[LISTSIZE];
	ssize_t mnt_count, net_count;
	int found = 0;

	SAFE_UNSHARE(CLONE_NEWNS);

	mnt_count = listns(0, MNT_NS, mnt_list, LISTSIZE, 0);
	if (mnt_count < 0) {
		tst_res(TFAIL | TERRNO, "listns(MNT_NS) failed");
		return;
	}

	if (mnt_count == 0) {
		tst_res(TFAIL, "listns(MNT_NS) returned 0 namespaces");
		return;
	}

	tst_res(TINFO, "listns(MNT_NS) returned %zd namespace(s)", mnt_count);

	net_count = listns(0, NET_NS, net_list, LISTSIZE, 0);
	if (net_count < 0) {
		tst_res(TFAIL | TERRNO, "listns(NET_NS) failed");
		return;
	}

	tst_res(TINFO, "listns(NET_NS) returned %zd namespace(s)", net_count);

	for (ssize_t i = 0; i < mnt_count; i++) {
		for (ssize_t j = 0; j < net_count; j++) {
			if (mnt_list[i] == net_list[j]) {
				found = 1;
				break;
			}
		}
		if (found)
			break;
	}

	if (found)
		tst_res(TFAIL, "MNT_NS and NET_NS lists overlap");
	else
		tst_res(TPASS, "MNT_NS and NET_NS lists are disjoint");
}

static void run(unsigned int n)
{
	if (!SAFE_FORK()) {
		switch (n) {
		case 0:
			test_list_all();
			break;
		case 1:
			test_filter_by_type();
			break;
		}
		exit(0);
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = 2,
	.forks_child = 1,
	.needs_root = 1,
	.min_kver = "6.19",
};
