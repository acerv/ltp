// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd., 2015
 * Copyright (c) Linux Test Project, 2015-2026
 */

/*\
 * Verify that :manpage:`sched_setattr(2)` succeeds with valid parameters
 * and fails with the correct errno for invalid inputs.
 *
 * - ESRCH when pid does not exist
 * - EINVAL when attr address is NULL
 * - EINVAL when flags value is invalid
 */

#define _GNU_SOURCE

#include <errno.h>
#include "tst_test.h"
#include "lapi/sched.h"

#define RUNTIME_VAL 10000000
#define PERIOD_VAL 30000000
#define DEADLINE_VAL 30000000

static pid_t pid;
static pid_t unused_pid;

static struct sched_attr attr = {
	.size = sizeof(struct sched_attr),
	.sched_flags = 0,
	.sched_nice = 0,
	.sched_priority = 0,
	.sched_policy = SCHED_DEADLINE,
	.sched_runtime = RUNTIME_VAL,
	.sched_period = PERIOD_VAL,
	.sched_deadline = DEADLINE_VAL,
};

static struct tcase {
	pid_t *pid;
	struct sched_attr *a;
	unsigned int flags;
	int exp_errno;
} tcases[] = {
	{&pid, &attr, 0, 0},
	{&unused_pid, &attr, 0, ESRCH},
	{&pid, NULL, 0, EINVAL},
	{&pid, &attr, 1000, EINVAL},
};

static void verify_sched_setattr(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	if (tc->exp_errno) {
		TST_EXP_FAIL(sched_setattr(*tc->pid, tc->a, tc->flags),
				tc->exp_errno,
				"sched_setattr(%d, %s, %u)",
				*tc->pid,
				tc->a ? "attr" : "NULL",
				tc->flags);
	} else {
		TST_EXP_PASS(sched_setattr(*tc->pid, tc->a, tc->flags),
				"sched_setattr(%d, attr, %u)",
				*tc->pid, tc->flags);
	}
}

static void setup(void)
{
	unused_pid = tst_get_unused_pid();
}

static struct tst_test test = {
	.setup = setup,
	.test = verify_sched_setattr,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_root = 1,
};
