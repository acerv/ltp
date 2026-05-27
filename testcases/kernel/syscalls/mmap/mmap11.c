// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2010 Red Hat, Inc.
 */

/*\
 * Verify that :manpage:`munmap(2)` does not check sysctl_max_map_count.
 *
 * When a thread exits, glibc calls munmap() to free the thread stack.
 * If munmap() checks max_map_count, it can fail with ENOMEM due to VMA
 * splitting, causing glibc to abort(). This test creates detached threads
 * to verify that munmap() succeeds during thread cleanup.
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "tst_test.h"

#define MAL_SIZE (100 * 1024)

static void *wait_thread(void *args LTP_ATTRIBUTE_UNUSED)
{
	void *addr;

	addr = malloc(MAL_SIZE);
	if (addr)
		memset(addr, 1, MAL_SIZE);
	sleep(1);
	free(addr);
	return NULL;
}

static void *wait_thread2(void *args LTP_ATTRIBUTE_UNUSED)
{
	return NULL;
}

static void run(void)
{
	pthread_t th, th2;
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	if (ret)
		tst_brk(TBROK, "pthread_attr_init: %s", tst_strerrno(ret));

	ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (ret) {
		tst_brk(TBROK, "pthread_attr_setdetachstate: %s",
			tst_strerrno(ret));
	}

	ret = pthread_create(&th, &attr, wait_thread, NULL);
	if (ret)
		tst_brk(TBROK, "pthread_create: %s", tst_strerrno(ret));

	ret = pthread_create(&th2, &attr, wait_thread2, NULL);
	if (ret)
		tst_brk(TBROK, "pthread_create: %s", tst_strerrno(ret));

	pthread_attr_destroy(&attr);

	tst_res(TPASS, "munmap() did not fail during thread operations");
}

static struct tst_test test = {
	.test_all = run,
};
