// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2006
 * Author: Yi Yang <yyangcdl@cn.ibm.com>
 * Copyright (c) Cyril Hrubis 2014 <chrubis@suse.cz>
 */

/*\
 * Verify basic :manpage:`mknodat(2)` functionality with various dirfd
 * and pathname combinations, including success and expected error
 * conditions (ENOTDIR, EBADF).
 */

#define _GNU_SOURCE

#include <sys/stat.h>

#include "tst_test.h"
#include "tst_safe_stdio.h"
#include "lapi/fcntl.h"

#define PATHNAME "mknodattestdir"

static int dir_fd = -1;
static int fd = -1;
static int fd_invalid = 100;
static int fd_atcwd = AT_FDCWD;

static char *abs_path;

static struct tcase {
	int *dir_fd;
	const char *name;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{&dir_fd, "testfile", 0, "relative path with dir fd"},
	{&dir_fd, NULL, 0, "absolute path with dir fd"},
	{&fd, "testfile2", ENOTDIR, "non-directory fd"},
	{&fd_invalid, "testfile", EBADF, "invalid fd"},
	{&fd_atcwd, "testfile", 0, "AT_FDCWD"},
};

static void setup(void)
{
	SAFE_MKDIR(PATHNAME, 0700);

	dir_fd = SAFE_OPEN(PATHNAME, O_DIRECTORY);
	fd = SAFE_OPEN("testfile2", O_CREAT | O_RDWR, 0600);

	SAFE_ASPRINTF(&abs_path, "%s/" PATHNAME "/testfile3", tst_tmpdir_path());

	tcases[1].name = abs_path;
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	if (tc->exp_errno) {
		TST_EXP_FAIL(mknodat(*tc->dir_fd, tc->name, S_IFREG, 0),
				tc->exp_errno, "%s", tc->desc);
	} else {
		TST_EXP_PASS(mknodat(*tc->dir_fd, tc->name, S_IFREG, 0),
				"%s", tc->desc);
	}

	if (TST_PASS && !tc->exp_errno) {
		if (tc->dir_fd == &dir_fd && tc->name != abs_path)
			SAFE_UNLINK(PATHNAME "/testfile");
		else if (tc->name == abs_path)
			SAFE_UNLINK(abs_path);
		else
			SAFE_UNLINK("testfile");
	}
}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);

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
