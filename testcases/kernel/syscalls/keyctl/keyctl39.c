// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Negative test cases for ``KEYCTL_WATCH_KEY`` of :manpage:`keyctl(2)`.
 *
 * [Algorithm]
 *
 * - verify ``KEYCTL_WATCH_KEY`` with ``watch_id < -1`` fails with ``EINVAL``
 * - verify ``KEYCTL_WATCH_KEY`` with ``watch_id > 255`` fails with ``EINVAL``
 * - verify ``KEYCTL_WATCH_KEY`` with bogus key id fails with ``ENOKEY``
 * - verify ``KEYCTL_WATCH_KEY`` without View permission fails with ``EACCES``
 * - verify ``KEYCTL_WATCH_KEY`` with invalid fd fails with ``EINVAL``
 * - verify ``KEYCTL_WATCH_KEY`` with non-watch-queue fd fails with ``EINVAL``
 * - verify ``KEYCTL_WATCH_KEY`` remove on unwatched key fails with ``EBADSLT``
 */

#define _GNU_SOURCE

#include <unistd.h>
#include "keyctl_common.h"
#include "lapi/watch_queue.h"

static key_serial_t key_valid;
static key_serial_t key_no_view;
static key_serial_t bogus_key = INT32_MAX;
static int wqueue_pipefd[2] = {-1, -1};
static int plain_pipefd[2] = {-1, -1};
static int bad_fd = -1;

static struct tcase {
	key_serial_t *key;
	int *fd;
	int watch_id;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{
		.key = &key_valid,
		.fd = &wqueue_pipefd[0],
		.watch_id = -2,
		.exp_errno = EINVAL,
		.desc = "watch_id < -1",
	},
	{
		.key = &key_valid,
		.fd = &wqueue_pipefd[0],
		.watch_id = 256,
		.exp_errno = EINVAL,
		.desc = "watch_id > 255",
	},
	{
		.key = &bogus_key,
		.fd = &wqueue_pipefd[0],
		.watch_id = 1,
		.exp_errno = ENOKEY,
		.desc = "bogus key id",
	},
	{
		.key = &key_no_view,
		.fd = &wqueue_pipefd[0],
		.watch_id = 1,
		.exp_errno = EACCES,
		.desc = "key without View permission",
	},
	{
		.key = &key_valid,
		.fd = &bad_fd,
		.watch_id = 1,
		.exp_errno = EINVAL,
		.desc = "invalid fd",
	},
	{
		.key = &key_valid,
		.fd = &plain_pipefd[0],
		.watch_id = 1,
		.exp_errno = EINVAL,
		.desc = "non-watch-queue fd",
	},
	{
		.key = &key_valid,
		.fd = &wqueue_pipefd[0],
		.watch_id = -1,
		.exp_errno = EBADSLT,
		.desc = "remove watch on unwatched key",
	},
};

static void setup(void)
{
	SAFE_KEYCTL(KEYCTL_JOIN_SESSION_KEYRING, 0, 0, 0, 0);

	key_valid = new_user_key("ltpkeyctl39_valid", "data", 4,
				 KEY_SPEC_PROCESS_KEYRING);

	key_no_view = new_user_key("ltpkeyctl39_noview", "data", 4,
				   KEY_SPEC_PROCESS_KEYRING);
	SAFE_KEYCTL(KEYCTL_SETPERM, key_no_view, KEY_PERM_NO_VIEW, 0, 0);

	TEST(pipe2(wqueue_pipefd, O_NOTIFICATION_PIPE));
	if (TST_RET < 0) {
		if (TST_ERR == ENOPKG)
			tst_brk(TCONF | TTERRNO, "CONFIG_WATCH_QUEUE is not set");
		if (TST_ERR == EINVAL)
			tst_brk(TCONF | TTERRNO, "O_NOTIFICATION_PIPE is not supported");
		tst_brk(TBROK | TTERRNO, "pipe2(O_NOTIFICATION_PIPE) failed");
	}

	SAFE_IOCTL(wqueue_pipefd[0], IOC_WATCH_QUEUE_SET_SIZE, 256);

	SAFE_PIPE(plain_pipefd);
}

static void cleanup(void)
{
	if (wqueue_pipefd[0] != -1)
		SAFE_CLOSE(wqueue_pipefd[0]);
	if (wqueue_pipefd[1] != -1)
		SAFE_CLOSE(wqueue_pipefd[1]);
	if (plain_pipefd[0] != -1)
		SAFE_CLOSE(plain_pipefd[0]);
	if (plain_pipefd[1] != -1)
		SAFE_CLOSE(plain_pipefd[1]);
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	TST_EXP_FAIL(keyctl(KEYCTL_WATCH_KEY, *tc->key, *tc->fd, tc->watch_id),
		     tc->exp_errno,
		     "%s", tc->desc);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.min_kver = "5.8",
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KEYS=y",
		"CONFIG_KEY_NOTIFICATIONS=y",
		"CONFIG_WATCH_QUEUE=y",
		NULL
	},
};
