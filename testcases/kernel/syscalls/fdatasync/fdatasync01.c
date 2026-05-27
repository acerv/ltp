// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Wipro Technologies Ltd, 2002
 */

/*\
 * Verify that :manpage:`fdatasync(2)` succeeds on a file opened for writing.
 */

#include "tst_test.h"

static int fd = -1;

static void setup(void)
{
	fd = SAFE_OPEN("testfile", O_CREAT | O_WRONLY, 0777);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, "test data", 9);
}

static void run(void)
{
	TST_EXP_PASS(fdatasync(fd));
}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
