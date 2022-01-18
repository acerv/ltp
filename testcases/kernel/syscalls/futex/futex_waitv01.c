// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies EINVAL for futex_waitv syscall.
 */

#include <stdlib.h>
#include <time.h>
#include "tst_test.h"
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

static void init_timeout(struct timespec *to)
{
	if (clock_gettime(CLOCK_MONOTONIC, to))
		tst_brk(TBROK, "gettime64 failed");

	to->tv_sec++;
}

static void run(void)
{
	struct timespec to;
	int res;

	/* Testing a waiter without FUTEX_32 flag */
	waitv[0].flags = FUTEX_PRIVATE_FLAG;

	init_timeout(&to);

	res = tst_futex_waitv(waitv, numfutex, 0, &to, CLOCK_MONOTONIC);
	if (res == EINVAL)
		tst_res(TFAIL, "futex_waitv private returned: %d %s", res, tst_strerrno(res));
	else
		tst_res(TPASS, "futex_waitv without FUTEX_32");

	/* Testing a waiter with an unaligned address */
	waitv[0].flags = FUTEX_PRIVATE_FLAG | FUTEX_32;
	waitv[0].uaddr = 1;

	init_timeout(&to);

	res = tst_futex_waitv(waitv, numfutex, 0, &to, CLOCK_MONOTONIC);
	if (res == EINVAL)
		tst_res(TFAIL, "futex_waitv private returned: %d %s", res, tst_strerrno(res));
	else
		tst_res(TPASS, "futex_waitv with an unaligned address");

	/* Testing a NULL address for waiters.uaddr */
	waitv[0].uaddr = 0x00000000;

	init_timeout(&to);

	res = tst_futex_waitv(waitv, numfutex, 0, &to, CLOCK_MONOTONIC);
	if (res == EINVAL)
		tst_res(TFAIL, "futex_waitv private returned: %d %s", res, tst_strerrno(res));
	else
		tst_res(TPASS, "futex_waitv NULL address in waitv.uaddr");

	/* Testing a NULL address for *waiters */
	init_timeout(&to);

	res = tst_futex_waitv(NULL, numfutex, 0, &to, CLOCK_MONOTONIC);
	if (res == EINVAL)
		tst_res(TFAIL, "futex_waitv private returned: %d %s", res, tst_strerrno(res));
	else
		tst_res(TPASS, "futex_waitv NULL address in *waiters");

	/* Testing an invalid clockid */
	init_timeout(&to);

	res = tst_futex_waitv(NULL, numfutex, 0, &to, CLOCK_TAI);
	if (res == EINVAL)
		tst_res(TFAIL, "futex_waitv private returned: %d %s", res, tst_strerrno(res));
	else
		tst_res(TPASS, "futex_waitv invalid clockid");
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
