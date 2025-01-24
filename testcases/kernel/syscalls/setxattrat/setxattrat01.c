// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test if setxattrat() syscall is correctly following symlink, setting a
 * xattr on a file.
 *
 * [Algorithm]
 *
 * - create a file and the symlink pointing to it
 * - run setxattrat() on the symlink following the pointing file
 * - verify that file xattr has been set
 * - verify that symlink xattr has not been set
 * - run setxattrat() on the symlink with AT_SYMLINK_NOFOLLOW
 * - verify that file xattr has not been set
 * - verify that symlink xattr has been set
 */

#include <sys/xattr.h>

#include "tst_test.h"
#include "lapi/xattr.h"
#include "lapi/fcntl.h"

#define FNAME "ltp_file"
#define SNAME "ltp_symbolic_file"
#define XATTR_TEST_KEY "user.testkey"
#define XATTR_TEST_VALUE "ltprulez"
#define XATTR_TEST_VALUE_SIZE 8

static struct xattr_args *args;
static int tmpdir_fd = -1;

static struct tcase {
	char *dst_set;
	char *dst_noset;
	int at_flags;
} tcases[] = {
	{
		.dst_set = FNAME,
		.dst_noset = SNAME,
		.at_flags = 0,
	},
	{
		.dst_set = SNAME,
		.dst_noset = FNAME,
		.at_flags = AT_SYMLINK_NOFOLLOW,
	}
};

static void expect_xattr(const char *fname)
{
	int ret;
	char buff[XATTR_TEST_VALUE_SIZE];

	tst_res(TINFO, "Check if %s has xattr", fname);

	memset(args, 0, sizeof(*args));
	memset(buff, 0, XATTR_TEST_VALUE_SIZE);

	args->value = buff;
	args->size = XATTR_TEST_VALUE_SIZE;

	ret = SAFE_GETXATTRAT(tmpdir_fd, fname, AT_SYMLINK_NOFOLLOW,
		XATTR_TEST_KEY, args, sizeof(*args));

	TST_EXP_EQ_LI(ret, XATTR_TEST_VALUE_SIZE);
	TST_EXP_EQ_LI(args->size, XATTR_TEST_VALUE_SIZE);
	TST_EXP_EQ_LI(args->flags, 0);
	TST_EXP_EQ_STRN(args->value, XATTR_TEST_VALUE, XATTR_TEST_VALUE_SIZE);
}

static void expect_no_xattr(const char *fname)
{
	char buff[XATTR_TEST_VALUE_SIZE];

	tst_res(TINFO, "Check if %s has no xattr", fname);

	memset(args, 0, sizeof(*args));
	memset(buff, 0, XATTR_TEST_VALUE_SIZE);

	args->value = buff;
	args->size = 0;

	TST_EXP_FAIL(tst_syscall(__NR_getxattrat, tmpdir_fd, fname,
		AT_SYMLINK_NOFOLLOW, XATTR_TEST_KEY, args, sizeof(*args)),
		ENODATA);

	TST_EXP_EQ_LI(args->size, 0);
	TST_EXP_EQ_LI(args->flags, 0);
	TST_EXP_EQ_STRN(args->value, "\0", 1);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	args->value = XATTR_TEST_VALUE;
	args->size = XATTR_TEST_VALUE_SIZE;
	args->flags = XATTR_CREATE;

	tst_res(TINFO, "Setting xattr '%s' in %s (flags=%s)",
		XATTR_TEST_KEY, SNAME,
		!tc->at_flags ? "0" : "AT_SYMLINK_NOFOLLOW");

	SAFE_SETXATTRAT(tmpdir_fd, SNAME, tc->at_flags, XATTR_TEST_KEY,
		 args, sizeof(*args));

	expect_xattr(tc->dst_set);
	expect_no_xattr(tc->dst_noset);

	SAFE_REMOVEXATTR(tc->dst_set, XATTR_TEST_KEY);
}

static void setup(void)
{
	char *tmpdir;

	tmpdir = tst_tmpdir_path();
	tmpdir_fd = SAFE_OPEN(tmpdir, O_DIRECTORY);

	SAFE_TOUCH(FNAME, 0777, NULL);
	SAFE_SYMLINK(FNAME, SNAME);
}

static void cleanup(void)
{
	if (tmpdir_fd != -1)
		SAFE_CLOSE(tmpdir_fd);

	SAFE_UNLINK(SNAME);
	SAFE_UNLINK(FNAME);
}

static struct tst_test test = {
	.test = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.needs_root = 1,
	.tcnt = ARRAY_SIZE(tcases),
	.bufs = (struct tst_buffers []) {
		{&args, .size = sizeof(struct xattr_args)},
		{},
	}
};
