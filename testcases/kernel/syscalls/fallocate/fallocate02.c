// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 * Author: Sharyathi Nagesh <sharyathi@in.ibm.com>
 */

/*\
 * Verify that :manpage:`fallocate(2)` fails with the expected errno
 * for various invalid conditions:
 *
 * - EBADF when the file descriptor is read-only
 * - EINVAL when offset is negative
 * - EINVAL when length is negative or zero
 * - EFBIG when offset+length exceeds the maximum file size (64-bit only)
 */

#define _GNU_SOURCE

#include <limits.h>

#include "tst_test.h"
#include "lapi/fallocate.h"
#include "lapi/abisize.h"

#define BLOCKS_WRITTEN		12
#ifdef TEST_DEFAULT
# define DEFAULT_TEST_MODE	0
#else
# define DEFAULT_TEST_MODE	1
#endif
#define FNAMER			"test_file1"
#define FNAMEW			"test_file2"
#define OFFSET			12
#define BLOCK_SIZE		1024
#define MAX_FILESIZE		(LLONG_MAX / 1024)

static int fdw = -1;
static int fdr = -1;

static struct tcase {
	int *fd;
	int mode;
	loff_t offset;
	loff_t len;
	int exp_errno;
} tcases[] = {
	{&fdr, DEFAULT_TEST_MODE, 0, 1, EBADF},
	{&fdw, DEFAULT_TEST_MODE, -1, 1, EINVAL},
	{&fdw, DEFAULT_TEST_MODE, 1, -1, EINVAL},
	{&fdw, DEFAULT_TEST_MODE, BLOCKS_WRITTEN, 0, EINVAL},
	{&fdw, DEFAULT_TEST_MODE, BLOCKS_WRITTEN, -1, EINVAL},
	{&fdw, DEFAULT_TEST_MODE, -(BLOCKS_WRITTEN + OFFSET), 1, EINVAL},
#ifdef TST_ABI64
	{&fdw, DEFAULT_TEST_MODE, MAX_FILESIZE, 1, EFBIG},
	{&fdw, DEFAULT_TEST_MODE, 1, MAX_FILESIZE, EFBIG},
#endif
};

static void setup(void)
{
	int i;
	char buf[BLOCK_SIZE];

	fdr = SAFE_OPEN(FNAMER, O_RDONLY | O_CREAT, 0400);
	fdw = SAFE_OPEN(FNAMEW, O_RDWR | O_CREAT, 0700);

	memset(buf, 'A', BLOCK_SIZE);
	for (i = 0; i < BLOCKS_WRITTEN; i++)
		SAFE_WRITE(SAFE_WRITE_ALL, fdw, buf, BLOCK_SIZE);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	TEST(fallocate(*tc->fd, tc->mode, tc->offset * BLOCK_SIZE,
			tc->len * BLOCK_SIZE));

	if (EOPNOTSUPP == TST_ERR || ENOSYS == TST_ERR)
		tst_brk(TCONF, "fallocate not supported");

	if (TST_RET != -1) {
		tst_res(TFAIL, "fallocate() succeeded unexpectedly");
		return;
	}

	if (tc->exp_errno == TST_ERR) {
		tst_res(TPASS | TTERRNO, "fallocate() failed as expected");
	} else {
		tst_res(TFAIL | TTERRNO, "fallocate() failed, expected %s",
			tst_strerrno(tc->exp_errno));
	}
}

static void cleanup(void)
{
	if (fdw != -1)
		SAFE_CLOSE(fdw);

	if (fdr != -1)
		SAFE_CLOSE(fdr);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
