// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that lsm_get_self_attr syscall is raising errors when invalid data is
 * provided.
 */

#include "tst_test.h"
#include "lapi/lsm.h"

static struct lsm_ctx *ctx;
static uint32_t ctx_size;
static uint32_t ctx_size_small;

static struct tcase {
	uint32_t attr;
	struct lsm_ctx **ctx;
	uint32_t *size;
	uint32_t flags;
	int exp_err;
	char *msg;
} tcases[] = {
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx,
		.exp_err = EINVAL,
		.msg = "size is NULL",
	},
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx,
		.size = &ctx_size,
		.flags = LSM_FLAG_SINGLE | (LSM_FLAG_SINGLE << 1),
		.exp_err = EINVAL,
		.msg = "flags is invalid",
	},
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx,
		.size = &ctx_size_small,
		.exp_err = E2BIG,
		.msg = "size is too smal",
	},
	{
		.attr = LSM_ATTR_CURRENT,
		.ctx = &ctx,
		.size = &ctx_size,
		.flags = LSM_FLAG_SINGLE,
		.exp_err = EINVAL,
		.msg = "flags force to use ctx attributes",
	},
};

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	memset(ctx, 0, sizeof(struct lsm_ctx));
	ctx_size = sizeof(struct lsm_ctx);
	ctx_size_small = 1;

	TST_EXP_FAIL(lsm_get_self_attr(
		LSM_ATTR_CURRENT, *tc->ctx, tc->size, tc->flags),
		tc->exp_err,
		"%s", tc->msg);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.bufs = (struct tst_buffers[]) {
		{&ctx, .size = sizeof(struct lsm_ctx)},
		{}
	},
};
