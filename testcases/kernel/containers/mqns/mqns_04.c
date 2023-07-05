// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2009
 * Copyright (c) Serge Hallyn <serue@us.ibm.com>
 * Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test mqueuefs manipulation from child/parent namespaces.
 *
 * [Algorithm]
 *
 * - parent creates mqueue folder in <tmpdir>
 * - child mounts mqueue there
 * - child creates /MQ1 mqueue
 * - parent checks for <tmpdir>/mqueue/MQ1 existence
 * - child exits
 * - parent checks for <tmpdir>/mqueue/MQ1 existence
 * - parent tries 'touch <tmpdir>/mqueue/MQ2' -> should fail
 * - parent umount mqueuefs
 */

#include <sys/wait.h>
#include "tst_test.h"
#include "lapi/sched.h"
#include "tst_safe_posix_ipc.h"
#include "tst_safe_stdio.h"
#include "tst_safe_macros.h"

#define CHECK_MQ_OPEN_RET(x) ((x) >= 0 || ((x) == -1 && errno != EMFILE))

#define MQNAME1 "/MQ1"
#define MQNAME2 "/MQ2"

static char *str_op;
static char *devdir;
static char *mqueue1;
static char *mqueue2;
static volatile int *mq_freed;

static void check_mqueue(void)
{
	mqd_t mqd;

	tst_res(TINFO, "Creating %s mqueue from within child process", MQNAME1);

	mqd = TST_RETRY_FUNC(
		mq_open(MQNAME1, O_RDWR | O_CREAT | O_EXCL, 0755, NULL),
		CHECK_MQ_OPEN_RET);
	if (mqd == -1)
		tst_brk(TBROK | TERRNO, "mq_open failed");

	SAFE_MQ_CLOSE(mqd);
	mq_freed[0] = 1;

	tst_res(TINFO, "Mount %s from within child process", devdir);

	SAFE_MOUNT("mqueue", devdir, "mqueue", 0, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);
}

static void run(void)
{
	const struct tst_clone_args clone_args = { CLONE_NEWIPC, SIGCHLD };
	struct stat statbuf;

	if (str_op && !strcmp(str_op, "clone")) {
		tst_res(TINFO, "Spawning isolated process");

		if (!SAFE_CLONE(&clone_args)) {
			check_mqueue();
			return;
		}
	} else if (str_op && !strcmp(str_op, "unshare")) {
		tst_res(TINFO, "Spawning unshared process");

		if (!SAFE_FORK()) {
			SAFE_UNSHARE(CLONE_NEWIPC);
			check_mqueue();
			return;
		}
	}

	TST_CHECKPOINT_WAIT(0);

	SAFE_STAT(mqueue1, &statbuf);
	tst_res(TPASS, "%s child's mqueue can be accessed from parent", mqueue1);

	TST_CHECKPOINT_WAKE(0);

	tst_res(TINFO, "Waiting child to exit");

	tst_reap_children();
	mq_freed[0] = 0;

	SAFE_STAT(mqueue1, &statbuf);
	tst_res(TPASS, "%s child's mqueue can be accessed from parent after child's dead", mqueue1);

	tst_res(TINFO, "Try to create %s from parent", mqueue2);

	TST_EXP_FAIL(creat(mqueue2, 0755), EACCES);
	if (!TST_PASS)
		mq_freed[1] = 1;

	SAFE_UMOUNT(devdir);
}

static void setup(void)
{
	char *tmpdir;

	if (!str_op || (strcmp(str_op, "clone") && strcmp(str_op, "unshare")))
		tst_brk(TCONF, "Please specify clone|unshare child isolation");

	tmpdir = tst_get_tmpdir();

	SAFE_ASPRINTF(&devdir, "%s/mqueue", tmpdir);
	SAFE_MKDIR(devdir, 0755);

	SAFE_ASPRINTF(&mqueue1, "%s" MQNAME1, devdir);
	SAFE_ASPRINTF(&mqueue2, "%s" MQNAME2, devdir);

	mq_freed = SAFE_MMAP(NULL,
		2 * sizeof(int),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);
}

static void cleanup(void)
{
	if (!devdir)
		return;

	if (!access(mqueue1, F_OK))
		SAFE_MQ_UNLINK(MQNAME1);

	if (!access(mqueue2, F_OK))
		SAFE_MQ_UNLINK(MQNAME2);

	if (tst_is_mounted(devdir))
		SAFE_UMOUNT(devdir);

	if (*mq_freed)
		SAFE_MUNMAP((void *)mq_freed, 2 * sizeof(int));
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.needs_tmpdir = 1,
	.needs_checkpoints = 1,
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Child process isolation <clone|unshare>" },
		{},
	},
	.needs_kconfigs = (const char *[]) {
		"CONFIG_USER_NS",
		NULL
	},
};
