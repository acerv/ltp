// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2009
 *				Veerendra C <vechandr@in.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * In Parent Process , create mesgq with key 154326L
 * Now create container by passing 1 of the flag values..
 *	Flag = clone, clone(CLONE_NEWIPC), or unshare(CLONE_NEWIPC)
 * In cloned process, try to access the created mesgq
 * Test PASS: If the mesgq is readable when flag is None.
 * Test FAIL: If the mesgq is readable when flag is Unshare or Clone.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_test.h"
#include "libclone.h"
#include "common.h"

#define KEY_VAL 154326L

static char *str_op = "clone";

static int p1[2];
static int p2[2];

struct msg_buf {
	long int mtype; /* type of received/sent message */
	char mtext[80]; /* text of the message */
} msg;

static void mesgq_read(int id)
{
	int READMAX = 80;
	int n;

	/* read msg type 5 on the Q; msgtype, flags are last 2 params.. */
	n = msgrcv(id, &msg, READMAX, 5, 0);
	if (n < 0)
		tst_brk(TBROK, "msgrcv: %s", tst_strerrno(-n));

	tst_res(TINFO, "Mesg read of %d bytes; Type %ld: Msg: %.*s", n,
		msg.mtype, n, msg.mtext);
}

static int check_mesgq(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	char buf[3];
	int id;

	SAFE_CLOSE(p1[1]);
	SAFE_CLOSE(p2[0]);

	SAFE_READ(1, p1[0], buf, 3);

	id = msgget(KEY_VAL, 0);
	if (id < 0)
		SAFE_WRITE(1, p2[1], "notfnd", 7);
	else {
		SAFE_WRITE(1, p2[1], "exists", 7);
		mesgq_read(id);
	}

	return 0;
}

static void run(void)
{
	int ret, use_clone = T_NONE, id, n;
	char buf[7];

	/* Using PIPE's to sync between container and Parent */
	SAFE_PIPE(p1);
	SAFE_PIPE(p2);

	if (!strcmp(str_op, "clone"))
		use_clone = T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		use_clone = T_UNSHARE;

	id = msgget(KEY_VAL, IPC_CREAT | IPC_EXCL | 0600);
	if (id < 0) {
		/* Retry without attempting to create the MQ */
		id = msgget(KEY_VAL, 0);
		if (id < 0)
			tst_brk(TBROK, "msgget: %s", tst_strerrno(-id));
	}

	msg.mtype = 5;
	strcpy(msg.mtext, "Message of type 5!");

	n = msgsnd(id, &msg, strlen(msg.mtext), 0);
	if (n < 0)
		tst_brk(TBROK, "msgsnd: %s", tst_strerrno(-n));

	tst_res(TINFO, "mesgq namespaces test : %s", str_op);

	/* fire off the test */
	ret = tst_clone_unshare_test(use_clone, CLONE_NEWIPC, check_mesgq,
				     NULL);
	if (ret < 0)
		tst_brk(TFAIL, "%s failed", str_op);

	SAFE_CLOSE(p1[0]);
	SAFE_CLOSE(p2[1]);

	SAFE_WRITE(1, p1[1], "go", 3);
	SAFE_READ(1, p2[0], buf, 7);

	if (!strcmp(buf, "exists")) {
		if (use_clone == T_NONE) {
			tst_res(TPASS, "Plain cloned process found mesgq "
				       "inside container");
		} else {
			tst_res(TFAIL, "%s: Container init process found mesgq",
				str_op);
		}
	} else {
		if (use_clone == T_NONE) {
			tst_res(TFAIL,
				"Plain cloned process didn't find mesgq");
		} else {
			tst_res(TPASS, "%s: Container didn't find mesgq",
				str_op);
		}
	}

	/* Delete the mesgQ */
	id = msgget(KEY_VAL, 0);
	msgctl(id, IPC_RMID, NULL);
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
