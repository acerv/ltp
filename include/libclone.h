// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef __LIBCLONE_H
#define __LIBCLONE_H

#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/namespaces_constants.h"

#define T_UNSHARE 0
#define T_CLONE 1
#define T_NONE 2

#ifndef SYS_unshare
#ifdef __NR_unshare
#define SYS_unshare __NR_unshare
#elif __i386__
#define SYS_unshare 310
#elif __ia64__
#define SYS_unshare 1296
#elif __x86_64__
#define SYS_unshare 272
#elif __s390x__ || __s390__
#define SYS_unshare 303
#elif __powerpc__
#define SYS_unshare 282
#else
#error "unshare not supported on this architecure."
#endif
#endif

#ifndef __NR_unshare
#define __NR_unshare SYS_unshare
#endif

/*
 * Run fn1 in a unshared environmnent, and fn2 in the original context
 * Fn2 may be NULL.
 */

int tst_clone_tests(unsigned long clone_flags, int (*fn1)(void *arg),
		    void *arg1, int (*fn2)(void *arg), void *arg2);

int tst_unshare_tests(unsigned long clone_flags, int (*fn1)(void *arg),
		      void *arg1, int (*fn2)(void *arg), void *arg2);

int tst_fork_tests(int (*fn1)(void *arg), void *arg1, int (*fn2)(void *arg),
		   void *arg2);

int tst_clone_unshare_test(int use_clone, unsigned long clone_flags,
			   int (*fn1)(void *arg), void *arg1);

int tst_clone_unshare_tests(int use_clone, unsigned long clone_flags,
			    int (*fn1)(void *arg), void *arg1,
			    int (*fn2)(void *arg), void *arg2);

#endif
