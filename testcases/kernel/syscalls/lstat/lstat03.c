// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 *    Author: David Fenner, Jon Hendrickson
 * Copyright (C) 2024 Andrea Cervesato andrea.cervesato@suse.com
 */

/*\
 * [Description]
 *
 * This test checks that lstat() executed on file provide the same information
 * of symlink linking to it.
 */

#include <stdlib.h>
#include "tst_test.h"

#define FILENAME "myfile.bin"

static void run(void)
{
	char *symname = "my_symlink0";

	SAFE_SYMLINK(FILENAME, symname);

	struct stat path;
	struct stat link;

	TST_EXP_PASS(lstat(FILENAME, &path));
	TST_EXP_PASS(lstat(symname, &link));

	TST_EXP_EQ_LI(path.st_dev, link.st_dev);
	TST_EXP_EQ_LI(path.st_nlink, link.st_nlink);
	TST_EXP_EQ_LI(path.st_uid, link.st_uid);
	TST_EXP_EQ_LI(path.st_gid, link.st_gid);
	TST_EXP_EQ_LI(path.st_rdev, link.st_rdev);
	TST_EXP_EQ_LI(path.st_blksize, link.st_blksize);
	TST_EXP_EXPR(path.st_ino != link.st_ino, "path.st_ino != link.st_ino");
	TST_EXP_EXPR(path.st_mode != link.st_mode, "path.st_mode != link.st_mode");
	TST_EXP_EXPR(path.st_size != link.st_size, "path.st_size != link.st_size");
	TST_EXP_EXPR(path.st_blocks != link.st_blocks, "path.st_blocks != link.st_blocks");

	TST_EXP_EQ_LI(path.st_atime, link.st_atime);
	TST_EXP_EQ_LI(path.st_mtime, link.st_mtime);
	TST_EXP_EQ_LI(path.st_ctime, link.st_ctime);

	//TST_EXP_EXPR(path.st_atime != link.st_atime, "path.st_atime != link.st_atime");
	//TST_EXP_EXPR(path.st_mtime != link.st_mtime, "path.st_mtime != link.st_mtime");
	//TST_EXP_EXPR(path.st_ctime != link.st_ctime, "path.st_ctime != link.st_ctime");

	SAFE_UNLINK(symname);
}

static void setup(void)
{
	int fd;

	SAFE_TOUCH(FILENAME, 0777, NULL);

	fd = SAFE_OPEN(FILENAME, O_WRONLY, 0777);
	tst_fill_fd(fd, 'a', TST_KB, 500);
	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_tmpdir = 1,
};
