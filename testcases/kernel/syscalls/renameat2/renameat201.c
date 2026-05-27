// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Cedric Hnyda <chnyda@suse.com>
 */

/*\
 * Verify that :manpage:`renameat2(2)` handles various flag combinations
 * correctly:
 *
 * - RENAME_NOREPLACE fails with EEXIST when target exists
 * - RENAME_EXCHANGE succeeds when both paths exist
 * - RENAME_EXCHANGE fails with ENOENT when target is missing
 * - RENAME_NOREPLACE succeeds when target does not exist
 * - RENAME_NOREPLACE | RENAME_EXCHANGE fails with EINVAL
 * - RENAME_WHITEOUT | RENAME_EXCHANGE fails with EINVAL
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "lapi/fcntl.h"
#include "renameat2.h"

#define TEST_DIR "test_dir/"
#define TEST_DIR2 "test_dir2/"

#define TEST_FILE "test_file"
#define TEST_FILE2 "test_file2"
#define TEST_FILE3 "test_file3"
#define NON_EXIST "non_exist"

static int olddirfd = -1;
static int newdirfd = -1;

static struct tcase {
	int *olddirfd;
	const char *oldpath;
	int *newdirfd;
	const char *newpath;
	int flags;
	int exp_errno;
} tcases[] = {
	{&olddirfd, TEST_FILE, &newdirfd, TEST_FILE2, RENAME_NOREPLACE, EEXIST},
	{&olddirfd, TEST_FILE, &newdirfd, TEST_FILE2, RENAME_EXCHANGE, 0},
	{&olddirfd, TEST_FILE, &newdirfd, NON_EXIST, RENAME_EXCHANGE, ENOENT},
	{&olddirfd, TEST_FILE, &newdirfd, TEST_FILE3, RENAME_NOREPLACE, 0},
	{&olddirfd, TEST_FILE, &newdirfd, TEST_FILE2,
		RENAME_NOREPLACE | RENAME_EXCHANGE, EINVAL},
	{&olddirfd, TEST_FILE, &newdirfd, TEST_FILE2,
		RENAME_WHITEOUT | RENAME_EXCHANGE, EINVAL},
};

static void setup(void)
{
	SAFE_MKDIR(TEST_DIR, 0700);
	SAFE_MKDIR(TEST_DIR2, 0700);

	olddirfd = SAFE_OPEN(TEST_DIR, O_DIRECTORY);
	newdirfd = SAFE_OPEN(TEST_DIR2, O_DIRECTORY);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	SAFE_TOUCH(TEST_DIR TEST_FILE, 0600, NULL);
	SAFE_TOUCH(TEST_DIR2 TEST_FILE2, 0600, NULL);
	SAFE_TOUCH(TEST_DIR TEST_FILE3, 0600, NULL);

	if (tc->exp_errno) {
		TST_EXP_FAIL(renameat2(*tc->olddirfd, tc->oldpath,
				*tc->newdirfd, tc->newpath, tc->flags),
				tc->exp_errno);
	} else {
		TST_EXP_PASS(renameat2(*tc->olddirfd, tc->oldpath,
				*tc->newdirfd, tc->newpath, tc->flags));
	}

	/* Reset files for next test case */
	unlink(TEST_DIR TEST_FILE);
	unlink(TEST_DIR2 TEST_FILE2);
	unlink(TEST_DIR TEST_FILE3);
	unlink(TEST_DIR2 TEST_FILE3);
	unlink(TEST_DIR2 TEST_FILE);
}

static void cleanup(void)
{
	if (olddirfd != -1)
		SAFE_CLOSE(olddirfd);

	if (newdirfd != -1)
		SAFE_CLOSE(newdirfd);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.skip_filesystems = (const char *const[]) {
		"btrfs",
		NULL,
	},
};
