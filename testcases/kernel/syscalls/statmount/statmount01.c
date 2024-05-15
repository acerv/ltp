// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/**
 * [Description]
 *
 * This test verifies that statmount() is working with no mask flags.
 *
 * [Algorithm]
 *
 * * create a mount point
 * * run statmount() on the mount point without giving any mask
 * * read results and check that mask and size are correct
 */

#include "statmount.h"
#include "lapi/stat.h"
#include "lapi/sched.h"

#define MNTPOINT "mntpoint"

static uint64_t root_id;
static struct statmount *st_mount;

static void run(void)
{
	memset(st_mount, 0, sizeof(struct statmount));

	TST_EXP_PASS(statmount(
		root_id,
		0,
		st_mount,
		sizeof(struct statmount),
		0));

	if (TST_RET == -1)
		return;

	TST_EXP_EQ_LI(st_mount->mask, 0);
	TST_EXP_EQ_LI(st_mount->size, sizeof(struct statmount));
}

static void setup(void)
{
	struct statx sx;

	SAFE_STATX(AT_FDCWD, MNTPOINT, 0, STATX_MNT_ID_UNIQUE, &sx);

	root_id = sx.stx_mnt_id;
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.min_kver = "6.8",
	.mount_device = 1,
	.mntpoint = MNTPOINT,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const []) {
		"fuse",
		NULL
	},
	.bufs = (struct tst_buffers []) {
		{&st_mount, .size = sizeof(struct statmount)},
		{}
	}
};
