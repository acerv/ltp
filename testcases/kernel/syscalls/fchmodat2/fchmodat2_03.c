// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that fchmodat2() syscall is properly working with
 * AT_EMPTY_PATH.
 */

#include "fchmodat2.h"
#include "lapi/fcntl.h"

#define MNTPOINT "mntpoint"
#define DNAME MNTPOINT "/mydir"

static void run(void)
{
	int fd;
	struct stat st;

	SAFE_MKDIR(DNAME, 0640);
	fd = SAFE_OPEN(DNAME, O_PATH | O_DIRECTORY, 0640);

	TST_EXP_PASS(fchmodat2(fd, "", 0777, AT_EMPTY_PATH));

	SAFE_FSTAT(fd, &st);
	TST_EXP_EQ_LI(st.st_mode, S_IFDIR | 0777);

	SAFE_CLOSE(fd);
	SAFE_RMDIR(DNAME);
}

static struct tst_test test = {
	.test_all = run,
	.min_kver = "6.6",
	.mntpoint = MNTPOINT,
	.format_device = 1,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const []) {
		"fuse",
		NULL
	},
};
