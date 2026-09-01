// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Negative test cases for ``KEYCTL_CAPABILITIES`` of :manpage:`keyctl(2)`.
 *
 * [Algorithm]
 *
 * - verify ``KEYCTL_CAPABILITIES`` with NULL buffer and ``buflen > 0`` fails with ``EFAULT``
 * - verify ``KEYCTL_CAPABILITIES`` with invalid pointer fails with ``EFAULT``
 */

#include "keyctl_common.h"

static void *bad_addr;

static struct tcase {
	void **buf;
	size_t buflen;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{
		.buf = NULL,
		.buflen = 1,
		.exp_errno = EFAULT,
		.desc = "NULL buffer with non-zero buflen",
	},
	{
		.buf = &bad_addr,
		.buflen = 1,
		.exp_errno = EFAULT,
		.desc = "invalid buffer pointer",
	},
};

static void setup(void)
{
	bad_addr = tst_get_bad_addr(NULL);
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];
	void *p = tc->buf ? *tc->buf : NULL;

	TST_EXP_FAIL2(keyctl(KEYCTL_CAPABILITIES, (unsigned long)p, tc->buflen, 0, 0),
		      tc->exp_errno,
		      "%s", tc->desc);
}

static struct tst_test test = {
	.setup = setup,
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.min_kver = "5.3",
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KEYS=y",
		NULL
	},
};
