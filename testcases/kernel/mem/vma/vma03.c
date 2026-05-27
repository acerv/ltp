// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012 Red Hat, Inc.
 */

/*\
 * Reproducer for CVE-2011-2496.
 *
 * The normal mmap paths all avoid creating a mapping where the pgoff
 * inside the mapping could wrap around due to overflow. However, an
 * expanding mremap() can take such a non-wrapping mapping and make it
 * bigger and cause a wrapping condition.
 *
 * This test tries to remap a mapping with a new size that would
 * wrap pgoff via :manpage:`mmap2(2)`. It only runs on 32-bit systems
 * with a 32-bit kernel.
 */

#define _GNU_SOURCE
#include <sys/mman.h>
#include <limits.h>

#include "tst_test.h"
#include "lapi/syscalls.h"

#ifdef __NR_mmap2

#define TESTFILE "testfile"

static size_t pgsz;
static int fd = -1;

static void *mmap2(void *addr, size_t length, int prot,
		int flags, int fd, off_t pgoffset)
{
	return (void *)tst_syscall(__NR_mmap2, addr, length, prot,
			flags, fd, pgoffset);
}

static void setup(void)
{
	pgsz = getpagesize();

	fd = SAFE_CREAT(TESTFILE, 0644);
	SAFE_CLOSE(fd);
}

static void run(void)
{
	void *map, *remap;
	off_t pgoff;

	fd = SAFE_OPEN(TESTFILE, O_RDWR);

	pgoff = (ULONG_MAX - 1) & (~((pgsz - 1) >> 12));
	map = mmap2(NULL, pgsz, PROT_READ | PROT_WRITE, MAP_PRIVATE,
			fd, pgoff);
	if (map == MAP_FAILED)
		tst_brk(TBROK | TERRNO, "mmap2");

	remap = mremap(map, pgsz, 2 * pgsz, 0);
	if (remap == MAP_FAILED) {
		if (errno == EINVAL)
			tst_res(TPASS, "mremap failed as expected");
		else
			tst_res(TFAIL | TERRNO, "mremap");
		SAFE_MUNMAP(map, pgsz);
	} else {
		tst_res(TFAIL, "mremap succeeded unexpectedly");
		SAFE_MUNMAP(remap, 2 * pgsz);
	}

	SAFE_CLOSE(fd);
}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.skip_in_compat = 1,
	.tags = (const struct tst_tag[]) {
		{"CVE", "2011-2496"},
		{}
	}
};

#else

TST_TEST_TCONF("__NR_mmap2 is not defined on your system");

#endif
