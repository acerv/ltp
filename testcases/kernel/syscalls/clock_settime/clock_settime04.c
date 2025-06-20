// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify that changing the value of the CLOCK_MONOTONIC clock via
 * clock_settime() shall have no effect on a thread that is blocked on
 * absolute/relative clock_nanosleep().
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

static void child_nanosleep(struct time64_variants *tv, const int flags)
{
	long long delta;

	TEST(tv->clock_gettime(CLOCK_MONOTONIC, tst_ts_get(begin)));
	if (TST_RET == -1)
		tst_brk(TBROK | TERRNO, "clock_gettime() error");

	if (flags & TIMER_ABSTIME) {
		tst_res(TINFO, "Using absolute time sleep");

		*sleep_child = tst_ts_add_us(*begin, CHILD_SLEEP_US);
	} else {
		tst_res(TINFO, "Using relative time sleep");

		tst_ts_set_sec(sleep_child, 0);
		tst_ts_set_nsec(sleep_child, 0);

		*sleep_child = tst_ts_add_us(*sleep_child, CHILD_SLEEP_US);
	}

	tst_res(TINFO, "begin: %lld %lld", tst_ts_get_nsec(*begin), tst_ts_get_sec(*begin));

	TST_CHECKPOINT_WAKE(0);

	TEST(tv->clock_nanosleep(CLOCK_MONOTONIC, flags, tst_ts_get(sleep_child), NULL));
	if (TST_RET)
		tst_brk(TBROK | TERRNO, "clock_nanosleep() error");

	TEST(tv->clock_gettime(CLOCK_MONOTONIC, tst_ts_get(end)));
	if (TST_RET == -1)
		tst_brk(TBROK | TERRNO, "clock_gettime() error");

	if (tst_ts_lt(*end, *begin)) {
		tst_res(TFAIL, "clock_settime() didn't sleep enough. "
			"begin=%lld >= end=%lld",
			tst_ts_to_ms(*begin),
			tst_ts_to_ms(*end));
		return;
	}

	delta = tst_ts_abs_diff_us(*begin, *end);
	if (delta > DELTA_US) {
		tst_res(TFAIL, "parent clock_settime() affected child sleep. "
			"begin: %lld ms , end: %lld ms",
			tst_ts_to_ms(*begin),
			tst_ts_to_ms(*end));
		return;
	}

	tst_res(TPASS, "parent clock_settime() didn't affect child sleep "
		"(delta time: %lld us)", delta);
}

static void run(unsigned int tc_index)
{
	struct time64_variants *tv = &variants[tst_variant];

	if (!SAFE_FORK()) {
		child_nanosleep(tv, tc_index ? TIMER_ABSTIME : 0);
		exit(0);
	}

	TST_CHECKPOINT_WAIT(0);

	tst_res(TINFO, "begin: %lld %lld", tst_ts_get_nsec(*begin), tst_ts_get_sec(*begin));

	TEST(tv->clock_nanosleep(CLOCK_MONOTONIC, 0, tst_ts_get(sleep_parent), NULL));
	if (TST_RET)
		tst_brk(TBROK | TERRNO, "clock_nanosleep() error");

	TEST(tv->clock_settime(CLOCK_MONOTONIC, tst_ts_get(begin)));
	if (TST_RET)
		tst_brk(TBROK | TERRNO, "clock_settime() error");
}

static void setup(void)
{
	begin->type = end->type = sleep_child->type = sleep_parent->type =
		variants[tst_variant].ts_type;

	tst_ts_set_sec(sleep_parent, 0);
	tst_ts_set_nsec(sleep_parent, 0);

	*sleep_parent = tst_ts_add_us(*sleep_parent, PARENT_SLEEP_US);

	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);
}

static struct tst_test test = {
	.test = run,
	.setup = setup,
	.tcnt = 2,
	.needs_root = 1,
	.forks_child = 1,
	.needs_checkpoints = 1,
	.restore_wallclock = 1,
	.test_variants = ARRAY_SIZE(variants),
	.bufs = (struct tst_buffers []) {
		{&begin, .size = sizeof(struct tst_ts)},
		{&sleep_child, .size = sizeof(struct tst_ts)},
		{&sleep_parent, .size = sizeof(struct tst_ts)},
		{&end, .size = sizeof(struct tst_ts)},
		{},
	}
};
