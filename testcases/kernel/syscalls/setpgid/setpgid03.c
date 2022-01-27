// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (c) 2013 Oracle and/or its affiliates. All Rights Reserved.
 * Copyright (c) 2014 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test to check the error and trivial conditions in setpgid system call
 *
 * EPERM   -  The calling process, process specified by pid and the target
 *            process group must be in the same session.
 *
 * EACCESS -  Proccess cannot change process group ID of a child after child
 *            has performed exec()
 */

#include <unistd.h>
#include <sys/wait.h>
#include "tst_test.h"

#define TEST_APP "setpgid03_child"

static void do_child(void)
{
	SAFE_SETSID();
	TST_CHECKPOINT_WAKE_AND_WAIT(0);
}

static void run(void)
{
	pid_t child_pid;
	int status;

	child_pid = SAFE_FORK();
	if (!child_pid) {
		do_child();
		return;
	}

	TST_CHECKPOINT_WAIT(0);

	TEST(setpgid(child_pid, getppid()));
	if (TST_RET == -1 && TST_ERR == EPERM)
		tst_res(TPASS, "setpgid failed with EPERM");
	else
		tst_res(TFAIL, "retval %ld, errno %s, expected EPERM", TST_RET,
			tst_strerrno(TST_ERR));

	TST_CHECKPOINT_WAKE(0);

	if (wait(&status) < 0)
		tst_res(TFAIL, "wait() for child 1 failed");

	if (!(WIFEXITED(status)) || (WEXITSTATUS(status) != 0))
		tst_res(TFAIL, "child 1 failed with status %d",
			WEXITSTATUS(status));

	/* child after exec() we are no longer allowed to set pgid */
	child_pid = SAFE_FORK();
	if (!child_pid)
		SAFE_EXECLP(TEST_APP, TEST_APP, NULL);

	TST_CHECKPOINT_WAIT(0);

	TEST(setpgid(child_pid, getppid()));
	if (TST_RET == -1 && TST_ERR == EACCES)
		tst_res(TPASS, "setpgid failed with EACCES");
	else
		tst_res(TFAIL, "retval %ld, errno %s, expected EACCES", TST_RET,
			tst_strerrno(TST_ERR));

	TST_CHECKPOINT_WAKE(0);

	if (wait(&status) < 0)
		tst_res(TFAIL, "wait() for child 2 failed");

	if (!(WIFEXITED(status)) || (WEXITSTATUS(status) != 0))
		tst_res(TFAIL, "child 2 failed with status %d",
			WEXITSTATUS(status));
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
	.needs_checkpoints = 1,
};
