// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 *    AUTHOR: William Roske
 *    CO-PILOT: Dave Fenner
 * Copyright (c) Linux Test Project, 2006-2026
 */

/*\
 * Verify that :manpage:`setpgrp(2)` succeeds and returns zero when called
 * from a process that is not a process group leader.
 */

#include <stdlib.h>
#include "tst_test.h"

static void run(void)
{
	if (!SAFE_FORK()) {
		TST_EXP_PASS(setpgrp());
		exit(0);
	}
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
};
