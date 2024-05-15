// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/**
 * [Description]
 *
 * This test verifies that statmount() is correctly reading mount information
 * (mount id, parent mount id, mount attributes etc.) using STATMOUNT_MNT_BASIC.
 *
 * [Algorithm]
 *
 * * create a mount point
 * * create a new parent folder inside the mount point and obtain its mount info
 * * create the new "/" mount folder and obtain its mount info
 * * run statmount() on the mount point using STATMOUNT_MNT_BASIC
 * * read results and check if mount info are correct
 */

#include "statmount.h"
#include "lapi/stat.h"
#include "lapi/sched.h"

#define MNTPOINT "mntpoint"
#define MYPARENT MNTPOINT "/myroot"

static uint64_t root_id;
static uint64_t root_id_old;
static uint64_t parent_id;
static uint64_t parent_id_old;
static struct statmount *st_mount;

static void run(void)
{
	memset(st_mount, 0, sizeof(struct statmount));

	TST_EXP_PASS(statmount(
		root_id,
		STATMOUNT_MNT_BASIC,
		st_mount,
		sizeof(struct statmount),
		0));

	if (TST_RET == -1)
		return;

	TST_EXP_EQ_LI(st_mount->mask, STATMOUNT_MNT_BASIC);
	TST_EXP_EQ_LI(st_mount->size, sizeof(struct statmount));
	TST_EXP_EQ_LI(st_mount->mnt_id, root_id);
	TST_EXP_EQ_LI(st_mount->mnt_id_old, root_id_old);
	TST_EXP_EQ_LI(st_mount->mnt_parent_id, parent_id);
	TST_EXP_EQ_LI(st_mount->mnt_parent_id_old, parent_id_old);
	TST_EXP_EQ_LI(st_mount->mnt_propagation, MS_PRIVATE);
}

static void setup(void)
{
	struct statx sx;

	SAFE_UNSHARE(CLONE_NEWNS);
	SAFE_MKDIR(MYPARENT, 0700);

	SAFE_STATX(AT_FDCWD, MYPARENT, 0, STATX_MNT_ID_UNIQUE, &sx);
	parent_id = sx.stx_mnt_id;

	SAFE_STATX(AT_FDCWD, MYPARENT, 0, STATX_MNT_ID, &sx);
	parent_id_old = sx.stx_mnt_id;

	SAFE_MOUNT("", "/", NULL, MS_REC | MS_PRIVATE, NULL);
	SAFE_MOUNT(MYPARENT, MYPARENT, NULL, MS_BIND, NULL);
	SAFE_CHROOT(MYPARENT);
	SAFE_CHDIR("/");

	SAFE_STATX(AT_FDCWD, "/", 0, STATX_MNT_ID_UNIQUE, &sx);
	root_id = sx.stx_mnt_id;

	SAFE_STATX(AT_FDCWD, "/", 0, STATX_MNT_ID, &sx);
	root_id_old = sx.stx_mnt_id;
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.min_kver = "6.8",
	.needs_tmpdir = 1,
	.format_device = 1,
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
