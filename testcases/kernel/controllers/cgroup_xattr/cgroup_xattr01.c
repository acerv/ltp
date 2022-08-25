// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013-2015 Oracle and/or its affiliates.
 *                         Alexey Kodanev <alexey.kodanev@oracle.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test checks if it is possible to set extended attributes to
 * cgroup files.
 */

#include "tst_test.h"

static struct tst_cg_group *cg_ltptest;

static void run(void)
{
	ssize_t size;
	char value[4];

	cg_ltptest = tst_cg_group_mk(tst_cg, "xattr");

	SAFE_CG_SETXATTR(cg_ltptest, "memory.stat", "test", "test", 4, 0);
	size = SAFE_CG_GETXATTR(cg_ltptest, "memory.stat", "test", value, 4);

	TST_EXP_EQ_SSZ(size, 4);
	TST_EXP_PASS(strcmp(value, "test"));

	cg_ltptest = tst_cg_group_rm(cg_ltptest);
}

static struct tst_test test = {
	.test_all = run,
	.min_kver = "3.7",
	.needs_root = 1,
	.needs_cgroup_ctrls = (const char *const []){ "memory", NULL },
};
