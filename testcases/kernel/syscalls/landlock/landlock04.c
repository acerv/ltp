// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that all landlock rules are working properly. The way we
 * do it is to verify that all disabled syscalls are not working but the one we
 * enabled via specifc landlock rules.
 */

#include "landlock_common.h"
#include "landlock_tester.h"

#define ACCESS_NAME(x) #x

static struct landlock_ruleset_attr *ruleset_attr;
static struct landlock_path_beneath_attr *path_beneath_attr;

static struct tvariant {
	int access;
	char *desc;
} tvariants[] = {
	{
		LANDLOCK_ACCESS_FS_EXECUTE,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_EXECUTE)
	},
	{
		LANDLOCK_ACCESS_FS_WRITE_FILE,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_WRITE_FILE)
	},
	{
		LANDLOCK_ACCESS_FS_READ_FILE,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_READ_FILE)
	},
	{
		LANDLOCK_ACCESS_FS_READ_DIR,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_READ_DIR)
	},
	{
		LANDLOCK_ACCESS_FS_REMOVE_DIR,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_REMOVE_DIR)
	},
	{
		LANDLOCK_ACCESS_FS_REMOVE_FILE,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_REMOVE_FILE)
	},
	{
		LANDLOCK_ACCESS_FS_MAKE_CHAR,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_MAKE_CHAR)
	},
	{
		LANDLOCK_ACCESS_FS_MAKE_BLOCK,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_MAKE_BLOCK)
	},
	{
		LANDLOCK_ACCESS_FS_MAKE_REG,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_MAKE_REG)
	},
	{
		LANDLOCK_ACCESS_FS_MAKE_SOCK,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_MAKE_SOCK)
	},
	{
		LANDLOCK_ACCESS_FS_MAKE_FIFO,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_MAKE_FIFO)
	},
	{
		LANDLOCK_ACCESS_FS_MAKE_SYM,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_MAKE_SYM)
	},
	{
		LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_TRUNCATE,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_TRUNCATE)
	},
	{
		LANDLOCK_ACCESS_FS_READ_FILE |
			LANDLOCK_ACCESS_FS_WRITE_FILE |
			LANDLOCK_ACCESS_FS_IOCTL_DEV,
		ACCESS_NAME(LANDLOCK_ACCESS_FS_MAKE_SYM)
	},
};

static void run(void)
{
	if (!SAFE_FORK()) {
		struct tvariant  variant = tvariants[tst_variant];

		tester_run_all_rules(variant.access);
		_exit(0);
	}
}

static void setup(void)
{
	struct tvariant variant = tvariants[tst_variant];

	verify_landlock_is_enabled();
	tester_create_tree();

	tst_res(TINFO, "Testing %s", variant.desc);

	ruleset_attr->handled_access_fs = tester_get_all_rules();

	apply_landlock_layer(
		ruleset_attr,
		path_beneath_attr,
		SANDBOX_FOLDER,
		variant.access);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.forks_child = 1,
	.needs_tmpdir = 1,
	.needs_root = 1,
	.test_variants = ARRAY_SIZE(tvariants),
	.resource_files = (const char *[]) {
		TESTAPP,
		NULL,
	},
	.needs_kconfigs = (const char *[]) {
		"CONFIG_SECURITY_LANDLOCK=y",
		NULL
	},
	.bufs = (struct tst_buffers []) {
		{&ruleset_attr, .size = sizeof(struct landlock_ruleset_attr)},
		{&path_beneath_attr, .size = sizeof(struct landlock_path_beneath_attr)},
		{},
	},
	.caps = (struct tst_cap []) {
		TST_CAP(TST_CAP_REQ, CAP_SYS_ADMIN),
		TST_CAP(TST_CAP_REQ, CAP_MKNOD),
		{}
	},
	.format_device = 1,
	.mount_device = 1,
	.mntpoint = SANDBOX_FOLDER,
	.all_filesystems = 1,
	.skip_filesystems = (const char *[]) {
		"vfat",
		NULL
	},
};
