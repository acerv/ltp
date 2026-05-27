// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project, 2007
 * Porting from Crackerjack to LTP by
 * Manas Kumar Nayak maknayak@in.ibm.com>
 */

/*\
 * Verify that :manpage:`rt_sigprocmask(2)` fails with:
 *
 * - EINVAL when sigsetsize is invalid
 * - EFAULT when oset points to invalid memory
 */

#include <signal.h>
#include <errno.h>

#include "tst_test.h"
#include "lapi/syscalls.h"

#define SIGSETSIZE (_NSIG / 8)

static sigset_t *bad_addr;
static sigset_t set;

static struct tcase {
	sigset_t **ss;
	int sssize;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{NULL, 1, EINVAL, "invalid sigsetsize"},
	{&bad_addr, SIGSETSIZE, EFAULT, "invalid oset pointer"},
};

static void setup(void)
{
	bad_addr = tst_get_bad_addr(NULL);

	SAFE_SIGFILLSET(&set);
}

static void verify_rt_sigprocmask(unsigned int n)
{
	struct tcase *tc = &tcases[n];
	sigset_t *oset = tc->ss ? *tc->ss : NULL;

	TST_EXP_FAIL(tst_syscall(__NR_rt_sigprocmask, SIG_BLOCK,
			&set, oset, tc->sssize),
			tc->exp_errno, "%s", tc->desc);
}

static struct tst_test test = {
	.setup = setup,
	.test = verify_rt_sigprocmask,
	.tcnt = ARRAY_SIZE(tcases),
};
