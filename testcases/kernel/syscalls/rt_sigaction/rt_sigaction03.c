// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project, 2007
 * Copyright (c) Manas Kumar Nayak <maknayak@in.ibm.com>
 */

/*\
 * Verify that :manpage:`rt_sigaction(2)` fails with EINVAL when
 * sigsetsize does not match the size of a sigset_t type.
 */

#include <signal.h>

#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/rt_sigaction.h"

#define INVAL_SIGSETSIZE (-1)

static struct tcase {
	int flags;
	const char *flags_str;
} tcases[] = {
	{SA_RESETHAND | SA_SIGINFO, "SA_RESETHAND|SA_SIGINFO"},
	{SA_RESETHAND, "SA_RESETHAND"},
	{SA_NODEFER, "SA_NODEFER"},
};

static void handler(int sig LTP_ATTRIBUTE_UNUSED)
{
}

static int set_handler(int sig, int mask_flags)
{
	struct sigaction sa, oldaction;

	sa.sa_sigaction = (void *)handler;
	sa.sa_flags = mask_flags;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, sig);

	return ltp_rt_sigaction(sig, &sa, &oldaction, INVAL_SIGSETSIZE);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];
	int sig;

	for (sig = SIGRTMIN; sig <= SIGRTMAX; sig++) {
		TST_EXP_FAIL(set_handler(sig, tc->flags), EINVAL,
				"sa_flags=%s sig=%d", tc->flags_str, sig);
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
};
