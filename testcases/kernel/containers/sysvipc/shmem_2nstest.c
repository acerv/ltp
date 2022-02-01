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
 * In Cont1, create Shared Memory segment with key 124426L
 * In Cont2, try to access the MQ created in Cont1.
 * PASS :
 * 		If flag = None and the shmem seg is accessible in Cont2.
 *		If flag = unshare/clone and the shmem seg is not accessible in
 *Cont2. If shmem seg is not accessible in Cont2, creates new shmem with same
 *key to double check isloation in IPCNS.
 *
 * FAIL :
 * 		If flag = none and the shmem seg is not accessible.
 * 		If flag = unshare/clone and shmem seg is accessible in Cont2.
 *		If the new shmem seg creation Fails.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_test.h"
#include "libclone.h"
#include "common.h"

#define TESTKEY 124426L

static char *str_op = "clone";

static int p1[2];
static int p2[2];

/*
 * check_shmem1() does not read -- it writes to check_shmem2() when it's done.
 */
static int check_shmem1(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	SAFE_CLOSE(p1[0]);

	/* first create the key */
	TEST(shmget(TESTKEY, 100, IPC_CREAT));
	if (TST_RET < 0)
		tst_brk(TBROK, "shmget: %s", tst_strerrno(-TST_ERR));

	tst_res(TINFO, "Cont1: Able to create shared mem segment");

	SAFE_WRITE(1, p1[1], "done", 5);

	return 0;
}

/*
 * check_shmem2() reads from check_shmem1() and writes to main() when it's done.
 */
static int check_shmem2(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	char buf[3];

	SAFE_CLOSE(p1[1]);
	SAFE_CLOSE(p2[0]);

	SAFE_READ(1, p1[0], buf, 3);

	/* Trying to access shmem, if not existing create new shmem */
	TEST(shmget(TESTKEY, 100, 0));
	if (TST_RET < 0) {
		TEST(shmget(TESTKEY, 100, IPC_CREAT));

		if (TST_RET < 0) {
			tst_brk(TBROK, "shmget: %s", tst_strerrno(-TST_ERR));
		} else {
			tst_res(TINFO, "Cont2: Able to allocate shmem seg with "
				       "the same key");
		}

		SAFE_WRITE(1, p2[1], "notfnd", 7);
	} else {
		SAFE_WRITE(1, p2[1], "exists", 7);
	}

	return 0;
}

static void run(void)
{
	int ret, use_clone = T_NONE;
	char buf[7];
	int id;

	/* Using PIPE's to sync between container and Parent */
	SAFE_PIPE(p1);
	SAFE_PIPE(p2);

	if (!strcmp(str_op, "clone"))
		use_clone = T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		use_clone = T_UNSHARE;

	tst_res(TINFO, "Shared Memory namespace test : %s", str_op);

	/* Create 2 containers */
	ret = tst_clone_unshare_test(use_clone, CLONE_NEWIPC, check_shmem1,
				     NULL);
	if (ret < 0)
		tst_brk(TBROK, "clone/unshare failed");

	ret = tst_clone_unshare_test(use_clone, CLONE_NEWIPC, check_shmem2,
				     NULL);
	if (ret < 0)
		tst_brk(TFAIL, "clone/unshare failed");

	SAFE_CLOSE(p2[1]);
	SAFE_READ(1, p2[0], buf, 7);

	if (strcmp(buf, "exists") == 0) {
		if (use_clone == T_NONE) {
			tst_res(TPASS,
				"Plain cloned process able to access shmem "
				"segment created");
		} else {
			tst_res(TFAIL,
				"%s : In namespace2 found the shmem segment "
				"created in Namespace1",
				str_op);
		}
	} else {
		if (use_clone == T_NONE) {
			tst_res(TFAIL,
				"Plain cloned process didn't find shmem seg");
		} else {
			tst_res(TPASS,
				"%s : In namespace2 unable to access the "
				"shmem seg "
				"created in Namespace1",
				str_op);
		}
	}

	/* destroy the key */
	id = shmget(TESTKEY, 100, 0);
	shmctl(id, IPC_RMID, NULL);
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
	.forks_child = 1,
	.needs_root = 1,
	.needs_checkpoints = 1,
	.options = (struct tst_option[]) {
		{"m:", &str_op, "Test execution mode <clone|unshare|none>"},
		{},
	},
};
