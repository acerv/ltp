// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Allocate file-backed memory pages inside child and reclaim it with
 * MADV_PAGEOUT. Then check if memory pages have been written back to the
 * backing storage.
 */

#define _GNU_SOURCE

#include <sys/mman.h>
#include "tst_test.h"
#include "lapi/mmap.h"
#include "lapi/syscalls.h"
#include "cma.h"

#define MNTPOINT	"mnt_point"
#define FNAME		MNTPOINT"/test"
#define FILE_SIZE_MB	600
#define FILE_SIZE	(FILE_SIZE_MB * TST_MB)
#define MODE		0644

static void **data_ptr;

static void child_alloc(void)
{
	int i;
	int fd;
	char *ptr;
	int page_size;
	unsigned long written;

	page_size = getpagesize();

	tst_res(TINFO, "Allocate file-backed memory");

	fd = SAFE_OPEN(FNAME, O_CREAT | O_RDWR, MODE);

	*data_ptr = SAFE_MMAP(NULL, FILE_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE, fd, 0);

	tst_res(TINFO, "Dirty memory");

	SAFE_TRUNCATE(FNAME, FILE_SIZE);
	for (i = 0, ptr = *data_ptr; i < FILE_SIZE; i += (2 * page_size))
		ptr[i] = 'a';

	tst_dev_sync(fd);
	tst_dev_bytes_written(tst_device->dev);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	tst_dev_sync(fd);
	written = tst_dev_bytes_written(tst_device->dev);

	SAFE_MUNMAP(*data_ptr, FILE_SIZE);
	*data_ptr = NULL;

	SAFE_CLOSE(fd);

	tst_res(TINFO, "After MADV_PAGEOUT written %li bytes", written);

	if (written >= FILE_SIZE)
		tst_res(TPASS, "Memory has been written back to the backing-storage");
	else
		tst_res(TFAIL, "Memory has not been written to the backing-storage");
}

static void setup(void)
{
	data_ptr = SAFE_MMAP(NULL, sizeof(void *),
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	if (*data_ptr)
		SAFE_MUNMAP(*data_ptr, FILE_SIZE);

	if (data_ptr)
		SAFE_MUNMAP(data_ptr, sizeof(void *));
}

static void run(void)
{
	int ret;
	int pidfd;
	pid_t pid_alloc;
	struct iovec vec;

	pid_alloc = SAFE_FORK();
	if (!pid_alloc) {
		child_alloc();
		return;
	}

	TST_CHECKPOINT_WAIT2(0, 60000);

	pidfd = SAFE_PIDFD_OPEN(pid_alloc, 0);

	vec.iov_base = *data_ptr;
	vec.iov_len = FILE_SIZE;

	tst_res(TINFO, "Apply MADV_PAGEOUT advise rule");

	ret = tst_syscall(__NR_process_madvise, pidfd, &vec, 1UL,
			MADV_PAGEOUT, 0UL);

	if (ret == -1)
		tst_brk(TBROK | TERRNO, "process_madvise failed");

	if (ret != FILE_SIZE)
		tst_brk(TBROK, "process_madvise reclaimed only %d bytes", ret);

	TST_CHECKPOINT_WAKE(0);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1,
	.min_kver = "5.10",
	.needs_checkpoints = 1,
	.mount_device = 1,
	.mntpoint = MNTPOINT,
	.dev_fs_type = "btrfs",
	.skip_filesystems = (const char *[]) {
		"tmpfs",
		NULL
	}
};
