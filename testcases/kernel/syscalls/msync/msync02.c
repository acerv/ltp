// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`msync(2)` succeeds with MS_INVALIDATE on a
 * shared-mapped file region and that data written to the file is visible
 * in the mapped memory.
 */

#include <sys/mman.h>

#include "tst_test.h"

#define TEMPFILE	"msync_file"
#define BUF_SIZE	256

static char *addr;
static size_t page_sz;
static int fd = -1;
static char write_buf[10] = "Testing";

static void setup(void)
{
	size_t c_total = 0, nwrite;
	char tst_buf[BUF_SIZE];

	page_sz = getpagesize();

	fd = SAFE_OPEN(TEMPFILE, O_RDWR | O_CREAT, 0666);

	while (c_total < page_sz) {
		nwrite = SAFE_WRITE(SAFE_WRITE_ANY, fd, tst_buf, sizeof(tst_buf));
		c_total += nwrite;
	}

	addr = SAFE_MMAP(NULL, page_sz, PROT_READ | PROT_WRITE,
			MAP_FILE | MAP_SHARED, fd, 0);

	SAFE_LSEEK(fd, 100, SEEK_SET);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, write_buf, strlen(write_buf));
}

static void run(void)
{
	TST_EXP_PASS(msync(addr, page_sz, MS_INVALIDATE));

	if (!TST_PASS)
		return;

	if (memcmp(addr + 100, write_buf, strlen(write_buf)) != 0)
		tst_res(TFAIL, "memory region contains invalid data");
	else
		tst_res(TPASS, "mapped memory matches written data");
}

static void cleanup(void)
{
	if (addr)
		SAFE_MUNMAP(addr, page_sz);

	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
