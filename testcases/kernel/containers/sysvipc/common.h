// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 *               Rishikesh K Rajak <risrajak@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef COMMON_H
#define COMMON_H

#include "libclone.h"

static int dummy_child(void *v)
{
	(void)v;
	return 0;
}

static void check_newipc(void)
{
	int pid, status;

	if (tst_kvercmp(2, 6, 19) < 0)
		tst_brk(TCONF, "CLONE_NEWIPC not supported");

	pid = tst_clone_unshare_test(T_CLONE, CLONE_NEWIPC, dummy_child, NULL);
	if (pid < 0)
		tst_brk(TCONF | TERRNO, "CLONE_NEWIPC not supported");

	SAFE_WAITPID(pid, &status, 0);
}

#endif
