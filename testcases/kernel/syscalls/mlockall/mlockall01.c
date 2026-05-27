// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * Author: Nirmala Devi Dhanasekar <nirmala.devi@wipro.com>
 */

/*\
 * Verify that :manpage:`mlockall(2)` locks all pages mapped into the
 * address space of the calling process with MCL_CURRENT, MCL_FUTURE,
 * and MCL_CURRENT|MCL_FUTURE flags.
 */

#include <sys/mman.h>
#include "tst_test.h"

static struct tcase {
	int flag;
	const char *fdesc;
} tcases[] = {
	{MCL_CURRENT, "MCL_CURRENT"},
	{MCL_FUTURE, "MCL_FUTURE"},
	{MCL_CURRENT | MCL_FUTURE, "MCL_CURRENT|MCL_FUTURE"},
};

static void verify_mlockall(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	TST_EXP_PASS(mlockall(tc->flag), "mlockall(%s)", tc->fdesc);
}

static struct tst_test test = {
	.test = verify_mlockall,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_root = 1,
};
