// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test that :manpage:`execve(2)` is blocked under the failfs root.
 *
 * After fchroot() moved the process root into failfs, loading a binary
 * fails because the absolute path lookup, and with a dynamically linked
 * binary also the lookup of the ELF interpreter, fails with EOPNOTSUPP.
 *
 * The exec runs in a grandchild: a wrongly successful exec would replace
 * the test image, so the outcome can only be reported when the exec call
 * returns, and the grandchild exit code tells the parent whether the
 * image was replaced.
 */

#define _GNU_SOURCE
#include <sys/wait.h>
#include <unistd.h>
#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/syscalls.h"

/* Marker exit code proving the grandchild image was not replaced. */
#define EXEC_NOT_REPLACED 42

static void check_exec_blocked(void)
{
	pid_t pid = SAFE_FORK();
	int status;

	if (!pid) {
		TEST(execl("/bin/true", "true", NULL));
		if (TST_ERR == EOPNOTSUPP)
			tst_res(TPASS, "exec blocked by the failfs root");
		else
			tst_res(TFAIL | TTERRNO, "exec failed unexpectedly");
		exit(EXEC_NOT_REPLACED);
	}

	SAFE_WAITPID(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != EXEC_NOT_REPLACED)
		tst_res(TFAIL, "exec replaced the test image");
}

static void run(void)
{
	if (!SAFE_FORK()) {
		TST_EXP_PASS(tst_syscall(__NR_fchroot, FD_FAILFS_ROOT, 0),
			"fchroot() with the FD_FAILFS_ROOT sentinel");

		check_exec_blocked();

		exit(0);
	}
}

static void setup(void)
{
	if (access("/bin/true", X_OK))
		tst_brk(TCONF | TERRNO, "/bin/true is not available");
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_root = 1,
	.forks_child = 1,
};
