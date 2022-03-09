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
 * In parent process create a new semaphore with a specific key.
 * In cloned process, try to access the created semaphore
 *
 * Test PASS if the semaphore is readable when flag is None.
 * Test FAIL if the semaphore is readable when flag is Unshare or Clone.
 */

#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "tst_safe_sysv_ipc.h"
#include "tst_test.h"
#include "common.h"

#define MY_KEY 154326L

static char *str_op = "clone";
static int use_clone;

static int check_semaphore(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	int id;

	id = semget(MY_KEY, 1, 0);

	if (id < 0) {
		if (use_clone == T_NONE)
			tst_res(TFAIL, "Plain cloned process didn't find semaphore");
		else
			tst_res(TPASS, "%s: container didn't find semaphore", str_op);
	} else {
		tst_res(TINFO, "PID %d: fetched existing semaphore..id = %d", getpid(), id);

		if (use_clone == T_NONE)
			tst_res(TPASS, "Plain cloned process found semaphore inside container");
		else
			tst_res(TFAIL, "%s: Container init process found semaphore", str_op);
	}

	TST_CHECKPOINT_WAKE(0);

	return 0;
}

static void run(void)
{
	int id;

	TEST(semget(MY_KEY, 1, IPC_CREAT | IPC_EXCL | 0666));
	if (TST_RET < 0) {
		if (TST_ERR != EEXIST)
			tst_brk(TBROK | TERRNO, "Semaphore creation failed");

		SAFE_SEMGET(MY_KEY, 1, 0);
	}

	tst_res(TINFO, "Semaphore namespaces Isolation test : %s", str_op);

	clone_unshare_test(use_clone, CLONE_NEWIPC, check_semaphore, NULL);

	TST_CHECKPOINT_WAIT(0);

	id = SAFE_SEMGET(MY_KEY, 1, 0);
	SAFE_SEMCTL(id, IPC_RMID, 0);
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
	.needs_checkpoints = 1,
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare|none>" },
		{},
	},
};
