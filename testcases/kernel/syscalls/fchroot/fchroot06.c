// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test path walks under the failfs root.
 *
 * Once fchroot() moved the process root into failfs, only lookups
 * anchored at a file descriptor keep working:
 *
 * - lookups relative to the working directory, which stays in the real
 *   filesystem, keep working
 * - lookups anchored at a pre-opened directory fd keep working, including
 *   resolution of relative symlinks
 * - absolute symlinks restart the walk at the failfs root and fail with
 *   EOPNOTSUPP
 * - ".." walks clamp at the top of the mount tree, not at the failfs
 *   root, so walking up from the working directory lands on the real
 *   root
 *
 * The test runs in a forked child so the root of the parent process is
 * left untouched.
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/stat.h>
#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/syscalls.h"
#include "tst_safe_file_at.h"

#define UPWARDS "../../../../../../../../../.."

static void run(void)
{
	if (!SAFE_FORK()) {
		struct stat realroot, st;
		int dfd, fd;

		SAFE_STAT("/", &realroot);
		dfd = SAFE_OPEN(".", O_RDONLY | O_DIRECTORY);

		TST_EXP_PASS(tst_syscall(__NR_fchroot, FD_FAILFS_ROOT, 0),
			"fchroot() with the FD_FAILFS_ROOT sentinel");

		fd = SAFE_OPENAT(AT_FDCWD, ".", O_RDONLY | O_DIRECTORY);
		SAFE_CLOSE(fd);

		fd = SAFE_OPENAT(dfd, "canary", O_WRONLY | O_CREAT, 0600);
		SAFE_WRITE(SAFE_WRITE_ALL, fd, "x", 1);
		SAFE_CLOSE(fd);

		fd = SAFE_OPENAT(dfd, "rel", O_RDONLY);
		SAFE_CLOSE(fd);

		TST_EXP_FAIL2(openat(dfd, "abs", O_RDONLY), EOPNOTSUPP,
			"resolution of an absolute symlink");

		fd = SAFE_OPENAT(AT_FDCWD, UPWARDS, O_PATH);
		SAFE_FSTAT(fd, &st);
		TST_EXP_EXPR(st.st_dev == realroot.st_dev &&
			st.st_ino == realroot.st_ino,
			"'..' walk clamps at the top of the mount tree");
		SAFE_CLOSE(fd);

		SAFE_CLOSE(dfd);

		exit(0);
	}
}

static void setup(void)
{
	SAFE_TOUCH("target", 0644, NULL);
	SAFE_SYMLINK("target", "rel");
	SAFE_SYMLINK("/etc", "abs");
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.forks_child = 1,
};
