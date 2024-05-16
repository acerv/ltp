// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that cachestat() syscall is properly counting cached pages
 * written inside a shared memory.
 *
 * [Algorithm]
 *
 * * create a shared memory with a specific amount of pages
 * * monitor file with cachestat()
 * * check if the right amount of pages have been moved into cache
 */

#include "cachestat.h"

#define FILENAME "myfile.bin"
#define NUMPAGES 32

static char *data;
static int file_size;
static struct cachestat *cs;
static struct cachestat_range *cs_range;

static void run(void)
{
	int fd;

	memset(cs, 0, sizeof(struct cachestat));

	fd = shm_open(FILENAME, O_RDWR | O_CREAT, 0600);
	if (fd < 0)
		tst_brk(TBROK | TERRNO, "shm_open error");

	SAFE_FTRUNCATE(fd, file_size);
	SAFE_WRITE(0, fd, data, file_size);

	TST_EXP_PASS(cachestat(fd, cs_range, cs, 0));
	print_cachestat(cs);

	TST_EXP_EQ_LI(cs->nr_cache + cs->nr_evicted, NUMPAGES);

	SAFE_CLOSE(fd);
	shm_unlink(FILENAME);
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
	.bufs = (struct tst_buffers []) {
		{&cs, .size = sizeof(struct cachestat)},
		{&cs_range, .size = sizeof(struct cachestat_range)},
		{}
	},
};
