// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test unprivileged fchroot() into failfs with no_new_privs set.
 *
 * An unprivileged process may enter failfs when it committed to
 * no_new_privs, since setuid binaries then pose no confused deputy risk
 * anymore. The test also verifies that a process which entered failfs
 * counts as chrooted: it can no longer create a user namespace with
 * :manpage:`unshare(2)` to regain CAP_SYS_CHROOT.
 *
 * The user namespace check only works when CONFIG_USER_NS is enabled:
 * without it unshare(CLONE_NEWUSER) fails with EINVAL for everybody and
 * the check is skipped with TCONF.
 *
 * Root is required to drop to an unprivileged user in the forked child.
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <pwd.h>
#include <sched.h>
#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/prctl.h"
#include "lapi/sched.h"
#include "lapi/syscalls.h"

static struct passwd *ltpuser;

static void check_userns_blocked(void)
{
	TEST(unshare(CLONE_NEWUSER));
	if (TST_RET == -1 && TST_ERR == EPERM) {
		tst_res(TPASS, "user namespace creation blocked by the failfs root");
	} else if (TST_RET == -1 && TST_ERR == EINVAL) {
		tst_res(TCONF, "CONFIG_USER_NS is disabled, "
			"user namespace creation can not be tested");
	} else {
		tst_res(TFAIL | TTERRNO,
			"unshare() unexpectedly returned %ld", TST_RET);
	}
}

static void run(void)
{
	if (!SAFE_FORK()) {
		SAFE_SETRESUID(ltpuser->pw_uid, ltpuser->pw_uid,
			ltpuser->pw_uid);

		SAFE_PRCTL(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

		TST_EXP_PASS(tst_syscall(__NR_fchroot, FD_FAILFS_ROOT, 0),
			"unprivileged fchroot() with no_new_privs");

		TST_EXP_FAIL2(open("/etc/passwd", O_RDONLY), EOPNOTSUPP,
			"absolute lookup after entering failfs");

		check_userns_blocked();

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
	.needs_kconfigs = (const char *[]) {
		"CONFIG_USER_NS=y",
		NULL,
	}
};
