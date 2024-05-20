// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that fchmodat2() syscall is properly working with
 * AT_SYMLINK_NOFOLLOW on regular files.
 */

#include "fchmodat2.h"
#include "lapi/fcntl.h"

#define MNTPOINT "mntpoint"
#define FNAME "myfile"

static int fd_dir = -1;

static void run(void)
{
	SAFE_CHMOD(MNTPOINT"/"FNAME, 0640);

	TST_EXP_PASS(fchmodat2(fd_dir, FNAME, 0700, AT_SYMLINK_NOFOLLOW));
	verify_mode(fd_dir, FNAME, S_IFREG | 0700);
}

static void setup(void)
{
	fd_dir = SAFE_OPEN(MNTPOINT, O_PATH | O_DIRECTORY, 0640);

	SAFE_TOUCH(MNTPOINT"/"FNAME, 0640, NULL);
}

static void cleanup(void)
{
	if (fd_dir != -1)
		SAFE_CLOSE(fd_dir);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.min_kver = "6.6",
	.mntpoint = MNTPOINT,
	.format_device = 1,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const []) {
		"fuse",
		NULL
	},
};
