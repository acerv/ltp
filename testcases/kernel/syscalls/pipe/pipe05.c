// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 *  07/2001 Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`pipe(2)` fails with EFAULT when passed
 * an invalid pointer.
 */

#include "tst_test.h"

static void run(void)
{
	TST_EXP_FAIL(pipe(NULL), EFAULT);
}

static struct tst_test test = {
	.test_all = run,
};
