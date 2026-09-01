// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test ``KEYCTL_MOVE`` displacement of :manpage:`keyctl(2)`.
 *
 * [Algorithm]
 *
 * - move a key without ``KEYCTL_MOVE_EXCL`` into a destination keyring that
 *   already contains a matching key, verify it displaces the existing key
 */

#include "keyctl_common.h"

#define RING_A_DESC	"ltpkeyctl20_a"
#define RING_B_DESC	"ltpkeyctl20_b"
#define KEY_DESC	"k"
#define PAYLOAD		"payload"

static key_serial_t ring_a, ring_b;
static key_serial_t key_a, key_excl;

static void setup(void)
{
	SAFE_KEYCTL(KEYCTL_JOIN_SESSION_KEYRING, 0, 0, 0, 0);

	ring_a = new_ring(RING_A_DESC);
	ring_b = new_ring(RING_B_DESC);

	key_a = new_user_key(KEY_DESC, PAYLOAD, sizeof(PAYLOAD), ring_a);
	key_excl = new_user_key(KEY_DESC, PAYLOAD, sizeof(PAYLOAD), ring_b);

	/* Keep key_excl in session keyring so displacement does not destroy it */
	SAFE_KEYCTL(KEYCTL_LINK, key_excl, KEY_SPEC_SESSION_KEYRING, 0, 0);
}

static void reset_state(void)
{
	SAFE_KEYCTL(KEYCTL_LINK, key_a, ring_a, 0, 0);
	TEST(keyctl(KEYCTL_UNLINK, key_a, ring_b, 0, 0));
	if (TST_RET == -1 && TST_ERR != ENOENT)
		tst_brk(TBROK | TTERRNO, "failed to unlink key_a from ring_b");

	SAFE_KEYCTL(KEYCTL_LINK, key_excl, ring_b, 0, 0);
}

static void run(void)
{
	reset_state();

	TST_EXP_PASS(keyctl(KEYCTL_MOVE, key_a, ring_a, ring_b, 0));

	TST_EXP_EQ_LI(search_ring(ring_b, "user", KEY_DESC), key_a);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.min_kver = "5.3",
};
