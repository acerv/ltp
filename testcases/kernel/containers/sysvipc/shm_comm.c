// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * 1. Clones two child processes with CLONE_NEWIPC flag, each child
 *    allocates System V shared memory segment (shm) with the _identical_
 *    key and attaches that segment into its address space.
 * 2. Child1 writes into the shared memory segment.
 * 3. Child2 writes into the shared memory segment.
 * 4. Writes to the shared memory segment with the identical key but from
 *    two different IPC namespaces should not interfere with each other
 *    and so child1 checks whether its shared segment wasn't changed
 *    by child2, if it wasn't test passes, otherwise test fails.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_test.h"
#include "common.h"

#define TESTKEY 124426L
#define SHMSIZE 50

static int chld1_shm(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id, rval = 0;
	char *shmem;

	id = shmget(TESTKEY, SHMSIZE, IPC_CREAT);
	if (id < 0)
		tst_brk(TBROK, "shmget: %s", tst_strerrno(-id));

	shmem = shmat(id, NULL, 0);
	if (shmem == (char *)-1) {
		shmctl(id, IPC_RMID, NULL);
		tst_brk(TBROK, "shmem error");
	}

	*shmem = 'A';

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	/* if child1 shared segment has changed (by child2) report fail */
	if (*shmem != 'A')
		rval = 1;

	/* tell child2 to continue */
	TST_CHECKPOINT_WAKE(0);

	shmdt(shmem);
	shmctl(id, IPC_RMID, NULL);

	return rval;
}

static int chld2_shm(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id;
	char *shmem;

	id = shmget(TESTKEY, SHMSIZE, IPC_CREAT);
	if (id < 0)
		tst_brk(TBROK, "shmget: %s", tst_strerrno(-id));

	shmem = shmat(id, NULL, 0);
	if (shmem == (char *)-1) {
		shmctl(id, IPC_RMID, NULL);
		tst_brk(TBROK, "shmem error");
	}

	/* wait for child1 to write to his segment */
	TST_CHECKPOINT_WAIT(0);

	*shmem = 'B';

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	shmdt(shmem);
	shmctl(id, IPC_RMID, NULL);

	return 0;
}

static void run(void)
{
	int status, ret = 0;

	clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld1_shm, NULL);
	clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld2_shm, NULL);

	while (wait(&status) > 0) {
		if (WIFEXITED(status) && WEXITSTATUS(status) == 1)
			ret = 1;

		if (WIFEXITED(status) && WEXITSTATUS(status) == 2)
			tst_brk(TBROK, "error in child");

		if (WIFSIGNALED(status)) {
			tst_brk(TBROK, "child was killed with signal %s",
				tst_strsig(WTERMSIG(status)));
		}
	}

	if (ret) {
		tst_res(TFAIL, "SysV shm: communication with identical keys"
			       " between namespaces");
	} else {
		tst_res(TPASS, "SysV shm: communication with identical keys"
			       " between namespaces");
	}
}

static void setup(void)
{
	check_newipc();
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.needs_checkpoints = 1,
};
