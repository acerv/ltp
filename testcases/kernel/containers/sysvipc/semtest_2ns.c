// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2009
 *				Veerendra C <vechandr@in.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Create 2 'containers' with the below flag value
 *   Flag = clone, clone(CLONE_NEWIPC), or unshare(CLONE_NEWIPC)
 * In Cont1, create semaphore with key 124326L
 * In Cont2, try to access the semaphore created in Cont1.
 * PASS :
 *		If flag = None and the semaphore is accessible in Cont2.
 *		If flag = unshare/clone and the semaphore is not accessible in
 *Cont2. If semaphore is not accessible in Cont2, creates new semaphore with the
 *same key to double check isloation in IPCNS.
 *
 * FAIL :
 *		If flag = none and the semaphore is not accessible.
 *		If flag = unshare/clone and semaphore is accessible in Cont2.
 *		If the new semaphore creation Fails.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "tst_test.h"
#include "libclone.h"
#include "common.h"

#define MY_KEY 124326L

static char *str_op = "clone";

static int p1[2];
static int p2[2];

static struct sembuf semop_lock[2] = {
	/* sem_num, sem_op, flag */
	{0, 0, 0},	 /* wait for sem#0 to become 0 */
	{0, 1, SEM_UNDO} /* then increment sem#0 by 1 */
};

static struct sembuf semop_unlock[1] = {
	/* sem_num, sem_op, flag */
	{0, -1,
	 (IPC_NOWAIT | SEM_UNDO)} /* decrement sem#0 by 1 (sets it to 0) */
};

/*
 * sem_lock() - Locks the semaphore for crit-sec updation, and unlocks it later
 */
static void sem_lock(int id)
{
	int ret;

	/* Checking the semlock and simulating as if the crit-sec is updated */
	ret = semop(id, &semop_lock[0], 2);
	if (ret < 0)
		tst_brk(TBROK, "semop: %s", tst_strerrno(-ret));

	tst_res(TINFO, "Sem1: File locked, Critical section is updated...");

	sleep(2);

	ret = semop(id, &semop_unlock[0], 1);
	if (ret < 0)
		tst_brk(TBROK, "semop: %s", tst_strerrno(-ret));
}

/*
 * check_sem1 -  does not read -- it writes to check_sem2() when it's done.
 */
static int check_sem1(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	SAFE_CLOSE(p1[0]);

	/* 1. Create (or fetch if existing) the binary semaphore */
	TEST(semget(MY_KEY, 1, IPC_CREAT | IPC_EXCL | 0666));
	if (TST_RET < 0) {
		tst_res(TINFO, "semget failure. Checking existing semaphore");

		if (TST_ERR != EEXIST)
			tst_brk(TBROK, "Semaphore creation failed");

		TEST(semget(MY_KEY, 1, 0));
		if (TST_RET < 0)
			tst_brk(TBROK, "Semaphore operation failed");
	}

	SAFE_WRITE(1, p1[1], "go", 3);

	tst_res(TINFO, "Cont1: Able to create semaphore");

	return 0;
}

/*
 * check_sem2() reads from check_sem1() and writes to main() when it's done.
 */
static int check_sem2(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	char buf[3];
	int id2;

	SAFE_CLOSE(p1[1]);
	SAFE_CLOSE(p2[0]);
	SAFE_READ(1, p1[0], buf, 3);

	id2 = semget(MY_KEY, 1, 0);
	if (id2 >= 0) {
		sem_lock(id2);
		SAFE_WRITE(1, p2[1], "exists", 7);
	} else {
		/* Trying to create a new semaphore, if semaphore is not
		 * existing */
		TEST(semget(MY_KEY, 1, IPC_CREAT | IPC_EXCL | 0666));
		if (TST_RET < 0) {
			if (TST_ERR != EEXIST) {
				tst_brk(TBROK, "semget: %s",
					tst_strerrno(-TST_RET));
			}
		} else {
			tst_res(TINFO,
				"Cont2: Able to create semaphore with sameKey");
		}

		/* Passing the pipe Not-found mesg */
		SAFE_WRITE(1, p2[1], "notfnd", 7);
	}

	return 0;
}

static void run(void)
{
	int ret, id, use_clone = T_NONE;
	char buf[7];

	/* Using PIPE's to sync between container and Parent */
	SAFE_PIPE(p1);
	SAFE_PIPE(p2);

	if (!strcmp(str_op, "clone"))
		use_clone = T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		use_clone = T_UNSHARE;

	tst_res(TINFO, "Semaphore Namespaces Test : %s", str_op);

	/* Create 2 containers */
	ret = tst_clone_unshare_test(use_clone, CLONE_NEWIPC, check_sem1, NULL);
	if (ret < 0)
		tst_brk(TBROK, "clone/unshare failed");

	ret = tst_clone_unshare_test(use_clone, CLONE_NEWIPC, check_sem2, NULL);
	if (ret < 0)
		tst_brk(TBROK, "clone/unshare failed");

	SAFE_CLOSE(p2[1]);
	SAFE_READ(1, p2[0], buf, 7);

	if (!strcmp(buf, "exists"))
		if (use_clone == T_NONE) {
			tst_res(TPASS, "Plain cloned process able to access "
				       "the semaphore "
				       "created");
		} else {
			tst_res(TFAIL,
				"%s : In namespace2 found the semaphore "
				"created in Namespace1",
				str_op);
		}
	else if (use_clone == T_NONE) {
		tst_res(TFAIL, "Plain cloned process didn't find semaphore");
	} else {
		tst_res(TPASS,
			"%s : In namespace2 unable to access the semaphore "
			"created in Namespace1",
			str_op);
	}

	/* Delete the semaphore */
	id = semget(MY_KEY, 1, 0);
	semctl(id, IPC_RMID, 0);
}

static void setup(void)
{
	check_newipc();

	if (strcmp(str_op, "clone") && strcmp(str_op, "unshare")
	    && strcmp(str_op, "none"))
		tst_brk(TBROK, "Test execution mode <clone|unshare|none>");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{"m:", &str_op, "Test execution mode <clone|unshare|none>"},
		{},
	},
};
