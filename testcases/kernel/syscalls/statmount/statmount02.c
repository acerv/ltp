// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/**
 * [Description]
 *
 * This test verifies that statmount() is correctly reading basic filesystem
 * info using STATMOUNT_SB_BASIC.
 * The btrfs validation is currently skipped due to the lack of support for VFS.
 *
 * [Algorithm]
 *
 * * create a mount point and read its mount info
 * * run statmount() on the mount point using STATMOUNT_SB_BASIC
 * * read results and check if mount info are correct
 */

#include "statmount.h"
#include "lapi/stat.h"
#include "lapi/sched.h"
#include <linux/btrfs.h>

#define MNTPOINT "mntpoint"

static struct statmount *st_mount;
static struct statx *sx_mount;
static struct statfs *sf_mount;

static void run(void)
{
	memset(st_mount, 0, sizeof(struct statmount));

	TST_EXP_PASS(statmount(
		sx_mount->stx_mnt_id,
		STATMOUNT_SB_BASIC,
		st_mount,
		sizeof(struct statmount),
		0));

	if (TST_RET == -1)
		return;

	TST_EXP_EQ_LI(st_mount->mask, STATMOUNT_SB_BASIC);
	TST_EXP_EQ_LI(st_mount->size, sizeof(struct statmount));
	TST_EXP_EQ_LI(st_mount->sb_dev_major, sx_mount->stx_dev_major);
	TST_EXP_EQ_LI(st_mount->sb_dev_minor, sx_mount->stx_dev_minor);
	TST_EXP_EQ_LI(st_mount->sb_magic, sf_mount->f_type);
}

static void setup(void)
{
	SAFE_STATX(AT_FDCWD, MNTPOINT, 0, STATX_MNT_ID_UNIQUE, sx_mount);
	SAFE_STATFS(MNTPOINT, sf_mount);
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
		"btrfs",
		NULL
	},
	.bufs = (struct tst_buffers []) {
		{&st_mount, .size = sizeof(struct statmount)},
		{&sx_mount, .size = sizeof(struct statx)},
		{&sf_mount, .size = sizeof(struct statfs)},
		{}
	}
};
