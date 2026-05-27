// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd., 2015
 * Copyright (c) Linux Test Project, 2015-2026
 */

/*\
 * Verify that :manpage:`sched_getattr(2)` correctly reads back SCHED_DEADLINE
 * scheduling attributes (runtime, period, deadline) that were
 * previously set via :manpage:`sched_setattr(2)`.
 */

#define _GNU_SOURCE

#include <pthread.h>
#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "lapi/sched.h"

#define RUNTIME_VAL 10000000
#define PERIOD_VAL 30000000
#define DEADLINE_VAL 30000000

static void *run_deadline(void *data LTP_ATTRIBUTE_UNUSED)
{
	struct sched_attr attr, attr_copy;
	int ret;
	unsigned int flags = 0;

	attr.size = sizeof(attr);
	attr.sched_flags = 0;
	attr.sched_nice = 0;
	attr.sched_priority = 0;

	/* This creates a 10ms/30ms reservation */
	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_runtime = RUNTIME_VAL;
	attr.sched_period = PERIOD_VAL;
	attr.sched_deadline = DEADLINE_VAL;

	ret = sched_setattr(0, &attr, flags);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "sched_setattr() failed");

	ret = sched_getattr(0, &attr_copy, sizeof(attr_copy), flags);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "sched_getattr() failed");

	TST_EXP_EQ_LU(attr_copy.sched_runtime, RUNTIME_VAL);
	TST_EXP_EQ_LU(attr_copy.sched_period, PERIOD_VAL);
	TST_EXP_EQ_LU(attr_copy.sched_deadline, DEADLINE_VAL);

	return NULL;
}

static void run(void)
{
	pthread_t thread;

	SAFE_PTHREAD_CREATE(&thread, NULL, run_deadline, NULL);
	SAFE_PTHREAD_JOIN(thread, NULL);
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
};
