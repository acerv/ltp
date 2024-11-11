// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that LSM_ATTR_CURRENT attribute is correctly recognizing
 * the current, active security context of the process. This is done by
 * checking that /proc/self/attr/current matches with the obtained value.
 */

#include "lsm_common.h"

static struct lsm_ctx *ctx;
static uint32_t page_size;

static void run(void)
{
	tst_res(TINFO, "Verifying 'LSM_ATTR_CURRENT' attribute");

	uint32_t count;
	uint32_t size = page_size;
	char attr[size];

	memset(attr, 0, size);
	memset(ctx, 0, sizeof(struct lsm_ctx));

	count = TST_EXP_POSITIVE(
		lsm_get_self_attr(LSM_ATTR_CURRENT, ctx, &size, 0));

	if (TST_RET == -1)
		return;

	if (!count) {
		tst_res(TFAIL, "Can't read any attribute");
		return;
	}

	read_proc_attr("current", attr, page_size);

	TST_EXP_EQ_STR(attr, (char *)ctx->ctx);

	struct lsm_ctx *next = ctx;

	for (uint32_t i = 1; i < count; i++) {
		TST_EXP_EXPR(strcmp(attr, (char *)next->ctx) != 0,
			"Attribute and next LSM context must be different");

		next = next_ctx(next);
	}
}

static void setup(void)
{
	uint32_t lsm_count = 0;

	if (verify_enabled_lsm("selinux"))
		lsm_count++;

	if (verify_enabled_lsm("apparmor"))
		lsm_count++;

	if (verify_enabled_lsm("smack"))
		lsm_count++;

	if (!lsm_count)
		tst_brk(TCONF, "LSM_ATTR_CURRENT is not supported by any LSM");

	page_size = SAFE_SYSCONF(_SC_PAGESIZE);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.bufs = (struct tst_buffers[]) {
		{&ctx, .size = sizeof(struct lsm_ctx)},
		{}
	},
};
