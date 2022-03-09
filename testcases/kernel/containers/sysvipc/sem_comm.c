// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test semaphore communication between cloned processes via SysV IPC.
 *
 * [Algorithm]
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

#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_safe_sysv_ipc.h"
#include "tst_test.h"
#include "lapi/sem.h"
#include "common.h"

#define TESTKEY 124426L

static int chld1_sem(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id;
	union semun su;
	struct sembuf sm;

	id = SAFE_SEMGET(TESTKEY, 1, IPC_CREAT);

	su.val = 1;
	TEST(semctl(id, 0, SETVAL, su));
	if (TST_RET < 0) {
		SAFE_SEMCTL(id, 0, IPC_RMID);
		tst_brk(TBROK | TERRNO, "semctl error");
	}

	/* tell child2 to continue and wait for it to create the semaphore */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	sm.sem_num = 0;
	sm.sem_op = -1;
	sm.sem_flg = IPC_NOWAIT;
	TEST(semop(id, &sm, 1));
	if (TST_RET < 0) {
		SAFE_SEMCTL(id, 0, IPC_RMID);
		tst_brk(TBROK | TERRNO, "semop error");
	}

	/* tell child2 to continue and wait for it to lock the semaphore */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	sm.sem_op = 1;
	SAFE_SEMOP(id, &sm, 1);

	SAFE_SEMCTL(id, 0, IPC_RMID);

	return 0;
}

static int chld2_sem(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id, rval = 0;
	struct sembuf sm;
	union semun su;

	/* wait for child1 to create the semaphore */
	TST_CHECKPOINT_WAIT(0);

	id = SAFE_SEMGET(TESTKEY, 1, IPC_CREAT);

	su.val = 1;
	TEST(semctl(id, 0, SETVAL, su));
	if (TST_RET < 0) {
		SAFE_SEMCTL(id, 0, IPC_RMID);
		tst_brk(TBROK | TERRNO, "semctl error");
	}

	/* tell child1 to continue and wait for it to lock the semaphore */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	sm.sem_num = 0;
	sm.sem_op = -1;
	sm.sem_flg = IPC_NOWAIT;
	TEST(semop(id, &sm, 1));
	if (TST_RET < 0) {
		if (TST_ERR == EAGAIN) {
			rval = 1;
		} else {
			SAFE_SEMCTL(id, 0, IPC_RMID);
			tst_brk(TBROK | TERRNO, "semop error");
		}
	}

	/* tell child1 to continue */
	TST_CHECKPOINT_WAKE(0);

	sm.sem_op = 1;
	SAFE_SEMOP(id, &sm, 1);

	SAFE_SEMCTL(id, 0, IPC_RMID);

	if (rval)
		tst_res(TFAIL, "SysV sem: communication with identical keys between namespaces");
	else
		tst_res(TPASS, "SysV sem: communication with identical keys between namespaces");

	return rval;
}

static void run(void)
{
	clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld2_sem, NULL);
	clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld1_sem, NULL);
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
