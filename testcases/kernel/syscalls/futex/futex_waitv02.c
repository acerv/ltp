// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies futex_waitv syscall using private data.
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <time.h>
#include "tst_test.h"
#include "lapi/futex.h"
#include "futex2test.h"
#include "tst_safe_pthread.h"
#include "tst_safe_clocks.h"

static char *str_numfutex;
static int numfutex = 30;

static uint32_t *futexes;
static struct futex_waitv *waitv;

static void setup(void)
{
	struct futex_test_variants tv;
	int i;

	tv = futex_variants();

	tst_res(TINFO, "Testing variant: %s", tv.desc);
	futex_supported_by_kernel(tv.fntype);

	if (tst_parse_int(str_numfutex, &numfutex, 1, FUTEX_WAITV_MAX))
		tst_brk(TBROK, "Invalid number of futexes '%s'", str_numfutex);

	futexes = tst_alloc(sizeof(uint32_t) * numfutex);
	memset(futexes, 0, numfutex);

	waitv = tst_alloc(sizeof(struct futex_waitv) * numfutex);
	for (i = 0; i < numfutex; i++) {
		waitv[i].uaddr = (uintptr_t)&futexes[i];
		waitv[i].flags = FUTEX_32 | FUTEX_PRIVATE_FLAG;
		waitv[i].val = 0;
	}
}

static void *threaded(void *arg)
{
	struct futex_test_variants tv;
	int ret, tid = *(int *)arg;

	tv = futex_variants();
	TST_THREAD_STATE_WAIT(tid, 'S', 0);

	ret = futex_wake(tv.fntype, (void *)(uintptr_t)waitv[numfutex - 1].uaddr,
			 1, FUTEX_PRIVATE_FLAG);
	if (ret < 0) {
		tst_brk(TBROK, "futex_wake private returned: %d %s", ret,
			tst_strerrno(-ret));
	}

	return NULL;
}

static void run(void)
{
	struct timespec to;
	int ret, tid;
	pthread_t t;

	tid = gettid();

	SAFE_PTHREAD_CREATE(&t, NULL, threaded, (void *)&tid);

	/* setting absolute timeout for futex2 */
	SAFE_CLOCK_GETTIME(CLOCK_MONOTONIC, &to);
	to.tv_sec++;

	ret = futex_waitv(waitv, numfutex, 0, &to, CLOCK_MONOTONIC);
	if (ret < 0)
		tst_brk(TBROK, "futex_waitv returned: %d %s", ret, tst_strerrno(-ret));
	else if (ret != numfutex - 1)
		tst_res(TFAIL, "futex_waitv returned: %d, expecting %d", ret, numfutex - 1);

	SAFE_PTHREAD_JOIN(t, NULL);
	tst_res(TPASS, "futex_waitv returned correctly");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.min_kver = "5.16",
	.bufs =
		(struct tst_buffers[]){
			{ &waitv, .size = sizeof(*waitv) },
			{ &futexes, .size = sizeof(*futexes) },
			{},
		},
	.options =
		(struct tst_option[]){
			{ "n:", &str_numfutex, "Number of futex (default 30)" },
			{},
		},
};
