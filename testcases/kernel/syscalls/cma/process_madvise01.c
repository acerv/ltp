// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Compare normal mode with MADV_COLD advise under memory pressure a fixed
 * amount of times. If MADV_COLD swapped more times then normal mode, we can
 * consider test passing.
 */

#define _GNU_SOURCE

#include <sys/mman.h>
#include "tst_test.h"
#include "lapi/mmap.h"
#include "lapi/syscalls.h"
#include "cma.h"

#define MEASUREMENTS	100
#define MEM_LIMIT	(10 * TST_MB)
#define MEM_CHILD	(3 * TST_MB)
#define MEM_SWAP	(MEM_LIMIT * 2)

static struct tst_cg_group *cg_mem;
static void **data_ptr;

static void apply_memory_pressure(void)
{
	int i;
	char *ptr;
	int page_size;
	int swapped = 0;

	page_size = getpagesize();

	while (swapped <= MEM_CHILD) {
		ptr = SAFE_MMAP(NULL, 500 * page_size,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		for (i = 0; i < 500; i++)
			ptr[i * page_size] = 'p';

		SAFE_CG_LINES_SCANF(cg_mem, "memory.stat", "swap %d", &swapped);
	}
}

static void child_alloc(struct tst_cg_group *cg)
{
	pid_t pid;
	int memory;

	SAFE_CG_PRINTF(cg_mem, "cgroup.procs", "%d", getpid());

	*data_ptr = SAFE_MMAP(NULL, MEM_CHILD,
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	memset(*data_ptr, 'a', MEM_CHILD);

	SAFE_CG_SCANF(cg, "memory.current", "%d", &memory);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	pid = SAFE_FORK();
	if (!pid) {
		apply_memory_pressure();
		return;
	}

	SAFE_WAITPID(pid, NULL, 0);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_MUNMAP(*data_ptr, MEM_CHILD);
}

static int check_normal(void)
{
	int swap;
	int swap_after;
	int swap_before;

	TST_CHECKPOINT_WAIT(0);

	SAFE_CG_LINES_SCANF(cg_mem, "memory.stat", "swap %d", &swap_before);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_CG_LINES_SCANF(cg_mem, "memory.stat", "swap %d", &swap_after);

	swap = swap_after - swap_before;

	TST_CHECKPOINT_WAKE(0);

	tst_res(TINFO, "Normal advise swapped %d bytes", swap);

	return swap;
}

static int check_cold(int pid)
{
	int ret;
	int swap;
	int pidfd;
	int swap_after;
	int swap_before;
	struct iovec vec;

	TST_CHECKPOINT_WAIT(0);

	SAFE_CG_LINES_SCANF(cg_mem, "memory.stat", "swap %d", &swap_before);

	pidfd = SAFE_PIDFD_OPEN(pid, 0);

	vec.iov_base = *data_ptr;
	vec.iov_len = MEM_CHILD;

	ret = tst_syscall(__NR_process_madvise, pidfd, &vec, 1UL,
			MADV_COLD, 0UL);

	if (ret == -1)
		tst_brk(TBROK | TERRNO, "process_madvise failed");

	if (ret != MEM_CHILD)
		tst_brk(TBROK, "process_madvise reclaimed only %d bytes", ret);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_CG_LINES_SCANF(cg_mem, "memory.stat", "swap %d", &swap_after);

	swap = swap_after - swap_before;

	TST_CHECKPOINT_WAKE(0);

	tst_res(TINFO, "MADV_COLD advise swapped %d bytes", swap);

	return swap;
}

static void setup(void)
{
	data_ptr = SAFE_MMAP(NULL, sizeof(void *),
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	cg_mem = tst_cg_group_mk(tst_cg, "madv_cold");
	SAFE_CG_PRINTF(cg_mem, "memory.max", "%d", MEM_LIMIT);
	SAFE_CG_PRINTF(cg_mem, "memory.swap.max", "%d", MEM_SWAP);
	SAFE_CG_PRINT(cg_mem, "memory.swappiness", "10");
}

static void cleanup(void)
{
	if (cg_mem)
		cg_mem = tst_cg_group_rm(cg_mem);

	if (*data_ptr)
		SAFE_MUNMAP(*data_ptr, MEM_CHILD);

	if (data_ptr)
		SAFE_MUNMAP(data_ptr, sizeof(void *));	
}

static void run(void)
{
	int i;
	pid_t pid;
	int swap_norm;
	int swap_cold;
	int passed = 0;
	int failed = 0;

	for (i = 0; i < MEASUREMENTS; i++) {
		pid = SAFE_FORK();
		if (!pid) {
			child_alloc(cg_mem);
			return;
		}

		swap_norm = check_normal();

		SAFE_WAITPID(pid, NULL, 0);

		pid = SAFE_FORK();
		if (!pid) {
			child_alloc(cg_mem);
			return;
		}

		swap_cold = check_cold(pid);

		SAFE_WAITPID(pid, NULL, 0);

		if (swap_cold > swap_norm)
			passed++;
		else
			failed++;
	}

	if (passed > failed)
		tst_res(TPASS, "MADV_COLD swapped %d/%d times more than normal mode", passed, MEASUREMENTS);
	else
		tst_res(TPASS, "MADV_COLD swapped %d/%d times less than normal mode", failed, MEASUREMENTS);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1,
	.min_kver = "5.10",
	.needs_checkpoints = 1,
	.needs_cgroup_ctrls = (const char *const []){ "memory", NULL },
};
