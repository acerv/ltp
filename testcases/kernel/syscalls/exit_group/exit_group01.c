// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Ported to LTP by Manas Kumar Nayak <maknayak@in.ibm.com>
 * Copyright (c) 2015 Linux Test Project
 * Copyright (C) 2015 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test checks if exit_group() correctly ends a spawned child and all its
 * running threads.
 */

#include <stdio.h>
#include <stdlib.h>
#include "tst_safe_pthread.h"
#include "tst_test.h"
#include "lapi/syscalls.h"

#define THREADS_NUM 10

static pid_t *tids;
static int *counter;
static pthread_mutex_t *lock;

static void *worker(void *arg)
{
	int i = *((int *)arg);

	tids[i] = tst_gettid();

	SAFE_PTHREAD_MUTEX_LOCK(lock);
	tst_atomic_inc(counter);
	SAFE_PTHREAD_MUTEX_UNLOCK(lock);

	pause();

	return arg;
}

static void spawn_threads(void)
{
	pthread_t threads[THREADS_NUM];

	for (int i = 0; i < THREADS_NUM; i++) {
		SAFE_PTHREAD_CREATE(&threads[i], NULL, worker, (void *)&i);
		usleep(100);
	}
}

static void run(void)
{
	pid_t pid;
	int status;

	pid = SAFE_FORK();
	if (!pid) {
		spawn_threads();

		TEST(tst_syscall(__NR_exit_group, 4));
		if (TST_RET == -1)
			tst_brk(TBROK | TERRNO, "exit_group() error");

		return;
	}

	SAFE_WAITPID(pid, &status, 0);

	while (*counter < THREADS_NUM)
		usleep(100);

	TST_EXP_EXPR(WIFEXITED(status) && WEXITSTATUS(status) == 4,
		"exit_group() succeeded");
}

static void setup(void)
{
	tids = SAFE_MMAP(
		NULL,
		sizeof(pid_t) * THREADS_NUM,
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);

	counter = SAFE_MMAP(
		NULL,
		sizeof(int),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);

	lock = SAFE_MMAP(
		NULL,
		sizeof(pthread_mutex_t),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);

	SAFE_PTHREAD_MUTEX_INIT(lock, NULL);
}

static void cleanup(void)
{
	SAFE_PTHREAD_MUTEX_DESTROY(lock);

	SAFE_MUNMAP(tids, sizeof(pid_t) * THREADS_NUM);
	SAFE_MUNMAP(counter, sizeof(int));
	SAFE_MUNMAP(counter, sizeof(pthread_mutex_t));
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1,
	.needs_checkpoints = 1,
};
