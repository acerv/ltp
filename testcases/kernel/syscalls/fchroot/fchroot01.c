// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test that fchroot() with a regular directory fd moves the process root
 * to the directory referenced by the fd.
 *
 * This is the fd-based counterpart of :manpage:`chroot(2)`, introduced in
 * Linux v7.3. The syscall runs in a forked child so that the root of the
 * parent process, which the test framework needs for its cleanup, is left
 * untouched.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/syscalls.h"

#define JAILDIR "jail"
#define CANARY JAILDIR "/canary"

static void run(void)
{
	struct stat st;

	if (!SAFE_FORK()) {
		int dfd = SAFE_OPEN(JAILDIR, O_PATH | O_DIRECTORY);

		TST_EXP_PASS(tst_syscall(__NR_fchroot, dfd, 0),
			"fchroot() with a directory fd");

		TST_EXP_PASS(stat("/canary", &st),
			"canary file visible under the new root");

		exit(0);
	}
}

static void setup(void)
{
	SAFE_MKDIR(JAILDIR, 0755);
	SAFE_TOUCH(CANARY, 0644, NULL);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.forks_child = 1,
};
