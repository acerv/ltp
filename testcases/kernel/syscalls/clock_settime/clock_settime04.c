// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify that changing the value of the CLOCK_REALTIME clock via
 * clock_settime() shall have no effect on a thread that is blocked on
 * absolute clock_nanosleep().
 */

#include "tst_test.h"
#include "tst_timer.h"
#include "time64_variants.h"

#define SEC_TO_US(x)     (x * 1000 * 1000)

#define CHILD_SLEEP_US   SEC_TO_US(5)
#define PARENT_SLEEP_US  SEC_TO_US(2)
#define DELTA_US         SEC_TO_US(1)

static struct tst_ts *begin, *sleep_child, *sleep_parent, *end;

static struct time64_variants variants[] = {
	{
		.clock_gettime = libc_clock_gettime,
		.clock_settime = libc_clock_settime,
		.clock_nanosleep = libc_clock_nanosleep,
		.ts_type = TST_LIBC_TIMESPEC,
		.desc = "vDSO or syscall with libc spec"
	},

#if (__NR_clock_nanosleep != __LTP__NR_INVALID_SYSCALL)
	{
		.clock_gettime = sys_clock_gettime,
		.clock_settime = sys_clock_settime,
		.clock_nanosleep = sys_clock_nanosleep,
		.ts_type = TST_KERN_OLD_TIMESPEC,
		.desc = "syscall with old kernel spec"
	},
#endif

#if (__NR_clock_nanosleep_time64 != __LTP__NR_INVALID_SYSCALL)
	{
		.clock_gettime = sys_clock_gettime64,
		.clock_settime = sys_clock_settime64,
		.clock_nanosleep = sys_clock_nanosleep64,
		.ts_type = TST_KERN_TIMESPEC,
		.desc = "syscall time64 with kernel spec"
	},
#endif
};

static void child_nanosleep(struct time64_variants *tv, unsigned int tc_index)
{
	long long delta;

	if (tc_index) {
		tst_res(TINFO, "Using absolute time sleep");

		*sleep_child = tst_ts_add_us(*begin, CHILD_SLEEP_US);

		TST_CHECKPOINT_WAKE(0);

		TEST(tv->clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
				  tst_ts_get(sleep_child), NULL));
		if (TST_RET)
			tst_brk(TBROK | TERRNO, "clock_nanosleep() error");
	} else {
		tst_res(TINFO, "Using relative time sleep");

		tst_ts_set_sec(sleep_child, 0);
		tst_ts_set_nsec(sleep_child, 0);

		*sleep_child = tst_ts_add_us(*sleep_child, CHILD_SLEEP_US);

		TST_CHECKPOINT_WAKE(0);

		TEST(tv->clock_nanosleep(CLOCK_REALTIME, 0,
				tst_ts_get(sleep_child), NULL));
		if (TST_RET)
			tst_brk(TBROK | TERRNO, "clock_nanosleep() error");

		/* normalize to absolute time so we can compare times later on */
		*sleep_child = tst_ts_add_us(*begin, CHILD_SLEEP_US - PARENT_SLEEP_US);
	}

	TEST(tv->clock_gettime(CLOCK_REALTIME, tst_ts_get(end)));
	if (TST_RET == -1)
		tst_brk(TBROK | TERRNO, "clock_gettime() error");

	if (tst_ts_lt(*end, *sleep_child)) {
		tst_res(TFAIL, "clock_settime() didn't sleep enough. "
			"end=%lld < sleep=%lld", tst_ts_get_sec(*end),
			tst_ts_get_sec(*sleep_child));
		return;
	}

	delta = tst_ts_abs_diff_us(*sleep_child, *end);
	if (delta > DELTA_US) {
		tst_res(TFAIL, "clock_settime() affected child sleep. "
			"end=%lld <= sleep=%lld", tst_ts_get_nsec(*end),
			tst_ts_get_nsec(*sleep_child));
		return;
	}

	tst_res(TPASS, "clock_settime() didn't affect child sleep "
		"(delta time: %lld us)", delta);
}

static void run(unsigned int tc_index)
{
	struct time64_variants *tv = &variants[tst_variant];

	TEST(tv->clock_gettime(CLOCK_REALTIME, tst_ts_get(begin)));
	if (TST_RET == -1)
		tst_brk(TBROK | TERRNO, "clock_gettime() error");

	if (!SAFE_FORK()) {
		child_nanosleep(tv, tc_index);
		exit(0);
	}

	*sleep_parent = tst_ts_add_us(*begin, PARENT_SLEEP_US);

	TST_CHECKPOINT_WAIT(0);

	TEST(tv->clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
			  tst_ts_get(sleep_parent), NULL));
	if (TST_RET)
		tst_brk(TBROK | TERRNO, "clock_nanosleep() error");

	TEST(tv->clock_settime(CLOCK_REALTIME, tst_ts_get(begin)));
	if (TST_RET)
		tst_brk(TBROK | TERRNO, "clock_settime() error");

	tst_reap_children();

	/* restore initial clock */
	*end = tst_ts_sub_us(*begin, PARENT_SLEEP_US);

	TEST(tv->clock_settime(CLOCK_REALTIME, tst_ts_get(end)));
	if (TST_RET)
		tst_brk(TBROK | TERRNO, "clock_settime() error");
}

static void setup(void)
{
	begin->type = end->type = sleep_child->type = sleep_parent->type =
		variants[tst_variant].ts_type;

	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);
}

static struct tst_test test = {
	.test = run,
	.setup = setup,
	.tcnt = 2,
	.needs_root = 1,
	.forks_child = 1,
	.needs_checkpoints = 1,
	.test_variants = ARRAY_SIZE(variants),
	.bufs = (struct tst_buffers []) {
		{&begin, .size = sizeof(struct tst_ts)},
		{&sleep_child, .size = sizeof(struct tst_ts)},
		{&sleep_parent, .size = sizeof(struct tst_ts)},
		{&end, .size = sizeof(struct tst_ts)},
		{},
	}
};
