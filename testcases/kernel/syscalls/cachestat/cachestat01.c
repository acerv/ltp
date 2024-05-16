// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that cachestat() syscall is properly counting cached pages
 * written inside a file. If storage device synchronization is requested, test
 * will check if the number of dirty pages is zero.
 *
 * [Algorithm]
 *
 * * create a file with specific amount of pages
 * ** synchronize storage device, if needed
 * * monitor file with cachestat()
 * * check if the right amount of pages have been moved into cache
 * ** if storage device synchronization is requested, check that dirty pages is
 *    zero
 */

#include "cachestat.h"

#define MNTPOINT "mntpoint"
#define FILENAME MNTPOINT "/myfile.bin"
#define NUMPAGES 32

static char *data;
static int file_size;
static struct cachestat *cs;
static struct cachestat_range *cs_range;
static char *run_fsync;

static void run(void)
{
	int fd;

	memset(cs, 0, sizeof(struct cachestat));

	fd = SAFE_OPEN(FILENAME, O_RDWR | O_CREAT, 0600);
	SAFE_WRITE(0, fd, data, file_size);

	if (run_fsync)
		fsync(fd);

	TST_EXP_PASS(cachestat(fd, cs_range, cs, 0));
	print_cachestat(cs);

	TST_EXP_EQ_LI(cs->nr_cache + cs->nr_evicted, NUMPAGES);

	if (run_fsync)
		TST_EXP_EQ_LI(cs->nr_dirty, 0);

	SAFE_CLOSE(fd);
	SAFE_UNLINK(FILENAME);
}

static void setup(void)
{
	int page_size;

	page_size = (int)sysconf(_SC_PAGESIZE);
	file_size = page_size * NUMPAGES;

	data = SAFE_MALLOC(file_size);
	memset(data, 'a', file_size);

	cs_range->off = 0;
	cs_range->len = file_size;
}

static void cleanup(void)
{
	if (data)
		free(data);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.min_kver = "6.5",
	.mount_device = 1,
	.mntpoint = MNTPOINT,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const []) {
		"fuse",
		"tmpfs",
		NULL
	},
	.bufs = (struct tst_buffers []) {
		{&cs, .size = sizeof(struct cachestat)},
		{&cs_range, .size = sizeof(struct cachestat_range)},
		{}
	},
	.options = (struct tst_option[]) {
		{"s", &run_fsync, "Synchronize file with storage device"},
		{},
	},
};
