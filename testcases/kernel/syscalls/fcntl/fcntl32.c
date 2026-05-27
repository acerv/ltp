// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Fujitsu Ltd.
 * Author: Guangwen Feng <fenggw-fnst@cn.fujitsu.com>
 */

/*\
 * Verify that :manpage:`fcntl(2)` with F_SETLEASE and F_WRLCK fails with
 * EAGAIN or EBUSY when another open file descriptor exists for the file.
 *
 * A write lease may be placed on a file only if there are no other open
 * file descriptors for the file.
 */

#include <errno.h>

#include "tst_test.h"

#define FILE_MODE	0777

static int fd1 = -1;
static int fd2 = -1;

static struct test_case_t {
	int fd1_flag;
	int fd2_flag;
} test_cases[] = {
	{O_RDONLY, O_RDONLY},
	{O_RDONLY, O_WRONLY},
	{O_RDONLY, O_RDWR},
	{O_WRONLY, O_RDONLY},
	{O_WRONLY, O_WRONLY},
	{O_WRONLY, O_RDWR},
	{O_RDWR, O_RDONLY},
	{O_RDWR, O_WRONLY},
	{O_RDWR, O_RDWR},
};

static void setup(void)
{
	SAFE_TOUCH("file", FILE_MODE, NULL);
}

static void verify_fcntl(unsigned int i)
{
	fd1 = SAFE_OPEN("file", test_cases[i].fd1_flag);
	fd2 = SAFE_OPEN("file", test_cases[i].fd2_flag);

	static const int exp_errs[] = {EAGAIN, EBUSY};

	TST_EXP_FAIL_ARR(fcntl(fd1, F_SETLEASE, F_WRLCK),
			exp_errs, ARRAY_SIZE(exp_errs),
			"fcntl(F_SETLEASE, F_WRLCK)");

	SAFE_CLOSE(fd1);
	SAFE_CLOSE(fd2);
}

static void cleanup(void)
{
	if (fd1 != -1)
		SAFE_CLOSE(fd1);

	if (fd2 != -1)
		SAFE_CLOSE(fd2);
}

static struct tst_test test = {
	.setup = setup,
	.test = verify_fcntl,
	.tcnt = ARRAY_SIZE(test_cases),
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.skip_filesystems = (const char *const []) {
		"nfs",
		"ramfs",
		"tmpfs",
		NULL
	},
};
