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
static pid_t child_einval;
static pid_t child_memrel;
static pid_t child_esrch;

static struct tcase {
	pid_t *child_pid;
	int *fd;
	int flags;
	int exp_errno;
	char *msg;
} tcases[] = {
	{NULL, &badfd, 0, EBADF, "bad file descriptor"},
	{&child_einval, &pidfd, -1, EINVAL, "flags is not 0"},
	{&child_memrel, &pidfd, 0, EINVAL, "task memory cannot be released"},
	{&child_esrch, &pidfd, 0, ESRCH, "child is not running"},
};

static pid_t spawn_child(void)
{
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) 
		exit(0);

	tst_res(TINFO, "Spawned child with pid=%d", pid);

	return pid;
}

static pid_t spawn_waiting_child(void)
{
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) {
		TST_CHECKPOINT_WAIT(0);
		exit(0);
	}

	tst_res(TINFO, "Spawned waiting child with pid=%d", pid);

	return pid;
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	if (tc->child_pid)
		pidfd = SAFE_PIDFD_OPEN(*tc->child_pid, 0);

	TST_EXP_FAIL(tst_syscall(__NR_process_mrelease, *tc->fd, tc->flags),
		tc->exp_errno,
		"%s", tc->msg);

	if (tc->child_pid) {
		if (tc->exp_errno == EINVAL)
			TST_CHECKPOINT_WAKE(0);

		SAFE_CLOSE(pidfd);
	}
}

static void setup(void)
{
	child_einval = spawn_child();
	child_memrel = spawn_child();
	child_esrch = spawn_waiting_child();
}

static struct tst_test test = {
	.test = run,
	.setup = setup,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_root = 1,
	.forks_child = 1,
	.min_kver = "5.15",
	.needs_checkpoints = 1,
};
