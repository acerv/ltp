// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Tests an unbindable mount: unbindable mount is an unbindable
 * private mount.
 *
 * [Algorithm]
 *
 * 1. Creates directories "A", "B" and files "A/A", "B/B"
 * 2. Unshares mount namespace and makes it private (so mounts/umounts
 *    have no effect on a real system)
 * 3. Bind mounts directory "A" to "A"
 * 4. Makes directory "A" unbindable
 * 5. Tries to bind mount unbindable "A" to "B":
 *    - if it fails, test passes
 *    - if it passes, test fails
 */

#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/mount.h>
#include "common.h"
#include "tst_test.h"

#if defined(MS_SHARED) && defined(MS_PRIVATE) && defined(MS_REC)               \
	&& defined(MS_UNBINDABLE)

static void run(void)
{
	/* unshares the mount ns */
	SAFE_UNSHARE(CLONE_NEWNS);

	/* makes sure mounts/umounts have no effect on a real system */
	SAFE_MOUNT("none", "/", "none", MS_REC | MS_PRIVATE, NULL);

	/* bind mounts DIRA to itself */
	SAFE_MOUNT(DIRA, DIRA, "none", MS_BIND, NULL);

	/* makes mount DIRA unbindable */
	SAFE_MOUNT("none", DIRA, "none", MS_UNBINDABLE, NULL);

	/* tries to bind mount unbindable DIRA to DIRB which should fail */
	if (mount(DIRA, DIRB, "none", MS_BIND, NULL) == -1) {
		tst_res(TPASS, "unbindable mount passed");
	} else {
		SAFE_UMOUNT(DIRB);
		tst_res(TFAIL, "unbindable mount faled");
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
