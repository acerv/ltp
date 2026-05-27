// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Copyright (c) 2012 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Verify that :manpage:`exit(2)` returns the correct status to the
 * waiting parent. The child exits with a known value and the parent
 * checks that wait() reports the expected exit code and no signal.
 */

#include <stdlib.h>
#include <sys/wait.h>
#include "tst_test.h"

static void run(void)
{
	int status;
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid)
		exit(1);

	SAFE_WAITPID(pid, &status, 0);

	if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
		tst_res(TPASS, "exit(1) returned correct status to parent");
	} else {
		tst_res(TFAIL, "unexpected wait status: %s",
			tst_strstatus(status));
	}
}

static struct tst_test test = {
	.forks_child = 1,
	.test_all = run,
};
