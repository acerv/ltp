// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2006
 *  Author: Yi Yang <yyangcdl@cn.ibm.com>
 * Copyright (c) Cyril Hrubis 2014 <chrubis@suse.cz>
 */

/*\
 * Verify basic function of :manpage:`mkdirat(2)`.
 *
 * - mkdirat() with a valid dirfd and relative pathname succeeds
 * - mkdirat() with a valid dirfd and absolute pathname succeeds
 * - mkdirat() with AT_FDCWD and relative pathname succeeds
 * - mkdirat() with a file descriptor fails with ENOTDIR
 * - mkdirat() with an invalid file descriptor fails with EBADF
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include "tst_test.h"

#define TEST_DIR "test_dir"
#define TEST_FILE "test_file"
#define REL_DIR "mkdirat_dir"
#define ABS_DIR "mkdirat_absdir"

static int dir_fd = -1;
static int fd = -1;
static int fd_invalid = 100;
static int fd_atcwd = AT_FDCWD;
static char abspath[PATH_MAX];

static struct tcase {
	int *dir_fd;
	const char *name;
	int exp_errno;
} tcases[] = {
	{&dir_fd, REL_DIR, 0},
	{&dir_fd, abspath, 0},
	{&fd_atcwd, REL_DIR "_atcwd", 0},
	{&fd, REL_DIR, ENOTDIR},
	{&fd_invalid, REL_DIR, EBADF},
};

static void verify_mkdirat(unsigned int i)
{
	struct tcase *tc = &tcases[i];
	char rmpath[PATH_MAX];

	if (tc->exp_errno == 0) {
		TST_EXP_PASS(mkdirat(*tc->dir_fd, tc->name, 0600));
		if (!TST_PASS)
			return;

		if (*tc->dir_fd == AT_FDCWD || tc->name[0] == '/') {
			SAFE_RMDIR(tc->name);
		} else {
			snprintf(rmpath, sizeof(rmpath), "%s/%s",
				 TEST_DIR, tc->name);
			SAFE_RMDIR(rmpath);
		}
	} else {
		TST_EXP_FAIL(mkdirat(*tc->dir_fd, tc->name, 0600),
				tc->exp_errno);
	}
}

static void setup(void)
{
	char tmpbuf[PATH_MAX];

	SAFE_MKDIR(TEST_DIR, 0700);
	dir_fd = SAFE_OPEN(TEST_DIR, O_DIRECTORY);
	fd = SAFE_OPEN(TEST_FILE, O_CREAT | O_RDWR, 0600);

	if (!realpath(TEST_DIR, tmpbuf))
		tst_brk(TBROK | TERRNO, "realpath() failed");

	snprintf(abspath, sizeof(abspath), "%s/%s", tmpbuf, ABS_DIR);
}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);
	if (dir_fd != -1)
		SAFE_CLOSE(dir_fd);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test = verify_mkdirat,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_tmpdir = 1,
};
