// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that process_mrelease() syscall is raising errors:
 * * EBADF when a bad file descriptor is given
 * * EINVAL when flags is not zero
 * * EINVAL when memory of a task cannot be released because it's still running
 */

#include "tst_test.h"
#include "lapi/syscalls.h"

static int badfd = -1;
static int pidfd;

static struct tcase {
	int needs_child;
	int *fd;
	int flags;
	int exp_errno;
	char *msg;
} tcases[] = {
	{0, &badfd,  0, EBADF,  "bad file descriptor"},
	{1, &pidfd, -1, EINVAL, "flags is not 0"},
	{1, &pidfd,  0, EINVAL,  "memory of running task cannot be released"},
};

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	if (tc->needs_child) {
		pid_t pid;

		pid = SAFE_FORK();
		if (!pid) {
			tst_res(TINFO, "Keep child alive");

			TST_CHECKPOINT_WAKE_AND_WAIT(0);
			exit(0);
		}

		TST_CHECKPOINT_WAIT(0);

		pidfd = SAFE_PIDFD_OPEN(pid, 0);
	}

	TST_EXP_FAIL(tst_syscall(__NR_process_mrelease, *tc->fd, tc->flags),
		tc->exp_errno,
		"%s", tc->msg);

	if (tc->needs_child) {
		SAFE_CLOSE(pidfd);

		TST_CHECKPOINT_WAKE(0);
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_root = 1,
	.forks_child = 1,
	.min_kver = "5.15",
	.needs_checkpoints = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_MMU=y",
		NULL,
	},
};
