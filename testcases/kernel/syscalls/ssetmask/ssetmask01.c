// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project, 2007
 * Copyright (c) Manas Kumar Nayak <maknayak@in.ibm.com>
 */

/*\
 * Verify that :manpage:`ssetmask(2)` correctly sets the current signal mask
 * and returns the previous one.
 *
 * - Set the signal mask to SIGALRM using ssetmask() and verify via
 *   sgetmask() that the mask is SIGALRM
 * - Set the signal mask to SIGUSR1 using ssetmask() and verify that
 *   the returned previous mask is SIGALRM
 */

#include <signal.h>

#include "tst_test.h"
#include "lapi/syscalls.h"

static void run(void)
{
	tst_syscall(__NR_ssetmask, SIGALRM);

	TST_EXP_VAL(tst_syscall(__NR_sgetmask), SIGALRM,
			"sgetmask() after ssetmask(SIGALRM)");

	TST_EXP_VAL(tst_syscall(__NR_ssetmask, SIGUSR1), SIGALRM,
			"ssetmask(SIGUSR1) returns previous mask");
}

static struct tst_test test = {
	.test_all = run,
};
