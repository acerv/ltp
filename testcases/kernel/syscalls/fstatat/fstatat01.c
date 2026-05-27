// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2016 Oracle and/or its affiliates. All Rights Reserved.
 * Copyright (c) International Business Machines Corp., 2006
 * Author: Yi Yang <yyangcdl@cn.ibm.com>
 */

/*\
 * Verify that :manpage:`fstatat(2)` handles various dirfd and flag
 * combinations correctly:
 *
 * - Valid directory fd with relative path succeeds
 * - Valid directory fd with absolute path succeeds
 * - Regular file fd as dirfd fails with ENOTDIR
 * - Invalid fd fails with EBADF
 * - Invalid flags value fails with EINVAL
 * - AT_FDCWD with relative path succeeds
 */

#define _GNU_SOURCE

#include <sys/stat.h>

#include "tst_test.h"
#include "tst_safe_stdio.h"
#include "lapi/fcntl.h"

#define TESTDIR		"fstatattestdir"
#define TESTFILE	"fstatattestfile.txt"
#define TESTFILE2	TESTDIR "/fstatattestfile.txt"

static int dir_fd = -1;
static int file_fd = -1;
static int bad_fd = 100;
static int atcwd_fd = AT_FDCWD;

static char *abs_path;

static struct tcase {
	int *fd;
	const char **path;
	int flags;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{&dir_fd, &(const char *){TESTFILE}, 0, 0, "dirfd + relative path"},
	{&dir_fd, NULL, 0, 0, "dirfd + absolute path"},
	{&file_fd, &(const char *){TESTFILE}, 0, ENOTDIR, "non-dir fd"},
	{&bad_fd, &(const char *){TESTFILE}, 0, EBADF, "bad fd"},
	{&dir_fd, &(const char *){TESTFILE}, 9999, EINVAL, "invalid flags"},
	{&atcwd_fd, &(const char *){TESTFILE}, 0, 0, "AT_FDCWD"},
};

static void setup(void)
{
	SAFE_MKDIR(TESTDIR, 0700);
	dir_fd = SAFE_OPEN(TESTDIR, O_DIRECTORY);

	SAFE_FILE_PRINTF(TESTFILE, TESTFILE);
	SAFE_FILE_PRINTF(TESTFILE2, TESTFILE2);

	SAFE_ASPRINTF(&abs_path, "%s/fstatattestfile3.txt", tst_tmpdir_path());
	file_fd = SAFE_OPEN(abs_path, O_CREAT | O_RDWR, 0600);

	tcases[1].path = (const char **)&abs_path;
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];
	struct stat statbuf;
	const char *path = tc->path ? *tc->path : TESTFILE;

	if (tc->exp_errno) {
		TST_EXP_FAIL(fstatat(*tc->fd, path, &statbuf, tc->flags),
				tc->exp_errno, "%s", tc->desc);
	} else {
		TST_EXP_PASS(fstatat(*tc->fd, path, &statbuf, tc->flags),
				"%s", tc->desc);
	}
}

static void cleanup(void)
{
	if (file_fd != -1)
		SAFE_CLOSE(file_fd);

	if (dir_fd != -1)
		SAFE_CLOSE(dir_fd);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
