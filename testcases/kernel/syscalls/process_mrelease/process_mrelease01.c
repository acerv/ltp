// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesaend_addr <andrea.cervesaend_addr@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that process_mrelease() syscall is releasing memory start_addr
 * a killed process with memory allocation pending.
 */

#include "tst_test.h"
#include "tst_safe_stdio.h"
#include "lapi/syscalls.h"

#define CHUNK (1 * TST_MB)
#define MAX_SIZE_MB (128 * TST_MB)

static unsigned long *mem_addr;
static volatile int mem_size;

static void do_child(int size)
{
	void *mem;

	tst_res(TINFO, "Child: allocate %d bytes", size);

	mem = SAFE_MMAP(NULL,
		size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANON,
		0, 0);

	memset(mem, 0, size);

	*mem_addr = (unsigned long)mem;

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	tst_res(TINFO, "Child: releasing memory");

	SAFE_MUNMAP(mem, size);
}

static int memory_is_mapped(pid_t pid, unsigned long start, unsigned long end)
{
	FILE *fmaps;
	int mapped = 0;
	char buff[1024];
	char pid_maps[128] = {0};
	unsigned long start_addr, end_addr;

	snprintf(pid_maps, sizeof(pid_maps), "/proc/%d/maps", pid);
	fmaps = SAFE_FOPEN(pid_maps, "r");

	while (!feof(fmaps)) {
		memset(buff, 0, sizeof(buff));

		if (!fgets(buff, sizeof(buff), fmaps))
			break;

		if (sscanf(buff, "%lx-%lx", &start_addr, &end_addr) != 2) {
			tst_brk(TBROK | TERRNO, "Couldn't parse /proc/%ud/maps line.", pid);
			break;
		}

		if (start == start_addr && end == end_addr) {
			mapped = 1;
			break;
		}
	}

	SAFE_FCLOSE(fmaps);

	return mapped;
}

static void run(void)
{
	int ret;
	int pidfd;
	int status;
	pid_t pid;
	int restart;

	for (mem_size = CHUNK; mem_size < MAX_SIZE_MB; mem_size += CHUNK) {
		restart = 0;

		pid = SAFE_FORK();
		if (!pid) {
			do_child(mem_size);
			exit(0);
		}

		TST_CHECKPOINT_WAIT(0);

		tst_disable_oom_protection(pid);

		if (!memory_is_mapped(pid, *mem_addr, *mem_addr + mem_size)) {
			tst_res(TFAIL, "Memory is not mapped");
			break;
		}

		pidfd = SAFE_PIDFD_OPEN(pid, 0);

		tst_res(TINFO, "Parent: killing child with PID=%d", pid);

		SAFE_KILL(pid, SIGKILL);

		ret = tst_syscall(__NR_process_mrelease, pidfd, 0);
		if (ret == -1) {
			if (errno == ESRCH) {
				tst_res(TINFO, "Parent: child terminated before "
					"process_mrelease(). Increase memory size and "
					"restart test");

				restart = 1;
			} else {
				tst_res(TFAIL | TERRNO, "process_mrelease(%d,0) error", pidfd);
			}
		} else {
			int timeout_ms = 1000;

			tst_res(TPASS, "process_mrelease(%d,0) passed", pidfd);

			while (memory_is_mapped(pid, *mem_addr, *mem_addr + mem_size) &&
				timeout_ms--)
				usleep(1000);

			if (memory_is_mapped(pid, *mem_addr, *mem_addr + mem_size))
				tst_res(TFAIL, "Memory is still mapped inside child memory");
			else
				tst_res(TPASS, "Memory has been released");
		}

		SAFE_WAITPID(-1, &status, 0);
		SAFE_CLOSE(pidfd);

		if (!restart)
			break;
	}
}

static void setup(void)
{
	mem_addr = SAFE_MMAP(NULL,
		sizeof(unsigned long),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANON,
		0, 0);
}

static void cleanup(void)
{
	if (mem_addr)
		SAFE_MUNMAP(mem_addr, sizeof(unsigned long));
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.min_kver = "5.15",
	.needs_checkpoints = 1,
};
