// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that sched_yield() syscall is properly switching context to an
 * another process which is running with the same priority of the caller.
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "lapi/cpuset.h"

static void setup_sched_policy(void)
{
	cpu_set_t set;
	pid_t pid = getpid();
	struct sched_param sp;

	CPU_ZERO(&set);
	CPU_SET(0, &set);

	TST_EXP_PASS_SILENT(sched_setaffinity(pid, CPU_SETSIZE, &set));

	sp.sched_priority = sched_get_priority_min(SCHED_RR);
	TST_EXP_PASS_SILENT(sched_setscheduler(pid, SCHED_RR, &sp));
}

static void child1(void)
{
	setup_sched_policy();
	TST_CHECKPOINT_WAIT(0);
}

static void child2(pid_t other_child_pid)
{
	setup_sched_policy();
	TST_CHECKPOINT_WAKE(0);

	TST_EXP_PASS(sched_yield());

	TST_EXP_FAIL(kill(other_child_pid, 0), ESRCH, "%s %d",
	      "sched_yield() switched to process", other_child_pid);
}

static void run(void)
{
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) {
		child1();
		exit(0);
	}

	if (!SAFE_FORK()) {
		child2(pid);
		exit(0);
	}
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
	.forks_child = 1,
	.needs_checkpoints = 1,
};

