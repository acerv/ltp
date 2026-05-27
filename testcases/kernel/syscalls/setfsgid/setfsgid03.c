// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) International Business Machines Corp., 2001
 * Ported by Wayne Boyer
 * Adapted by Dustin Kirkland (k1rkland@us.ibm.com)
 */

/*\
 * Verify that :manpage:`setfsgid(2)` called by a non-root user does not
 * change the filesystem GID to a value the caller does not have permission
 * to use, and returns the previous filesystem GID.
 */

#include <pwd.h>
#include <grp.h>
#include <sys/fsuid.h>

#include "tst_test.h"
#include "compat_tst_16.h"

static void run(void)
{
	gid_t gid;
	gid_t prev_fsgid;

	gid = 1;
	while (!getgrgid(gid))
		gid++;

	GID16_CHECK(gid, setfsgid);

	prev_fsgid = SETFSGID(-1);

	TEST(SETFSGID(gid));

	if (TST_RET == -1) {
		tst_res(TFAIL | TTERRNO, "setfsgid() failed unexpectedly");
		return;
	}

	if (gid == TST_RET) {
		tst_res(TFAIL, "setfsgid() returned %ld, expected %d",
			TST_RET, prev_fsgid);
	} else {
		tst_res(TPASS, "setfsgid() returned expected value: %ld",
			TST_RET);
	}
}

static void setup(void)
{
	struct passwd *ltpuser;

	ltpuser = SAFE_GETPWNAM("nobody");
	SAFE_SETUID(ltpuser->pw_uid);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
};
