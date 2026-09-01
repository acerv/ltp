// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test basic ``KEYCTL_MOVE`` of :manpage:`keyctl(2)`, added in Linux 5.3.
 *
 * [Algorithm]
 *
 * - move a key between two keyrings, verify it can be found only in the
 *   destination afterwards
 */

#include "keyctl_common.h"

#define RING_A_DESC	"ltpkeyctl18_a"
#define RING_B_DESC	"ltpkeyctl18_b"
#define KEY_DESC	"k"
#define PAYLOAD		"payload"

static key_serial_t ring_a, ring_b;
static key_serial_t key;

static void setup(void)
{
	SAFE_KEYCTL(KEYCTL_JOIN_SESSION_KEYRING, 0, 0, 0, 0);

	ring_a = new_ring(RING_A_DESC);
	ring_b = new_ring(RING_B_DESC);
	key = new_user_key(KEY_DESC, PAYLOAD, sizeof(PAYLOAD), ring_a);
}

static void reset_state(void)
{
	SAFE_KEYCTL(KEYCTL_LINK, key, ring_a, 0, 0);
	TEST(keyctl(KEYCTL_UNLINK, key, ring_b, 0, 0));
	if (TST_RET == -1 && TST_ERR != ENOENT)
		tst_brk(TBROK | TTERRNO, "failed to unlink key from ring_b");
}

static void run(void)
{
	reset_state();

	TST_EXP_PASS(keyctl(KEYCTL_MOVE, key, ring_a, ring_b, 0));
	if (!TST_PASS)
		return;

	TST_EXP_EQ_LI(search_ring(ring_b, "user", KEY_DESC), key);

	TST_EXP_FAIL2(search_ring(ring_a, "user", KEY_DESC), ENOKEY,
		      "key no longer found in the source keyring");
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.min_kver = "5.3",
};
