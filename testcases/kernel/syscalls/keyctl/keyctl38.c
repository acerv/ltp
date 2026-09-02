// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test ``KEYCTL_WATCH_KEY`` of :manpage:`keyctl(2)`, added in Linux 5.8.
 *
 * ``KEYCTL_WATCH_KEY`` adds or removes a watch on a key or keyring to/from
 * a watch queue notification pipe.
 *
 * [Algorithm]
 *
 * - open a notification pipe via :manpage:`pipe2()` with
 *   ``O_NOTIFICATION_PIPE``
 * - add a watch on a key with ``watch_id = 1`` and verify success
 * - remove the watch on the key with ``watch_id = -1`` and verify success
 * - add a watch on a keyring with ``watch_id = 2`` and verify success
 * - remove the watch on the keyring with ``watch_id = -1`` and verify success
 */

#define _GNU_SOURCE

#include <sys/resource.h>
#include <unistd.h>
#include "keyctl_common.h"
#include "lapi/watch_queue.h"

static key_serial_t key;

static void setup(void)
{
	SAFE_KEYCTL(KEYCTL_JOIN_SESSION_KEYRING, 0, 0, 0, 0);

	key = new_user_key("ltpkeyctl38", "payload", 7,
			   KEY_SPEC_PROCESS_KEYRING);
}

static void run(void)
{
	int pipefd[2];

	TEST(pipe2(pipefd, O_NOTIFICATION_PIPE));
	if (TST_RET < 0) {
		if (TST_ERR == ENOPKG)
			tst_brk(TCONF | TTERRNO, "CONFIG_WATCH_QUEUE is not set");
		if (TST_ERR == EINVAL)
			tst_brk(TCONF | TTERRNO, "O_NOTIFICATION_PIPE is not supported");
		tst_brk(TBROK | TTERRNO, "pipe2(O_NOTIFICATION_PIPE) failed");
	}

	SAFE_IOCTL(pipefd[0], IOC_WATCH_QUEUE_SET_SIZE, 256);

	TST_EXP_PASS(keyctl(KEYCTL_WATCH_KEY, key, pipefd[0], 1),
		     "KEYCTL_WATCH_KEY add watch on key");

	TST_EXP_PASS(keyctl(KEYCTL_WATCH_KEY, key, pipefd[0], -1),
		     "KEYCTL_WATCH_KEY remove watch on key");

	TST_EXP_PASS(keyctl(KEYCTL_WATCH_KEY, KEY_SPEC_SESSION_KEYRING,
			    pipefd[0], 2),
		     "KEYCTL_WATCH_KEY add watch on session keyring");

	TST_EXP_PASS(keyctl(KEYCTL_WATCH_KEY, KEY_SPEC_SESSION_KEYRING,
			    pipefd[0], -1),
		     "KEYCTL_WATCH_KEY remove watch on session keyring");

	SAFE_CLOSE(pipefd[0]);
	SAFE_CLOSE(pipefd[1]);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.min_kver = "5.8",
	/*
	 * add_one_watch() charges per-user watches against RLIMIT_NOFILE,
	 * so raise the limit high enough to accommodate pre-existing
	 * watches on the calling user.
	 */
	.ulimit = (const struct tst_ulimit_val []) {
		{RLIMIT_NOFILE, 524288},
		{}
	},
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KEYS=y",
		"CONFIG_KEY_NOTIFICATIONS=y",
		"CONFIG_WATCH_QUEUE=y",
		NULL
	},
};
