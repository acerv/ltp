// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that process_mrelease() syscall is releasing memory from
 * a killed process with memory allocation pending.
 */

#include "tst_test.h"
#include "lapi/syscalls.h"

#define CHUNK (1 * TST_MB)
#define MAX_SIZE_MB (128 * TST_MB)

static void *mem;
static volatile int *mem_size;

static void do_child(int size)
{
	tst_res(TINFO, "Child: allocate %d bytes", size);

	mem = SAFE_MMAP(NULL,
		size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANON,
		0, 0);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	tst_res(TINFO, "Child: releasing memory");

	SAFE_MUNMAP(mem, size);
}

static void run(void)
{
	int ret;
	int pidfd;
	int status;
	pid_t pid;
	volatile int restart;

	for (*mem_size = CHUNK; *mem_size < MAX_SIZE_MB; *mem_size += CHUNK) {
		restart = 0;

		pid = SAFE_FORK();
		if (!pid) {
			do_child(*mem_size);
			exit(0);
		}

		TST_CHECKPOINT_WAIT(0);

		tst_disable_oom_protection(pid);

		pidfd = SAFE_PIDFD_OPEN(pid, 0);

		tst_res(TINFO, "Parent: killing child with PID=%d", pid);

		SAFE_KILL(pid, SIGKILL);

		ret = tst_syscall(__NR_process_mrelease, pidfd, 0);
		if (ret == -1) {
			if (errno == ESRCH) {
				tst_res(TINFO, "Parent: child terminated before process_mrelease()."
					" Increase memory size and restart test");

				restart = 1;
			} else {
				tst_res(TFAIL | TERRNO, "process_mrelease(%d,0) error", pidfd);
			}
		} else {
			tst_res(TPASS, "process_mrelease(%d,0) passed", pidfd);
		}

		SAFE_WAITPID(-1, &status, 0);
		SAFE_CLOSE(pidfd);

		if (!restart)
			break;
	}
}

static void setup(void)
{
	mem_size = SAFE_MMAP(
		NULL,
		sizeof(int),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);
}

static void cleanup(void)
{
	if (mem)
		SAFE_MUNMAP(mem, *mem_size);

	if (mem_size)
		SAFE_MUNMAP((void *)mem_size, sizeof(int));
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.needs_root = 1,
	.forks_child = 1,
	.min_kver = "5.15",
	.needs_checkpoints = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_MMU=y",
	},
};
