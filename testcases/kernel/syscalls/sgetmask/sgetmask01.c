// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project, 2007
 * Ported from Crackerjack to LTP by Manas Kumar Nayak <maknayak@in.ibm.com>
 */

/*\
 * Verify that :manpage:`sgetmask(2)` returns the value previously set via
 * :manpage:`ssetmask(2)` for signal values from -3 to SIGRTMAX+1.
 */

#include <signal.h>

#include "tst_test.h"
#include "lapi/syscalls.h"

static void run(void)
{
	int sig;
	long ret;

	for (sig = -3; sig <= SIGRTMAX + 1; sig++) {
		tst_syscall(__NR_ssetmask, sig);

		ret = tst_syscall(__NR_sgetmask);
		if (ret != sig)
			tst_res(TINFO, "set %d, got %ld", sig, ret);
		else
			tst_res(TPASS, "set %d, got %ld", sig, ret);
	}
}

static struct tst_test test = {
	.test_all = run,
};
