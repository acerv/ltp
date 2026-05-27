// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Fujitsu Ltd.
 * Author: Xiao Yang <yangx.jy@cn.fujitsu.com>
 */

/*\
 * Verify that :manpage:`removexattr(2)` fails with the correct errno for
 * various error conditions.
 *
 * - ENODATA when the named attribute does not exist
 * - ENOENT when path is an empty string
 * - EFAULT when path points to an invalid address
 */

#include "config.h"
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "tst_test.h"

#ifdef HAVE_SYS_XATTR_H

static struct tcase {
	const char *path;
	const char *name;
	int exp_err;
} tcases[] = {
	{"testfile", "user.test", ENODATA},
	{"", "user.test", ENOENT},
	{(char *)-1, "user.test", EFAULT},
};

static void verify_removexattr(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	TST_EXP_FAIL(removexattr(tc->path, tc->name), tc->exp_err,
			"removexattr(%s, %s)",
			tc->path == (char *)-1 ? "(invalid)" : tc->path,
			tc->name);
}

static void setup(void)
{
	SAFE_TOUCH("testfile", 0644, NULL);

	TEST(setxattr("testfile", "user.test", "test", 4, XATTR_CREATE));
	if (TST_RET == -1 && TST_ERR == ENOTSUP)
		tst_brk(TCONF, "no xattr support in fs or mount without user_xattr option");

	if (TST_RET == 0)
		SAFE_REMOVEXATTR("testfile", "user.test");
}

static struct tst_test test = {
	.setup = setup,
	.test = verify_removexattr,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_tmpdir = 1,
};

#else
TST_TEST_TCONF("<sys/xattr.h> does not exist");
#endif
