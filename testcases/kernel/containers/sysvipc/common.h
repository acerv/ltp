// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 *               Rishikesh K Rajak <risrajak@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/namespaces_constants.h"

enum {
	T_CLONE,
	T_UNSHARE,
	T_NONE,
};

static int dummy_child(void *v)
{
	(void)v;
	return 0;
}

static void check_newipc(void)
{
	int pid, status;

	if (tst_kvercmp(2, 6, 19) < 0)
		tst_brk(TCONF, "CLONE_NEWIPC not supported");

	pid = ltp_clone_quick(CLONE_NEWIPC | SIGCHLD, dummy_child, NULL);
	if (pid < 0)
		tst_brk(TCONF | TERRNO, "CLONE_NEWIPC not supported");

	SAFE_WAITPID(pid, &status, 0);
}

static inline int get_clone_unshare_enum(const char* str_op)
{
	int use_clone;

	if (strcmp(str_op, "clone") &&
		strcmp(str_op, "unshare") &&
		strcmp(str_op, "none"))
		tst_brk(TBROK, "Test execution mode <clone|unshare|none>");

	if (!strcmp(str_op, "clone"))
		use_clone = T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		use_clone = T_UNSHARE;
	else
		use_clone = T_NONE;

	return use_clone;
}

static int clone_test(unsigned long clone_flags, int (*fn1)(void *arg),
		      void *arg1)
{
	int ret;

	ret = ltp_clone_quick(clone_flags | SIGCHLD, fn1, arg1);

	if (ret != -1) {
		/* no errors: we'll get the PID id that means success */
		ret = 0;
	}

	return ret;
}

static int unshare_test(unsigned long clone_flags, int (*fn1)(void *arg),
			void *arg1)
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

	return ret;
}

static int plain_test(int (*fn1)(void *arg), void *arg1)
{
	int ret = 0, pid;

	pid = SAFE_FORK();
	if (!pid)
		exit(fn1(arg1));

	return ret;
}

static void clone_unshare_test(int use_clone, unsigned long clone_flags,
			       int (*fn1)(void *arg), void *arg1)
{
	int ret = 0;

	switch (use_clone) {
	case T_NONE:
		ret = plain_test(fn1, arg1);
		break;
	case T_CLONE:
		ret = clone_test(clone_flags, fn1, arg1);
		break;
	case T_UNSHARE:
		ret = unshare_test(clone_flags, fn1, arg1);
		break;
	default:
		ret = -1;
		tst_brk(TBROK, "%s: bad use_clone option: %d", __FUNCTION__,
			use_clone);
		break;
	}

	if (ret == -1)
		tst_brk(TBROK, "child2 clone failed");
}

#endif
