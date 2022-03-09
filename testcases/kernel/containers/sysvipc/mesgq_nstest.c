// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2009
 *				Veerendra C <vechandr@in.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test if mesgq is sent/read between namespaces via SysV IPC.
 *
 * [Algorithm]
 *
 * In parent process create a new mesgq with a specific key.
 * In cloned process try to access the created mesgq.
 *
 * Test will PASS if the mesgq is readable when flag is None.
 * Test will FAIL if the mesgq is readable when flag is Unshare or Clone or
 * the message received is wrong.
 */

#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_safe_sysv_ipc.h"
#include "tst_test.h"
#include "common.h"

#define KEY_VAL 154326L
#define MSG_TYPE 5
#define MSG_TEXT "My message!"

static char *str_op = "clone";
static int use_clone;

static struct msg_buf {
	long mtype;
	char mtext[80];
} msg;

static void mesgq_read(int id)
{
	int n;

	n = SAFE_MSGRCV(id, &msg, sizeof(msg.mtext), MSG_TYPE, 0);

	tst_res(TINFO, "Mesg read of %d bytes, Type %ld, Msg: %s", n, msg.mtype, msg.mtext);

	if (strcmp(msg.mtext, MSG_TEXT))
		tst_res(TFAIL, "Received the wrong text message");
}

static int check_mesgq(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	int id;

	id = msgget(KEY_VAL, 0);

	if (id < 0) {
		if (use_clone == T_NONE)
			tst_res(TFAIL, "Plain cloned process didn't find mesgq");
		else
			tst_res(TPASS, "%s: container didn't find mesgq", str_op);
	} else {
		if (use_clone == T_NONE)
			tst_res(TPASS, "Plain cloned process found mesgq inside container");
		else
			tst_res(TFAIL, "%s: container init process found mesgq", str_op);

		mesgq_read(id);
	}

	TST_CHECKPOINT_WAKE(0);

	return 0;
}

static void run(void)
{
	int id;

	id = SAFE_MSGGET(KEY_VAL, IPC_CREAT | IPC_EXCL | 0600);

	msg.mtype = MSG_TYPE;
	strcpy(msg.mtext, "My message!");

	SAFE_MSGSND(id, &msg, strlen(msg.mtext), 0);

	tst_res(TINFO, "mesgq namespaces test: %s", str_op);

	clone_unshare_test(use_clone, CLONE_NEWIPC, check_mesgq, NULL);

	TST_CHECKPOINT_WAIT(0);

	id = SAFE_MSGGET(KEY_VAL, 0);
	SAFE_MSGCTL(id, IPC_RMID, NULL);
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
