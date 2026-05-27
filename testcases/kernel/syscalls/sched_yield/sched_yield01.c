// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Copyright (c) Linux Test Project, 2001-2025
 * Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`sched_yield(2)` succeeds.
 */

#include "tst_test.h"

static void run(void)
{
	TST_EXP_PASS(sched_yield());
}

static struct tst_test test = {
	.test_all = run,
};
