// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2002
 * 01/02/2003	Port to LTP	avenkat@us.ibm.com
 * 06/30/2001	Port to Linux	nsharoff@us.ibm.com
 */

/*\
 * Verify that :manpage:`abs(3)` and llabs() correctly compute the absolute
 * value of integers, including zero, the minimum integer value, and
 * values derived from bitwise complement operations.
 */

#include <stdlib.h>
#include <limits.h>

#include "tst_test.h"

static void run(void)
{
	long long i;
	int j, k, l, m;

	i = llabs(INT_MIN) + (long long)INT_MIN;
	if (i != 0)
		tst_res(TFAIL, "llabs(INT_MIN) + INT_MIN != 0");
	else
		tst_res(TPASS, "abs of minimum integer correct");

	i = llabs(0);
	if (i != 0)
		tst_res(TFAIL, "llabs(0) returned %lld", i);
	else
		tst_res(TPASS, "abs(0) returned 0");

	for (m = 1; m >= 0; m <<= 1) {
		j = ~m;
		k = j + 1;
		l = abs(k);

		if (l != m) {
			tst_res(TFAIL, "abs(%d) = %d, expected %d", k, l, m);
			return;
		}
	}
	tst_res(TPASS, "abs of bitwise complement values correct");
}

static struct tst_test test = {
	.test_all = run,
};
