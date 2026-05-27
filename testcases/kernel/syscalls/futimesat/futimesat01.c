// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2006
 * Copyright (c) 2016 Oracle and/or its affiliates. All Rights Reserved.
 * Author: Yi Yang <yyangcdl@cn.ibm.com>
 */

/*\
 * Verify basic functionality of :manpage:`futimesat(2)`.
 *
 * - futimesat() with a directory fd and relative path succeeds
 * - futimesat() with a directory fd and absolute path succeeds
 * - futimesat() with a regular file fd fails with ENOTDIR
 * - futimesat() with an invalid fd fails with EBADF
 * - futimesat() with AT_FDCWD and relative path succeeds
 */

#define _GNU_SOURCE
#include <sys/time.h>
#include "tst_test.h"
#include "lapi/syscalls.h"

#define TESTDIR		"futimesattestdir"
#define TESTFILE	"futimesattestfile.txt"
#define TESTFILE_INDIR	TESTDIR "/" TESTFILE

static int dir_fd = -1;
static int file_fd = -1;
static char *abs_path;

static void run(void)
{
	struct timeval times[2];

	gettimeofday(&times[0], NULL);
	gettimeofday(&times[1], NULL);

	TST_EXP_PASS(tst_syscall(__NR_futimesat, dir_fd, TESTFILE, times),
			"dir fd, relative path");

	TST_EXP_PASS(tst_syscall(__NR_futimesat, dir_fd, abs_path, times),
			"dir fd, absolute path");

	TST_EXP_FAIL(tst_syscall(__NR_futimesat, file_fd, TESTFILE, times),
			ENOTDIR, "regular file fd");

	TST_EXP_FAIL(tst_syscall(__NR_futimesat, 100, TESTFILE, times),
			EBADF, "invalid fd");

	TST_EXP_PASS(tst_syscall(__NR_futimesat, AT_FDCWD, TESTFILE, times),
			"AT_FDCWD");
}

static void setup(void)
{
	abs_path = tst_tmpdir_genpath("futimesatfile3.txt");

	SAFE_MKDIR(TESTDIR, 0700);
	SAFE_FILE_PRINTF(TESTFILE, TESTFILE);
	SAFE_FILE_PRINTF(TESTFILE_INDIR, TESTFILE_INDIR);

	dir_fd = SAFE_OPEN(TESTDIR, O_DIRECTORY);
	file_fd = SAFE_OPEN(abs_path, O_CREAT | O_RDWR, 0600);
}

static void cleanup(void)
{
	if (dir_fd != -1)
		SAFE_CLOSE(dir_fd);
	if (file_fd != -1)
		SAFE_CLOSE(file_fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
