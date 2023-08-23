// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Spawn child inside cgroup and set max memory. Allocate anonymous memory
 * pages inside child and deactivate them with MADV_COLD. Then apply memory
 * pressure and check if memory pages have been swapped out.
 *
 * The advice might be ignored for some pages in the range when it is
 * not applicable, so test passes if swap memory increases after
 * reclaiming memory with MADV_COLD.
 */

#define _GNU_SOURCE

#include <sys/mman.h>
#include "tst_test.h"
#include "lapi/mmap.h"
#include "lapi/syscalls.h"
#include "cma.h"

#define MEM_LIMIT	(50 * 1024 * 1024)
#define MEM_CHILD	(10 * 1024 * 1024)
#define MEM_PRESS	MEM_LIMIT - (MEM_CHILD / 2)

static void **data_ptr;
static size_t *cswap;

static void child_alloc(void)
{
	char *ptr;
	char *data;
	size_t cmem;
	int freed = 1;
	struct addr_mapping map_before;
	struct addr_mapping map_after;

	tst_res(TINFO, "Memory limit: %d bytes", MEM_LIMIT);

	SAFE_CG_PRINTF(tst_cg, "cgroup.procs", "%d", getpid());
	SAFE_CG_PRINTF(tst_cg, "memory.high", "%d", MEM_LIMIT);

	tst_res(TINFO, "Allocate memory: %d bytes", MEM_CHILD);

	*data_ptr = SAFE_MMAP(NULL, MEM_CHILD,
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	memset(*data_ptr, 'a', MEM_CHILD);

	read_address_mapping((unsigned long)*data_ptr, &map_before);

	SAFE_CG_SCANF(tst_cg, "memory.current", "%zu", &cmem);
	tst_res(TINFO, "Allocated %lu / %d bytes", cmem, MEM_LIMIT);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	tst_res(TINFO, "Apply memory pressure: %d bytes", MEM_PRESS);

	data = SAFE_MMAP(NULL, MEM_PRESS,
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	memset(data, 'b', MEM_PRESS);
	SAFE_MUNMAP(data, MEM_PRESS);

	SAFE_CG_SCANF(tst_cg, "memory.swap.current", "%zu", cswap);
	tst_res(TINFO, "Swap now contains %lu bytes", *cswap);

	for (ptr = *data_ptr; *ptr != '\0'; ptr++) {
		if (*ptr == 'a') {
			freed = 0;
			break;
		}
	}

	if (freed) {
		tst_res(TFAIL, "Memory has been freed");
		return;
	}

	read_address_mapping((unsigned long)*data_ptr, &map_after);

	SAFE_MUNMAP(*data_ptr, MEM_CHILD);

	TST_EXP_EXPR(map_before.swap < map_after.swap,
		"Memory has been swapped out");

	TST_CHECKPOINT_WAKE(0);
}

static void setup(void)
{
	struct sysinfo sys_buf_start;

	sysinfo(&sys_buf_start);
	if (sys_buf_start.freeram < MEM_LIMIT)
		tst_brk(TCONF, "System RAM is too small (%d bytes needed)", MEM_LIMIT);

	if (sys_buf_start.freeswap < MEM_LIMIT)
		tst_brk(TCONF, "System swap is too small (%d bytes needed)", MEM_LIMIT);

	data_ptr = SAFE_MMAP(NULL, sizeof(void *),
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	cswap = SAFE_MMAP(NULL, sizeof(size_t),
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	if (cswap)
		SAFE_MUNMAP(cswap, sizeof(size_t));

	if (*data_ptr)
		SAFE_MUNMAP(*data_ptr, MEM_CHILD);

	if (data_ptr)
		SAFE_MUNMAP(data_ptr, sizeof(void *));
}

static void run(void)
{
	int ret;
	int pidfd;
	pid_t pid_alloc;
	struct iovec vec;
	int mem_normal = 0;
	int mem_madv_cold = 0;

	/* read how much swap is freed up inside a container after pressure */
	pid_alloc = SAFE_FORK();
	if (!pid_alloc) {
		child_alloc();
		return;
	}

	TST_CHECKPOINT_WAIT(0);
	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	mem_normal = *cswap;

	/* read how much swap we freed up after applying MADV_COLD and pressure */
	pid_alloc = SAFE_FORK();
	if (!pid_alloc) {
		child_alloc();
		return;
	}

	TST_CHECKPOINT_WAIT(0);

	tst_res(TINFO, "Advise memory with MADV_COLD rule");

	pidfd = SAFE_PIDFD_OPEN(pid_alloc, 0);

	vec.iov_base = *data_ptr;
	vec.iov_len = MEM_CHILD;

	ret = TST_EXP_POSITIVE(
		tst_syscall(
			__NR_process_madvise,
			pidfd,
			&vec,
			1UL,
			MADV_COLD,
			0UL)
	);

	if (ret != MEM_CHILD)
		tst_brk(TBROK, "process_madvise reclaimed only %d bytes", ret);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	mem_madv_cold = *cswap;

	TST_EXP_EXPR(mem_madv_cold > mem_normal,
		"Memory advised with MADV_COLD swapped more (%d > %d)",
		mem_madv_cold,
		mem_normal);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1,
	.min_kver = "5.10",
	.needs_checkpoints = 1,
	.needs_cgroup_ver = TST_CG_V2,
	.needs_cgroup_ctrls = (const char *const []){ "memory", NULL },
};
