// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Tests a shared mount: shared mount can be replicated to as many
 * mountpoints and all the replicas continue to be exactly same.
 *
 * [Algorithm]
 *
 * 1. Creates directories "A", "B" and files "A/A", "B/B"
 * 2. Unshares mount namespace and makes it private (so mounts/umounts
 *    have no effect on a real system)
 * 3. Bind mounts directory "A" to "A"
 * 4. Makes directory "A" shared
 * 5. Clones a new child process with CLONE_NEWNS flag
 * 6. There are two test cases (where X is parent namespace and Y child
 *    namespace):
 * 1)
 *	X: bind mounts "B" to "A"
 *	Y: must see "A/B"
 *	X: umounts "A"
 * 2)
 *	Y: bind mounts "B" to "A"
 *	X: must see "A/B"
 *	Y: umounts "A"
 */

#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/mount.h>
#include "common.h"
#include "tst_test.h"

#if defined(MS_SHARED) && defined(MS_PRIVATE) && defined(MS_REC)

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int ret = 0;

	TST_CHECKPOINT_WAIT(0);

	if (access(DIRA "/B", F_OK) < 0)
		ret = 2;

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	/* bind mounts DIRB to DIRA making contents of DIRB visible in DIRA */
	SAFE_MOUNT(DIRB, DIRA, "none", MS_BIND, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_UMOUNT(DIRA);

	return ret;
}

static void run(void)
{
	int status, ret;

	/* unshares the mount ns */
	SAFE_UNSHARE(CLONE_NEWNS);

	/* makes sure parent mounts/umounts have no effect on a real system */
	SAFE_MOUNT("none", "/", "none", MS_REC | MS_PRIVATE, NULL);

	/* bind mounts DIRA to itself */
	SAFE_MOUNT(DIRA, DIRA, "none", MS_BIND, NULL);

	/* makes mount DIRA shared */
	SAFE_MOUNT("none", DIRA, "none", MS_SHARED, NULL);

	ret = ltp_clone_quick(CLONE_NEWNS | SIGCHLD, child_func, NULL);
	if (ret < 0)
		tst_brk(TBROK, "clone failed");

	/* bind mounts DIRB to DIRA making contents of DIRB visible in DIRA */
	SAFE_MOUNT(DIRB, DIRA, "none", MS_BIND, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_UMOUNT(DIRA);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	if (access(DIRA "/B", F_OK) == 0)
		tst_res(TPASS, "shared mount in child passed");
	else
		tst_res(TFAIL, "shared mount in child failed");

	TST_CHECKPOINT_WAKE(0);

	SAFE_WAIT(&status);

	if (WIFEXITED(status)) {
		if ((WEXITSTATUS(status) == 0))
			tst_res(TPASS, "shared mount in parent passed");
		else
			tst_res(TFAIL, "shared mount in parent failed");
	}

	if (WIFSIGNALED(status)) {
		tst_brk(TBROK, "child was killed with signal %s",
			tst_strsig(WTERMSIG(status)));
	}

	SAFE_UMOUNT(DIRA);
}

static void setup(void)
{
	check_newns();
	create_folders();
}

static void cleanup(void)
{
	umount_folders();
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.needs_root = 1,
	.needs_checkpoints = 1,
};

#else
TST_TEST_TCONF("needed mountflags are not defined");
#endif
