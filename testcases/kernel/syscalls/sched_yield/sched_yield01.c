// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Simply verify that sched_yield() syscall is not failing when it's called.
 */

#include "tst_test.h"

static void run(void)
{
	TST_EXP_PASS(sched_yield());
}

static struct tst_test test = {
	.test_all = run,
};
