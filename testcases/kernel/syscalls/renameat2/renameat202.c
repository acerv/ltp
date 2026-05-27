// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Cedric Hnyda <chnyda@suse.com>
 */

/*\
 * Verify that :manpage:`renameat2(2)` with RENAME_EXCHANGE swaps the
 * contents of two files. After the call, the file that previously had
 * content should now be empty and vice versa.
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "lapi/fcntl.h"
#include "renameat2.h"

#define TEST_DIR "test_dir/"
#define TEST_DIR2 "test_dir2/"

#define TEST_FILE "test_file"
#define TEST_FILE2 "test_file2"

static int olddirfd = -1;
static int newdirfd = -1;

static const char content[] = "content";

static void setup(void)
{
	SAFE_MKDIR(TEST_DIR, 0700);
	SAFE_MKDIR(TEST_DIR2, 0700);

	SAFE_TOUCH(TEST_DIR TEST_FILE, 0600, NULL);
	SAFE_TOUCH(TEST_DIR2 TEST_FILE2, 0600, NULL);

	olddirfd = SAFE_OPEN(TEST_DIR, O_DIRECTORY);
	newdirfd = SAFE_OPEN(TEST_DIR2, O_DIRECTORY);

	SAFE_FILE_PRINTF(TEST_DIR TEST_FILE, "%s", content);
}

static void run(void)
{
	char str[BUFSIZ] = { 0 };
	struct stat st;
	int fd, readn;

	TST_EXP_PASS(renameat2(olddirfd, TEST_FILE,
			newdirfd, TEST_FILE2, RENAME_EXCHANGE));
	if (!TST_PASS)
		return;

	/* After exchange: content should be in TEST_DIR2/TEST_FILE2 */
	fd = SAFE_OPEN(TEST_DIR2 TEST_FILE2, O_RDONLY);
	readn = SAFE_READ(0, fd, str, BUFSIZ);
	SAFE_CLOSE(fd);

	SAFE_STAT(TEST_DIR TEST_FILE, &st);

	if (readn != (int)(sizeof(content) - 1)) {
		tst_res(TFAIL, "wrong byte count: expected %zu, got %d",
			sizeof(content) - 1, readn);
		return;
	}

	if (strncmp(content, str, sizeof(content) - 1)) {
		tst_res(TFAIL, "content mismatch: expected '%s', got '%s'",
			content, str);
		return;
	}

	if (st.st_size) {
		tst_res(TFAIL, "swapped file has non-zero size");
		return;
	}

	tst_res(TPASS, "RENAME_EXCHANGE swapped contents correctly");

	/* Swap back for next iteration */
	SAFE_FILE_PRINTF(TEST_DIR TEST_FILE, "%s", content);
	SAFE_FILE_PRINTF(TEST_DIR2 TEST_FILE2, "%s", "");
}

static void cleanup(void)
{
	if (olddirfd != -1)
		SAFE_CLOSE(olddirfd);

	if (newdirfd != -1)
		SAFE_CLOSE(newdirfd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.skip_filesystems = (const char *const[]) {
		"btrfs",
		NULL,
	},
};
