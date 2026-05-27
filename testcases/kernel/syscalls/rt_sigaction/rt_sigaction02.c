// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project, 2007
 * Ported to LTP by Manas Kumar Nayak <maknayak@in.ibm.com>
 * Copyright (c) Linux Test Project, 2007-2026
 */

/*\
 * Verify that :manpage:`rt_sigaction(2)` returns EFAULT when an invalid
 * act pointer is passed.
 */

#include <signal.h>

#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/rt_sigaction.h"

static void run(void)
{
	int sig;

	for (sig = SIGRTMIN; sig <= SIGRTMAX; sig++) {
		TST_EXP_FAIL(ltp_rt_sigaction(sig, INVAL_SA_PTR, NULL, SIGSETSIZE),
				EFAULT, "rt_sigaction(sig=%d, INVAL_SA_PTR)", sig);
	}
}

static struct tst_test test = {
	.test_all = run,
};
