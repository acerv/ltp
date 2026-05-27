// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Bull S.A. 2001
 * Copyright (c) International Business Machines Corp., 2001
 *     06/2002 Ported by Jacky Malcles
 */

/*\
 * Verify that :manpage:`fcntl(2)` with F_SETLK fails with EAGAIN when the
 * operation is prohibited by locks held by other processes.
 *
 * Parent acquires a write lock on the whole file, then the child tries
 * to acquire its own write lock via F_SETLK and expects EAGAIN.
 */

#include <fcntl.h>
#include "tst_test.h"

static int fd = -1;

static void run(void)
{
	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0,
		.l_len = 0,
	};
	pid_t pid;

	fd = SAFE_CREAT("regfile", 0777);

	if (fcntl(fd, F_SETLK, &fl) < 0)
		tst_brk(TBROK | TERRNO, "fcntl(F_SETLK) parent lock failed");

	pid = SAFE_FORK();
	if (!pid) {
		TST_EXP_FAIL(fcntl(fd, F_SETLK, &fl), EAGAIN,
				"fcntl(F_SETLK) child lock");
		exit(0);
	}

	SAFE_WAITPID(pid, NULL, 0);
	SAFE_CLOSE(fd);
}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.forks_child = 1,
};
