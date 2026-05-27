// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Author: Ramon de Carvalho Valle <rcvalle@br.ibm.com>
 */

/*\
 * Verify that :manpage:`set_robust_list(2)` fails with EINVAL when
 * the len argument does not match the expected size, and succeeds when
 * the correct size is passed.
 */

#include <sys/syscall.h>

#include "tst_test.h"
#include "lapi/syscalls.h"

#ifdef __NR_set_robust_list

struct robust_list {
	struct robust_list *next;
};

struct robust_list_head {
	struct robust_list list;
	long futex_offset;
	struct robust_list *list_op_pending;
};

static struct robust_list_head head;

static struct tcase {
	size_t len;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{(size_t)-1, EINVAL, "invalid len"},
	{0, 0, "correct len"},
};

static void setup(void)
{
	tcases[1].len = sizeof(struct robust_list_head);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	if (tc->exp_errno) {
		TST_EXP_FAIL(syscall(__NR_set_robust_list, &head, tc->len),
				tc->exp_errno, "%s", tc->desc);
	} else {
		TST_EXP_PASS(syscall(__NR_set_robust_list, &head, tc->len),
				"%s", tc->desc);
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
};

#else
TST_TEST_TCONF("__NR_set_robust_list is not defined on your system");
#endif
