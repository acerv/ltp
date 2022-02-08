// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 *				Serge Hallyn <serue@us.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Create shm with key 0xEAEAEA and in cloned process try to get the shm.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_test.h"
#include "common.h"

#define TESTKEY 0xEAEAEA

static char *str_op = "clone";

static int p1[2];
static int p2[2];

static int check_shmid(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	char buf[3];
	int id;

	SAFE_CLOSE(p1[1]);
	SAFE_CLOSE(p2[0]);

	SAFE_READ(1, p1[0], buf, 3);

	id = shmget(TESTKEY, 100, 0);
	if (id < 0) {
		SAFE_WRITE(1, p2[1], "notfnd", 7);
	} else {
		SAFE_WRITE(1, p2[1], "exists", 7);
		shmctl(id, IPC_RMID, NULL);
	}

	return 0;
}

static void run(void)
{
	int use_clone = T_NONE;
	int id;
	char buf[7];

	/* Using PIPE's to sync between container and Parent */
	SAFE_PIPE(p1);
	SAFE_PIPE(p2);

	if (!strcmp(str_op, "clone"))
		use_clone = T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		use_clone = T_UNSHARE;

	/* first create the key */
	TEST(shmget(TESTKEY, 100, IPC_CREAT));
	if (TST_RET < 0)
		tst_brk(TBROK, "shmget: %s", tst_strerrno(-TST_ERR));

	id = (int)TST_RET;

	tst_res(TINFO, "shmid namespaces test : %s", str_op);

	/* fire off the test */
	clone_unshare_test(use_clone, CLONE_NEWIPC, check_shmid, NULL);

	SAFE_CLOSE(p1[0]);
	SAFE_CLOSE(p2[1]);

	SAFE_WRITE(1, p1[1], "go", 3);
	SAFE_READ(1, p2[0], buf, 7);

	if (strcmp(buf, "exists") == 0) {
		if (use_clone == T_NONE)
			tst_res(TPASS, "plain cloned process found shmid");
		else
			tst_res(TFAIL, "%s: child process found shmid", str_op);
	} else {
		if (use_clone == T_NONE) {
			tst_res(TFAIL,
				"plain cloned process didn't find shmid");
		} else {
			tst_res(TPASS, "%s: child process didn't find shmid",
				str_op);
		}
	}

	/* destroy the key */
	shmctl(id, IPC_RMID, NULL);
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
	.forks_child = 1,
	.needs_root = 1,
	.needs_checkpoints = 1,
	.options =
		(struct tst_option[]){
			{ "m:", &str_op,
			  "Test execution mode <clone|unshare|none>" },
			{},
		},
};
