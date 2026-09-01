// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test ``KEYCTL_CAPABILITIES`` size query of :manpage:`keyctl(2)`, added in Linux 5.3.
 *
 * [Algorithm]
 *
 * - call ``KEYCTL_CAPABILITIES`` with NULL buffer and ``buflen = 0``
 * - verify the return value is at least 2 bytes
 * - verify calling ``KEYCTL_CAPABILITIES`` with a valid buffer returns the
 *   same size
 */

#include "keyctl_common.h"

static unsigned char buf[8];

static void run(void)
{
	long size;

	TEST(keyctl(KEYCTL_CAPABILITIES, (unsigned long)NULL, 0, 0, 0));
	if (TST_RET < 0)
		tst_brk(TBROK | TTERRNO, "KEYCTL_CAPABILITIES size query failed");

	size = TST_RET;
	if (size < 2) {
		tst_res(TFAIL, "KEYCTL_CAPABILITIES size query returned %ld, expected >= 2",
			size);
		return;
	}

	tst_res(TPASS, "KEYCTL_CAPABILITIES size query returned %ld", size);

	TST_EXP_VAL(keyctl(KEYCTL_CAPABILITIES, (unsigned long)buf, sizeof(buf), 0, 0),
		    size);
}

static struct tst_test test = {
	.test_all = run,
	.min_kver = "5.3",
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KEYS=y",
		NULL
	},
};
