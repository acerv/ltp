// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`getresuid(2)` successfully retrieves the real,
 * effective and saved user IDs of the calling process.
 */

#define _GNU_SOURCE
#include <unistd.h>

#include "tst_test.h"

static void run(void)
{
	uid_t real_uid, eff_uid, sav_uid;
	uid_t exp_ruid, exp_euid;

	exp_ruid = getuid();
	exp_euid = geteuid();

	TST_EXP_PASS(getresuid(&real_uid, &eff_uid, &sav_uid));

	if (!TST_PASS)
		return;

	if (real_uid != exp_ruid || eff_uid != exp_euid ||
	    sav_uid != exp_euid) {
		tst_res(TFAIL,
			"got ruid=%d euid=%d suid=%d, expected ruid=%d euid=%d suid=%d",
			real_uid, eff_uid, sav_uid,
			exp_ruid, exp_euid, exp_euid);
	} else {
		tst_res(TPASS, "getresuid returned correct values");
	}
}

static struct tst_test test = {
	.test_all = run,
};
