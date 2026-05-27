// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Fujitsu Ltd.
 * Author: Xiao Yang <yangx.jy@cn.fujitsu.com>
 */

/*\
 * Verify that :manpage:`removexattr(2)` removes an extended attribute
 * from a file and a subsequent :manpage:`getxattr(2)` returns ENODATA.
 */

#include "config.h"
#include <sys/types.h>
#ifdef HAVE_SYS_XATTR_H
# include <sys/xattr.h>
#endif
#include "tst_test.h"

#ifdef HAVE_SYS_XATTR_H

#define USER_KEY	"user.test"
#define VALUE		"test"
#define VALUE_SIZE	(sizeof(VALUE) - 1)

static void verify_removexattr(void)
{
	int n;
	char buf[64];

	n = setxattr("testfile", USER_KEY, VALUE, VALUE_SIZE, XATTR_CREATE);
	if (n == -1) {
		if (errno == ENOTSUP)
			tst_brk(TCONF, "no xattr support in fs or mounted without user_xattr option");
		else
			tst_brk(TBROK | TERRNO, "setxattr() failed");
	}

	TST_EXP_PASS(removexattr("testfile", USER_KEY));
	if (!TST_PASS)
		return;

	n = getxattr("testfile", USER_KEY, buf, sizeof(buf));
	if (n != -1) {
		tst_res(TFAIL, "getxattr() succeeded for deleted key");
		return;
	}

	if (errno != ENODATA)
		tst_res(TFAIL | TERRNO, "getxattr() failed unexpectedly");
	else
		tst_res(TPASS, "removexattr() succeeded");
}

static void setup(void)
{
	SAFE_TOUCH("testfile", 0644, NULL);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = verify_removexattr,
	.needs_tmpdir = 1,
};

#else /* HAVE_SYS_XATTR_H */
TST_TEST_TCONF("<sys/xattr.h> does not exist");
#endif
