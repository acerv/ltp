// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE Wei Gao <wegao@suse.com>
 */

/*\
 * Basic test for fcntl using F_CREATED_QUERY.
 * Verify if the fcntl() syscall is recognizing whether a file has been
 * created or not via O_CREAT when O_CLOEXEC is also used.
 *
 * Test is based on a kernel selftests.
 */

#include "lapi/fcntl.h"
#include "tst_test.h"

#define TEST_NAME "LTP_FCNTL_CREATED_QUERY_TEST"

static void verify_fcntl(void)
{
	int fd;

	fd = SAFE_OPEN("/dev/null", O_RDONLY | O_CLOEXEC);

	/* We didn't create "/dev/null". */
	SAFE_FCNTL(fd, F_CREATED_QUERY, 0);
	SAFE_CLOSE(fd);

	fd = SAFE_OPEN(TEST_NAME, O_CREAT | O_RDONLY | O_CLOEXEC, 0600);

	SAFE_FCNTL(fd, F_CREATED_QUERY, 0);
	SAFE_CLOSE(fd);

	fd = SAFE_OPEN(TEST_NAME, O_RDONLY | O_CLOEXEC);

	/* We're opening it again, so no positive creation check. */
	SAFE_FCNTL(fd, F_CREATED_QUERY, 0);
	SAFE_CLOSE(fd);
	SAFE_UNLINK(TEST_NAME);

	tst_res(TPASS, "fcntl F_CREATED_QUERY check pass");
}

static struct tst_test test = {
	.test_all = verify_fcntl,
	.needs_tmpdir = 1,
	.min_kver = "6.12",
	.tags = (const struct tst_tag[]) {
		{"linux-git", "d0fe8920cbe4"},
		{}
	}
};
