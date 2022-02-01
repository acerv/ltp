// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * 1. Clones two child processes with CLONE_NEWIPC flag, each child
 *    gets System V message queue (msg) with the _identical_ key.
 * 2. Child1 appends a message with identifier #1 to the message queue.
 * 3. Child2 appends a message with identifier #2 to the message queue.
 * 4. Appends to the message queue with the identical key but from
 *    two different IPC namespaces should not interfere with each other
 *    and so child1 checks whether its message queue doesn't contain
 *    a message with identifier #2, if it doesn't test passes, otherwise
 *    test fails.
 */

#define _GNU_SOURCE

#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/types.h>
#include "tst_test.h"
#include "libclone.h"
#include "common.h"

#define TESTKEY 124426L
#define MSGSIZE 50

struct sysv_msg {
	long mtype;
	char mtext[MSGSIZE];
};

static int chld1_msg(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id, ret, rval = 0;
	struct sysv_msg m;
	struct sysv_msg rec;

	id = msgget(TESTKEY, IPC_CREAT | 0600);
	if (id < 0)
		tst_brk(TBROK, "msgget: %s", tst_strerrno(-id));

	m.mtype = 1;
	m.mtext[0] = 'A';

	ret = msgsnd(id, &m, sizeof(struct sysv_msg) - sizeof(long), 0);
	if (ret < 0) {
		msgctl(id, IPC_RMID, NULL);
		tst_brk(TBROK, "msgsnd: %s", tst_strerrno(-ret));
	}

	/* wait for child2 to write into the message queue */
	TST_CHECKPOINT_WAIT(0);

	/* if child1 message queue has changed (by child2) report fail */
	ret = msgrcv(id, &rec, sizeof(struct sysv_msg) - sizeof(long), 2,
		     IPC_NOWAIT);
	if (ret < 0 && errno != ENOMSG) {
		msgctl(id, IPC_RMID, NULL);
		tst_brk(TBROK, "msgrcv: %s", tst_strerrno(-ret));
	}

	/* if mtype #2 was found in the message queue, it is fail */
	if (ret > 0)
		rval = 1;

	/* tell child2 to continue */
	TST_CHECKPOINT_WAKE(0);

	msgctl(id, IPC_RMID, NULL);

	return rval;
}

static int chld2_msg(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int id, ret;
	struct sysv_msg m;

	id = msgget(TESTKEY, IPC_CREAT | 0600);
	if (id < 0)
		tst_brk(TBROK, "msgget: %s", tst_strerrno(-id));

	m.mtype = 2;
	m.mtext[0] = 'B';

	ret = msgsnd(id, &m, sizeof(struct sysv_msg) - sizeof(long), 0);
	if (ret < 0) {
		msgctl(id, IPC_RMID, NULL);
		tst_brk(TBROK, "msgsnd: %s", tst_strerrno(-ret));
	}

	/* tell child1 to continue and wait for it */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	msgctl(id, IPC_RMID, NULL);

	return 0;
}

static void run(void)
{
	int status, ret = 0;

	ret = tst_clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld1_msg, NULL);
	if (ret == -1)
		tst_brk(TBROK, "child1 clone failed");

	ret = tst_clone_unshare_test(T_CLONE, CLONE_NEWIPC, chld2_msg, NULL);
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

	if (ret) {
		tst_res(TFAIL, "SysV msg: communication with identical keys"
			       " between namespaces");
	} else {
		tst_res(TPASS, "SysV msg: communication with identical keys"
			       " between namespaces");
	}
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
