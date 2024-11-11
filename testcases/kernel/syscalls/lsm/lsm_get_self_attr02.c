// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that lsm_get_self_attr syscall is acting correctly when ctx is NULL.
 * The syscall can behave in different ways according to the current system
 * status:
 *
 * - if any LSM is running inside the system, the syscall will pass and it will
 *   provide a size as big as the attribute
 * - if no LSM(s) are running inside the system, the syscall will fail with -1
 *   return code
 */
#include "lsm_common.h"

static uint32_t page_size;
static uint32_t lsm_count;

static void run(void)
{
	uint32_t size = page_size;

	if (lsm_count) {
		TST_EXP_EXPR(lsm_get_self_attr(
			LSM_ATTR_CURRENT, NULL, &size, 0) >= 1);
		TST_EXP_EXPR(size > 1);
	} else {
		TST_EXP_EQ_LI(lsm_get_self_attr(
			LSM_ATTR_CURRENT, NULL, &size, 0), -1);
	}
}

static void setup(void)
{
	page_size = SAFE_SYSCONF(_SC_PAGESIZE);

	if (verify_enabled_lsm("selinux"))
		lsm_count++;

	if (verify_enabled_lsm("apparmor"))
		lsm_count++;

	if (verify_enabled_lsm("smack"))
		lsm_count++;
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
};
