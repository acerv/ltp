// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2002
 * Author: Dave Olien <oliend@us.ibm.com>
 * Ported to LTP: Paul Larson <plars@linuxtestproject.org>
 */

/*\
 * Verify that SEM_UNDO on :manpage:`semop(2)` is only undone when the
 * last pthread exits, not when an individual thread exits.
 *
 * [Algorithm]
 *
 * - Create a semaphore and set its value to 1.
 * - A poster thread increments the semaphore with SEM_UNDO then exits.
 * - A waiter thread sleeps briefly, then decrements the semaphore.
 * - If the undo happened at thread exit, the waiter would block forever.
 * - The main thread joins both threads and checks the result.
 */

#include <pthread.h>

#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "lapi/sem.h"
#include "tst_safe_sysv_ipc.h"

static int sem_id = -1;

static struct sembuf sem_wait = {0, -1, SEM_UNDO};
static struct sembuf sem_post = {0, 1, SEM_UNDO};
static volatile int waiter_done;

static void *waiter(void *arg LTP_ATTRIBUTE_UNUSED)
{
	tst_res(TINFO, "%s waiting", __func__);
	usleep(500000);

	if (semop(sem_id, &sem_wait, 1) == -1)
		tst_brk(TBROK | TERRNO, "semop P failed in %s", __func__);

	tst_res(TINFO, "%s done waiting", __func__);
	waiter_done = 1;
	return NULL;
}

static void *poster(void *arg LTP_ATTRIBUTE_UNUSED)
{
	tst_res(TINFO, "%s posting", __func__);

	if (semop(sem_id, &sem_post, 1) == -1)
		tst_brk(TBROK | TERRNO, "semop V failed in %s", __func__);

	tst_res(TINFO, "%s done, exiting thread", __func__);
	return NULL;
}

static void run(void)
{
	pthread_t th_waiter, th_poster;
	union semun semunion;

	semunion.val = 1;
	SAFE_SEMCTL(sem_id, 0, SETVAL, semunion);

	waiter_done = 0;

	SAFE_PTHREAD_CREATE(&th_waiter, NULL, waiter, NULL);
	SAFE_PTHREAD_CREATE(&th_poster, NULL, poster, NULL);

	SAFE_PTHREAD_JOIN(th_poster, NULL);
	SAFE_PTHREAD_JOIN(th_waiter, NULL);

	if (waiter_done)
		tst_res(TPASS, "SEM_UNDO not triggered on thread exit");
	else
		tst_res(TFAIL, "SEM_UNDO triggered on thread exit");
}

static void setup(void)
{
	sem_id = SAFE_SEMGET(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
}

static void cleanup(void)
{
	if (sem_id != -1)
		SAFE_SEMCTL(sem_id, 0, IPC_RMID);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
};
