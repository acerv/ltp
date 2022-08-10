// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd., 2015
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone a process with CLONE_NEWPID flag and reach the maximum amount of
 * nested containers checking for errors.
 */

#include "tst_test.h"
#include "lapi/namespaces_constants.h"

#define MAXNEST 32

static int child_func(void *arg)
{
	pid_t cpid;
	int *level = (int *)arg;
	int status;

	tst_res(TINFO, "%p=%d", level, *level);

	if (*level == MAXNEST)
		return 0;

	(*level)++;

	cpid = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, level);
	if (cpid < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	SAFE_WAITPID(cpid, &status, 0);

	return 0;
}

static void run(void)
{
	int ret, status;
	int level = 1;

	ret = ltp_clone_quick(CLONE_NEWPID | SIGCHLD, child_func, &level);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "clone failed");

	tst_res(TINFO, "%p=%d", &level, level);

	SAFE_WAITPID(ret, &status, 0);

	tst_res(TINFO, "%p=%d", &level, level);

	if (level < MAXNEST) {
		tst_res(TFAIL, "Not enough nested containers: %d", level);
		return;
	}

	tst_res(TPASS, "All containers have been nested");
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
};
