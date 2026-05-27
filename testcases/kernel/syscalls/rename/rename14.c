// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2002
 * Ported to Linux: nsharoff@us.ibm.com
 * Ported to LTP: robbiew@us.ibm.com
 */

/*\
 * Stress test for :manpage:`rename(2)` under concurrent create/unlink.
 *
 * Two child processes run concurrently: one repeatedly creates and
 * unlinks a file, the other repeatedly renames it. The test passes
 * if no crash or hang occurs during the runtime.
 */

#include <stdlib.h>

#include "tst_test.h"

static void child_create(void)
{
	int fd;

	while (tst_remaining_runtime()) {
		fd = creat("rename14", 0666);
		if (fd != -1)
			close(fd);
		unlink("rename14");
	}
	exit(0);
}

static void child_rename(void)
{
	while (tst_remaining_runtime())
		rename("rename14", "rename14xyz");
	exit(0);
}

static void run(void)
{
	if (!SAFE_FORK())
		child_create();

	if (!SAFE_FORK())
		child_rename();

	sleep(tst_remaining_runtime());

	tst_res(TPASS, "rename under concurrent create/unlink survived");
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
	.needs_tmpdir = 1,
	.runtime = 5,
};
