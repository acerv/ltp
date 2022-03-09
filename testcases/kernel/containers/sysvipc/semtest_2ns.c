// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2009
 *				Veerendra C <vechandr@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test semaphore usage between namespaces via SysV IPC.
 *
 * [Algorithm]
 *
 * Create 2 'containers'
 * In container1 create semaphore with a specific key
 * In container2 try to access the semaphore created in container1
 *
 * Test is PASS if flag = none and the semaphore is accessible in container2 or
 * if flag = unshare/clone and the semaphore is not accessible in container2.
 * If semaphore is not accessible in container2, creates new semaphore with the
 * same key to double check isloation in IPCNS.
 *
 * Test is FAIL if flag = none and the semaphore is not accessible, if
 * flag = unshare/clone and semaphore is accessible in container2 or if the new
 * semaphore creation Fails.
 */

#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "tst_safe_sysv_ipc.h"
#include "tst_test.h"
#include "common.h"

#define MY_KEY 124326L

static char *str_op = "clone";
static int use_clone;

static struct sembuf semop_lock[2] = {
	/* sem_num, sem_op, flag */
	{ 0, 0, 0 }, /* wait for sem#0 to become 0 */
	{ 0, 1, SEM_UNDO } /* then increment sem#0 by 1 */
};

static struct sembuf semop_unlock[1] = {
	/* sem_num, sem_op, flag */
	{ 0, -1, (IPC_NOWAIT | SEM_UNDO) } /* decrement sem#0 by 1 (sets it to 0) */
};

/*
 * sem_lock() - Locks the semaphore for crit-sec updation, and unlocks it later
 */
static void sem_lock(int id)
{
	SAFE_SEMOP(id, &semop_lock[0], 2);

	tst_res(TINFO, "semaphore1: File locked, Critical section is updated...");

	sleep(2);

	SAFE_SEMOP(id, &semop_unlock[0], 1);
}

/*
 * check_sem1 -  does not read -- it writes to check_sem2() when it's done.
 */
static int check_sem1(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	TEST(semget(MY_KEY, 1, IPC_CREAT | IPC_EXCL | 0666));
	if (TST_RET < 0) {
		tst_res(TINFO, "semget failure. Checking existing semaphore");

		if (TST_ERR != EEXIST)
			tst_brk(TBROK | TRERRNO, "Semaphore creation failed");

		TEST(semget(MY_KEY, 1, 0));
		if (TST_RET < 0)
			tst_brk(TBROK | TERRNO, "Semaphore operation failed");
	}

	tst_res(TINFO, "container1: Able to create semaphore");

	return 0;
}

/*
 * check_sem2() reads from check_sem1() and writes to main() when it's done.
 */
static int check_sem2(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	int id;

	id = semget(MY_KEY, 1, 0);
	if (id >= 0) {
		sem_lock(id);

		if (use_clone == T_NONE)
			tst_res(TPASS, "Plain cloned process able to access the semaphore created");
		else
			tst_res(TFAIL, "%s : In namespace2 found the semaphore created in Namespace1", str_op);
	} else {
		/* Trying to create a new semaphore, if semaphore is not existing */
		TEST(semget(MY_KEY, 1, IPC_CREAT | IPC_EXCL | 0666));
		if (TST_RET < 0) {
			if (TST_ERR != EEXIST)
				tst_brk(TBROK | TERRNO, "semget error");
		} else {
			tst_res(TINFO, "container2: Able to create semaphore with sameKey");
		}

		if (use_clone == T_NONE)
			tst_res(TFAIL, "Plain cloned process didn't find semaphore");
		else
			tst_res(TPASS, "%s : In namespace2 unable to access the semaphore created in namespace1", str_op);
	}

	id = SAFE_SEMGET(MY_KEY, 1, 0);
	SAFE_SEMCTL(id, IPC_RMID, 0);

	return 0;
}

static void run(void)
{
	clone_unshare_test(use_clone, CLONE_NEWIPC, check_sem1, NULL);
	clone_unshare_test(use_clone, CLONE_NEWIPC, check_sem2, NULL);
}

static void setup(void)
{
	use_clone = get_clone_unshare_enum(str_op);

	if (use_clone != T_NONE)
		check_newipc();
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare|none>" },
		{},
	},
};
