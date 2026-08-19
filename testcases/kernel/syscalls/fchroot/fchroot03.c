// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test the fchroot() permission checks with a regular directory fd.
 *
 * fchroot() was introduced in Linux v7.3. With a regular directory fd the
 * kernel first checks that the caller has execute permission on the
 * directory, then that it holds CAP_SYS_CHROOT:
 *
 * - an unprivileged process with an accessible directory fails with EPERM
 * - a process without execute permission on the directory fails with
 *   EACCES, proving the permission check comes before the capability
 *   check
 *
 * Root is required to open the directory file descriptors before dropping
 * to an unprivileged user in forked children.
 */

#include <fcntl.h>
#include <pwd.h>
#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/syscalls.h"

static struct tcase {
	const char *dir;
	mode_t mode;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{"pubdir", 0755, EPERM, "no CAP_SYS_CHROOT"},
	{"privdir", 0600, EACCES, "no execute permission"},
};

static struct passwd *ltpuser;

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	if (!SAFE_FORK()) {
		int dfd = SAFE_OPEN(tc->dir, O_PATH | O_DIRECTORY);

		SAFE_SETRESUID(ltpuser->pw_uid, ltpuser->pw_uid,
			ltpuser->pw_uid);

		TST_EXP_FAIL(tst_syscall(__NR_fchroot, dfd, 0),
			tc->exp_errno, "fchroot() with %s", tc->desc);

		exit(0);
	}
}

static void setup(void)
{
	unsigned int i;

	ltpuser = SAFE_GETPWNAM("nobody");

	for (i = 0; i < ARRAY_SIZE(tcases); i++) {
		SAFE_MKDIR(tcases[i].dir, tcases[i].mode);
		SAFE_CHMOD(tcases[i].dir, tcases[i].mode);
	}
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.forks_child = 1,
};
