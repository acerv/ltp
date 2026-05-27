// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`getresgid(2)` successfully retrieves the real,
 * effective and saved group IDs of the calling process.
 */

#define _GNU_SOURCE
#include <unistd.h>

#include "tst_test.h"

static void run(void)
{
	gid_t real_gid, eff_gid, sav_gid;
	gid_t exp_rgid, exp_egid;

	exp_rgid = getgid();
	exp_egid = getegid();

	TST_EXP_PASS(getresgid(&real_gid, &eff_gid, &sav_gid));

	if (!TST_PASS)
		return;

	if (real_gid != exp_rgid || eff_gid != exp_egid ||
	    sav_gid != exp_egid) {
		tst_res(TFAIL,
			"got rgid=%d egid=%d sgid=%d, expected rgid=%d egid=%d sgid=%d",
			real_gid, eff_gid, sav_gid,
			exp_rgid, exp_egid, exp_egid);
	} else {
		tst_res(TPASS, "getresgid returned correct values");
	}
}

static struct tst_test test = {
	.test_all = run,
};
