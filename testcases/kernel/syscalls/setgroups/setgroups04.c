// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) Bull S.A. 2001
 * Copyright (c) International Business Machines Corp., 2001
 */

/*\
 * Verify that :manpage:`setgroups(2)` fails with EFAULT when the list
 * argument points to an invalid address.
 */

#include "tst_test.h"
#include "compat_tst_16.h"

static void run(void)
{
	TST_EXP_FAIL(SETGROUPS(NGROUPS, sbrk(0)), EFAULT);
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
};
