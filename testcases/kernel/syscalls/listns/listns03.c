// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify that listns() pagination works correctly by comparing a full
 * namespace list read in one shot with one read in small groups.
 *
 * [Algorithm]
 *
 * - create multiple mount namespaces via unshare() in child processes
 * - read the full list of namespace IDs using listns() in one call
 * - read the list of namespace IDs using groups of fixed size
 * - compare the first list with the second list
 */

#define _GNU_SOURCE

#include "listns.h"
#include "lapi/sched.h"

#define NUM_NS 8
#define GROUPS_SIZE 3
#define LISTSIZE 512

static void run(void)
{
	ssize_t ret;
	size_t count = 0;
	ssize_t tot_ids;
	uint64_t all_ids[LISTSIZE];
	uint64_t group_ids[LISTSIZE];
	uint64_t last_id;
	int pids[NUM_NS];

	SAFE_UNSHARE(CLONE_NEWNS);

	for (int i = 0; i < NUM_NS; i++) {
		pids[i] = SAFE_FORK();
		if (!pids[i]) {
			SAFE_UNSHARE(CLONE_NEWNS);
			TST_CHECKPOINT_WAIT(0);
			exit(0);
		}
	}

	tst_res(TINFO, "Reading all mount namespace IDs at once");

	tot_ids = listns(0, MNT_NS, all_ids, LISTSIZE, 0);
	if (tot_ids < 0) {
		tst_res(TFAIL | TERRNO, "listns() failed");
		goto end;
	}

	tst_res(TINFO, "listns() returned %zd namespace(s)", tot_ids);

	if (tot_ids == 0) {
		tst_res(TFAIL, "listns() returned 0 namespaces");
		goto end;
	}

	tst_res(TINFO, "Reading mount namespace IDs in groups of %d",
		GROUPS_SIZE);

	while (count < (size_t)tot_ids) {
		last_id = count ? group_ids[count - 1] : 0;
		ret = listns(last_id, MNT_NS, group_ids + count,
			     GROUPS_SIZE, 0);

		tst_res(TDEBUG, "listns(%lu, MNT_NS, list + %lu, %d, 0)",
			last_id, count, GROUPS_SIZE);

		if (ret < 0) {
			tst_res(TFAIL | TERRNO, "listns() failed");
			goto end;
		}

		count += ret;

		if (ret < GROUPS_SIZE)
			break;
	}

	if (count != (size_t)tot_ids) {
		tst_res(TFAIL, "Paginated read returned %lu IDs, expected %zd",
			count, tot_ids);
		goto end;
	}

	for (size_t i = 0; i < count; i++) {
		if (all_ids[i] != group_ids[i]) {
			tst_res(TFAIL, "Namespace ID differs at index %lu", i);
			goto end;
		}
	}

	tst_res(TPASS, "All namespace IDs match between full and paginated reads");

end:
	TST_CHECKPOINT_WAKE2(0, NUM_NS);

	for (int i = 0; i < NUM_NS; i++)
		SAFE_WAITPID(pids[i], NULL, 0);
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
	.needs_root = 1,
	.needs_checkpoints = 1,
	.min_kver = "6.19",
};
