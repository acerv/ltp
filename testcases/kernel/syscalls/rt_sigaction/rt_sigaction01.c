// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project, 2007
 * Copyright (c) Linux Test Project, 2007-2026
 * Ported from Crackerjack to LTP by Manas Kumar Nayak <maknayak@in.ibm.com>
 */

/*\
 * Verify that :manpage:`rt_sigaction(2)` succeeds to set signal handlers
 * for all real-time signals (SIGRTMIN to SIGRTMAX) with various sa_flags
 * combinations, and that the installed handler is invoked when the signal
 * is raised.
 */

#include <signal.h>

#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/rt_sigaction.h"

static struct tcase {
	int flags;
	const char *flags_str;
} tcases[] = {
	{SA_RESETHAND | SA_SIGINFO, "SA_RESETHAND|SA_SIGINFO"},
	{SA_RESETHAND, "SA_RESETHAND"},
	{SA_NODEFER, "SA_NODEFER"},
};

static volatile int handler_called;

static void handler(int sig LTP_ATTRIBUTE_UNUSED)
{
	handler_called = 1;
}

static int set_handler(int sig, int mask_flags)
{
	struct sigaction sa, oldaction;

	sa.sa_handler = handler;
	sa.sa_flags = mask_flags;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, sig);

	return ltp_rt_sigaction(sig, &sa, &oldaction, SIGSETSIZE);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];
	int sig;

	for (sig = SIGRTMIN; sig <= SIGRTMAX; sig++) {
		handler_called = 0;

		TST_EXP_PASS(set_handler(sig, tc->flags),
				"signal=%d sa_flags=%s", sig, tc->flags_str);

		if (!TST_PASS)
			continue;

		SAFE_KILL(getpid(), sig);

		if (handler_called)
			tst_res(TPASS, "handler invoked for signal=%d", sig);
		else
			tst_res(TFAIL, "handler not invoked for signal=%d", sig);
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
};
