// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Wei Gao <wegao@suse.com>
 */

/*\
 * Verify that mount will raise ENOENT if we try to mount on magic links
 * under /proc/<pid>/fd/<nr>.
 */

#include "tst_test.h"
#include <sys/mount.h>
#include "tst_safe_file_at.h"

#define MNTPOINT "mntpoint"
#define FOO MNTPOINT "/foo"
#define BAR MNTPOINT "/bar"

static void run(void)
{
	char path[PATH_MAX];
	int foo_fd, newfd, proc_fd;

	foo_fd = SAFE_OPEN(FOO, O_RDONLY | O_NONBLOCK, 0640);
	newfd = SAFE_DUP(foo_fd);
	SAFE_CLOSE(foo_fd);

	sprintf(path, "/proc/%d/fd/%d", getpid(), newfd);

	proc_fd = SAFE_OPENAT(AT_FDCWD, path, O_PATH | O_NOFOLLOW);

	sprintf(path, "/proc/%d/fd/%d", getpid(), proc_fd);

	TST_EXP_FAIL(
		mount(BAR, path, "", MS_BIND, 0),
		ENOENT,
		"mount() on proc failed expectedly"
	);
}

static void setup(void)
{
	SAFE_CREAT(FOO, 0777);
	SAFE_CREAT(BAR, 0777);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_root = 1,
	.mntpoint = MNTPOINT,
	.min_kver = "6.12",
	.tags = (const struct tst_tag[]) {
		{"linux-git", "d80b065bb172"},
		{}
	}
};
