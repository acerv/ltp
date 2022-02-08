// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2009
 *				Veerendra C <vechandr@in.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * In Parent Process , create semaphore with key 154326L
 * Now create container by passing 1 of the below flag values..
 *	clone(NONE), clone(CLONE_NEWIPC), or unshare(CLONE_NEWIPC)
 * In cloned process, try to access the created semaphore
 * Test PASS: If the semaphore is readable when flag is None.
 * Test FAIL: If the semaphore is readable when flag is Unshare or Clone.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "tst_test.h"
#include "common.h"

#define MY_KEY 154326L

static char *str_op = "clone";

static int p1[2];
static int p2[2];

static int check_semaphore(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	char buf[3];
	int id;

	SAFE_CLOSE(p1[1]);
	SAFE_CLOSE(p2[0]);

	SAFE_READ(1, p1[0], buf, 3);

	id = semget(MY_KEY, 1, 0);
	if (id < 0) {
		SAFE_WRITE(1, p2[1], "notfnd", 7);
	} else {
		SAFE_WRITE(1, p2[1], "exists", 7);
		tst_res(TINFO, "PID %d: Fetched existing semaphore..id = %d",
			getpid(), id);
	}

	return 0;
}

static void run(void)
{
	int use_clone = T_NONE, id;
	char buf[7];

	/* Using PIPE's to sync between container and Parent */
	SAFE_PIPE(p1);
	SAFE_PIPE(p2);

	if (!strcmp(str_op, "clone"))
		use_clone = T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		use_clone = T_UNSHARE;

	/* 1. Create (or fetch if existing) the binary semaphore */
	id = semget(MY_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
	if (id < 0) {
		tst_res(TINFO, "semget failure. Checking existing semaphore");

		if (errno != EEXIST)
			tst_brk(TBROK, "Semaphore creation failed");

		id = semget(MY_KEY, 1, 0);
		if (id < 0)
			tst_brk(TBROK, "Semaphore operation failed");
	}

	tst_res(TINFO, "Semaphore namespaces Isolation test : %s", str_op);

	/* fire off the test */
	clone_unshare_test(use_clone, CLONE_NEWIPC, check_semaphore, NULL);

	SAFE_CLOSE(p1[0]);
	SAFE_CLOSE(p2[1]);
	SAFE_WRITE(1, p1[1], "go", 3);
	SAFE_READ(1, p2[0], buf, 7);

	if (!strcmp(buf, "exists")) {
		if (use_clone == T_NONE) {
			tst_res(TPASS, "Plain cloned process found semaphore "
				       "inside container");
		} else {
			tst_res(TFAIL,
				"%s: Container init process found semaphore",
				str_op);
		}
	} else {
		if (use_clone == T_NONE) {
			tst_res(TFAIL,
				"Plain cloned process didn't find semaphore");
		} else {
			tst_res(TPASS, "%s: Container didn't find semaphore",
				str_op);
		}
	}

	/* Delete the semaphore */
	id = semget(MY_KEY, 1, 0);
	semctl(id, IPC_RMID, 0);
}

static void setup(void)
{
	check_newipc();

	if (strcmp(str_op, "clone") && strcmp(str_op, "unshare") &&
	    strcmp(str_op, "none"))
		tst_brk(TBROK, "Test execution mode <clone|unshare|none>");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_root = 1,
	.forks_child = 1,
	.options =
		(struct tst_option[]){
			{ "m:", &str_op,
			  "Test execution mode <clone|unshare|none>" },
			{},
		},
};
