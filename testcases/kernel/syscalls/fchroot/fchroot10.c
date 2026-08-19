// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test that unprivileged fchroot() into failfs is refused without
 * no_new_privs.
 *
 * Without no_new_privs a setuid binary on a regular mount is still
 * reachable via an inherited directory file descriptor, and executing it
 * with an unusable root directory is the classic confused deputy, so the
 * kernel refuses the syscall with EPERM.
 *
 * Root is required to drop to an unprivileged user in the forked child.
 */

#define _GNU_SOURCE
#include <pwd.h>
#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/syscalls.h"

static struct passwd *ltpuser;

static void run(void)
{
	if (!SAFE_FORK()) {
		SAFE_SETRESUID(ltpuser->pw_uid, ltpuser->pw_uid,
			ltpuser->pw_uid);

		TST_EXP_FAIL(tst_syscall(__NR_fchroot, FD_FAILFS_ROOT, 0),
			EPERM, "unprivileged fchroot() without no_new_privs");

		exit(0);
	}
}

static void setup(void)
{
	ltpuser = SAFE_GETPWNAM("nobody");
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_root = 1,
	.forks_child = 1,
};
