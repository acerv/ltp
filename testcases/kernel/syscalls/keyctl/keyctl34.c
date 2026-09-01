// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test ``KEYCTL_CAPABILITIES`` of :manpage:`keyctl(2)`, added in Linux 5.3.
 *
 * ``KEYCTL_CAPABILITIES`` retrieves the bitmask of features and capabilities
 * supported by the keyrings subsystem in the running kernel.
 *
 * [Algorithm]
 *
 * - retrieve keyrings subsystem capabilities into a buffer
 * - verify return value is at least 2 bytes
 * - verify standard capability bits ``KEYCTL_CAPS0_CAPABILITIES``,
 *   ``KEYCTL_CAPS0_INVALIDATE``, ``KEYCTL_CAPS0_RESTRICT_KEYRING``,
 *   ``KEYCTL_CAPS0_MOVE``, ``KEYCTL_CAPS1_NS_KEYRING_NAME``, and
 *   ``KEYCTL_CAPS1_NS_KEY_TAG`` are set
 */

#include "keyctl_common.h"

static unsigned char caps[8];

static void run(void)
{
	memset(caps, 0, sizeof(caps));

	TEST(keyctl(KEYCTL_CAPABILITIES, (unsigned long)caps, sizeof(caps), 0, 0));
	if (TST_RET < 0)
		tst_brk(TBROK | TTERRNO, "KEYCTL_CAPABILITIES failed");

	if (TST_RET < 2) {
		tst_res(TFAIL, "KEYCTL_CAPABILITIES returned %ld, expected at least 2",
			TST_RET);
		return;
	}

	TST_EXP_EXPR(caps[0] & KEYCTL_CAPS0_CAPABILITIES,
		     "KEYCTL_CAPS0_CAPABILITIES is set");
	TST_EXP_EXPR(caps[0] & KEYCTL_CAPS0_INVALIDATE,
		     "KEYCTL_CAPS0_INVALIDATE is set");
	TST_EXP_EXPR(caps[0] & KEYCTL_CAPS0_RESTRICT_KEYRING,
		     "KEYCTL_CAPS0_RESTRICT_KEYRING is set");
	TST_EXP_EXPR(caps[0] & KEYCTL_CAPS0_MOVE,
		     "KEYCTL_CAPS0_MOVE is set");

	TST_EXP_EXPR(caps[1] & KEYCTL_CAPS1_NS_KEYRING_NAME,
		     "KEYCTL_CAPS1_NS_KEYRING_NAME is set");
	TST_EXP_EXPR(caps[1] & KEYCTL_CAPS1_NS_KEY_TAG,
		     "KEYCTL_CAPS1_NS_KEY_TAG is set");
}

static struct tst_test test = {
	.test_all = run,
	.min_kver = "5.3",
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KEYS=y",
		NULL
	},
};
