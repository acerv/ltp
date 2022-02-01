// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>

#define TST_NO_DEFAULT_MAIN

#include "libclone.h"
#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/namespaces_constants.h"

int tst_clone_tests(unsigned long clone_flags, int (*fn1)(void *arg),
		    void *arg1, int (*fn2)(void *arg), void *arg2)
{
	int ret;

	ret = ltp_clone_quick(clone_flags | SIGCHLD, fn1, arg1);

	if (ret == -1) {
		return -1;
	}
	if (fn2)
		ret = fn2(arg2);
	else
		ret = 0;

	return ret;
}

int tst_unshare_tests(unsigned long clone_flags, int (*fn1)(void *arg),
		      void *arg1, int (*fn2)(void *arg), void *arg2)
{
	int pid, ret = 0;
	int retpipe[2];
	char buf[2];

	SAFE_PIPE(retpipe);

	pid = fork();
	if (pid < 0) {
		SAFE_CLOSE(retpipe[0]);
		SAFE_CLOSE(retpipe[1]);
		tst_brk(TBROK, "fork");
	}

	if (!pid) {
		SAFE_CLOSE(retpipe[0]);

		ret = tst_syscall(SYS_unshare, clone_flags);
		if (ret == -1) {
			SAFE_WRITE(1, retpipe[1], "0", 2);
			SAFE_CLOSE(retpipe[1]);
			exit(1);
		} else {
			SAFE_WRITE(1, retpipe[1], "1", 2);
		}

		SAFE_CLOSE(retpipe[1]);

		ret = fn1(arg1);
		exit(ret);
	}

	SAFE_CLOSE(retpipe[1]);
	SAFE_READ(1, retpipe[0], &buf, 2);
	SAFE_CLOSE(retpipe[0]);

	if (*buf == '0')
		return -1;

	if (fn2)
		ret = fn2(arg2);

	return ret;
}

int tst_plain_tests(int (*fn1)(void *arg), void *arg1, int (*fn2)(void *arg),
		    void *arg2)
{
	int ret = 0, pid;

	pid = SAFE_FORK();
	if (!pid)
		exit(fn1(arg1));

	if (fn2)
		ret = fn2(arg2);

	return ret;
}

int tst_clone_unshare_test(int use_clone, unsigned long clone_flags,
			   int (*fn1)(void *arg), void *arg1)
{
	int ret = 0;

	switch (use_clone) {
	case T_NONE:
		ret = tst_plain_tests(fn1, arg1, NULL, NULL);
		break;
	case T_CLONE:
		ret = tst_clone_tests(clone_flags, fn1, arg1, NULL, NULL);
		break;
	case T_UNSHARE:
		ret = tst_unshare_tests(clone_flags, fn1, arg1, NULL, NULL);
		break;
	default:
		ret = -1;
		tst_brk(TBROK, "%s: bad use_clone option: %d", __FUNCTION__,
			use_clone);
		break;
	}

	return ret;
}

/*
 * Run fn1 in a unshared environmnent, and fn2 in the original context
 */
int tst_clone_unshare_tests(int use_clone, unsigned long clone_flags,
			    int (*fn1)(void *arg), void *arg1,
			    int (*fn2)(void *arg), void *arg2)
{
	int ret = 0;

	switch (use_clone) {
	case T_NONE:
		ret = tst_plain_tests(fn1, arg1, fn2, arg2);
		break;
	case T_CLONE:
		ret = tst_clone_tests(clone_flags, fn1, arg1, fn2, arg2);
		break;
	case T_UNSHARE:
		ret = tst_unshare_tests(clone_flags, fn1, arg1, fn2, arg2);
		break;
	default:
		ret = -1;
		tst_brk(TBROK, "%s: bad use_clone option: %d", __FUNCTION__,
			use_clone);
		break;
	}

	return ret;
}
