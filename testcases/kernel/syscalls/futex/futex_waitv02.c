// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies futex_waitv syscall using private data.
 */

#include <stdlib.h>
#include <time.h>
#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "futextest.h"

static char *str_numfutex;
static int numfutex = 30;

static uint32_t *futexes;
static struct futex_waitv *waitv;

static void setup(void)
{
	struct futex_test_variants tv;
	int i;

	tv = futex_variants[tst_variant];

	tst_res(TINFO, "Testing variant: %s", tv.desc);
	futex_supported_by_kernel(tv.fntype);

	if (tst_parse_int(str_numfutex, &numfutex, 1, FUTEX_WAITV_MAX))
		tst_brk(TBROK, "Invalid number of futexes '%s'", str_numfutex);

	futexes = SAFE_MALLOC(sizeof(uint32_t) * numfutex);
	memset(futexes, 0, numfutex);

	waitv = SAFE_MALLOC(sizeof(struct futex_waitv) * numfutex);
	for (i = 0; i < numfutex; i++) {
		waitv[i].uaddr = (uintptr_t)&futexes[i];
		waitv[i].flags = FUTEX_32 | FUTEX_PRIVATE_FLAG;
		waitv[i].val = 0;
	}
}

static void cleanup(void)
{
	free(futexes);
	free(waitv);
}

static void *threaded(void *arg)
{
	struct futex_test_variants tv;
	int ret, pid = *(int *)arg;

	tv = futex_variants[tst_variant];

	TST_PROCESS_STATE_WAIT(pid, 'S', 0);

	ret = futex_wake(tv.fntype, (void *)(uintptr_t)waitv[numfutex - 1].uaddr, 1, FUTEX_PRIVATE_FLAG);
	if (ret < 0)
		tst_brk(TBROK, "futex_wake private returned: %d %s", ret, tst_strerrno(-ret));

	return NULL;
}

static void run(void)
{
	struct timespec to;
	int ret, pid = getpid();
	pthread_t t;

	SAFE_PTHREAD_CREATE(&t, NULL, threaded, (void *)&pid);

	/* setting absolute timeout for futex2 */
	if (clock_gettime(CLOCK_MONOTONIC, &to))
		tst_brk(TBROK, "gettime64 failed");

	to.tv_sec++;

	ret = tst_futex_waitv(waitv, numfutex, 0, &to, CLOCK_MONOTONIC);
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
	.cleanup = cleanup,
	.min_kver = "5.16",
	.test_variants = ARRAY_SIZE(futex_variants),
	.options = (struct tst_option[]){
		{"n:", &str_numfutex, "Number of futex (default 30)"},
		{},
	},
};
