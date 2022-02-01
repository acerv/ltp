// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * 1. Clones two child processes with CLONE_NEWIPC flag, each child
 *    creates System V semaphore (sem) with the _identical_ key.
 * 2. Child1 locks the semaphore.
 * 3. Child2 locks the semaphore.
 * 4. Locking the semaphore with the identical key but from two different
 *    IPC namespaces should not interfere with each other, so if child2
 *    is able to lock the semaphore (after child1 locked it), test passes,
 *    otherwise test fails.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_test.h"
#include "lapi/sem.h"
#include "libclone.h"
#include "common.h"

#define TESTKEY 124426L

static int chld1_sem(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id, ret;
	union semun su;
	struct sembuf sm;

	id = semget(TESTKEY, 1, IPC_CREAT);
	if (id < 0)
		tst_brk(TBROK, "semget: %s", tst_strerrno(-id));

	su.val = 1;

	ret = semctl(id, 0, SETVAL, su);
	if (ret < 0) {
		semctl(id, 0, IPC_RMID);
		tst_brk(TBROK, "semctl: %s", tst_strerrno(-ret));
	}

	/* tell child2 to continue and wait for it to create the semaphore */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	sm.sem_num = 0;
	sm.sem_op = -1;
	sm.sem_flg = IPC_NOWAIT;

	ret = semop(id, &sm, 1);
	if (ret < 0) {
		semctl(id, 0, IPC_RMID);
		tst_brk(TBROK, "semop: %s", tst_strerrno(-ret));
	}

	/* tell child2 to continue and wait for it to lock the semaphore */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	sm.sem_op = 1;
	semop(id, &sm, 1);

	semctl(id, 0, IPC_RMID);

	return 0;
}

static int chld2_sem(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id, ret, rval = 0;
	struct sembuf sm;
	union semun su;

	/* wait for child1 to create the semaphore */
	TST_CHECKPOINT_WAIT(0);

	id = semget(TESTKEY, 1, IPC_CREAT);
	if (id < 0)
		tst_brk(TBROK, "semget: %s", tst_strerrno(-id));

	su.val = 1;

	ret = semctl(id, 0, SETVAL, su);
	if (ret < 0) {
		semctl(id, 0, IPC_RMID);
		tst_brk(TBROK, "semctl: %s", tst_strerrno(-ret));
	}

	/* tell child1 to continue and wait for it to lock the semaphore */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	sm.sem_num = 0;
	sm.sem_op = -1;
	sm.sem_flg = IPC_NOWAIT;

	ret = semop(id, &sm, 1);
	if (ret < 0) {
		if (errno == EAGAIN) {
			rval = 1;
		} else {
			semctl(id, 0, IPC_RMID);
			tst_brk(TBROK, "semop: %s", tst_strerrno(-ret));
		}
	}

	/* tell child1 to continue */
	TST_CHECKPOINT_WAKE(0);

	sm.sem_op = 1;
	semop(id, &sm, 1);

	semctl(id, 0, IPC_RMID);

	return rval;
}

static void run(void)
{
	int status, ret = 0;

	ret = tst_clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld1_sem, NULL);
	if (ret == -1)
		tst_brk(TBROK, "child1 clone failed");

	ret = tst_clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld2_sem, NULL);
	if (ret == -1)
		tst_brk(TBROK, "child2 clone failed");

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

	if (ret)
		tst_res(TFAIL, "SysV sem: communication with identical keys"
			       " between namespaces");
	else
		tst_res(TPASS, "SysV sem: communication with identical keys"
			       " between namespaces");
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
