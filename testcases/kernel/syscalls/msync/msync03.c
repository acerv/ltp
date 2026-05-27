// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Copyright (c) 2014 Fujitsu Ltd.
 * Author: Xiaoguang Wang <wangxg.fnst@cn.fujitsu.com>
 */

/*\
 * Verify that :manpage:`msync(2)` fails with the expected errno for
 * various invalid conditions:
 *
 * - EBUSY with MS_INVALIDATE on MAP_LOCKED memory
 * - EINVAL with conflicting MS_ASYNC | MS_SYNC flags
 * - EINVAL with invalid flags
 * - EINVAL with non-page-aligned address
 * - EINVAL with address outside valid range
 * - ENOMEM with unmapped memory
 */

#include <sys/mman.h>
#include <sys/resource.h>

#include "tst_test.h"

#define INV_SYNC	-1
#define TEMPFILE	"msync_file"
#define BUF_SIZE	256

static int fd = -1;
static char *addr1;
static char *addr2;
static char *addr3;
static char *addr4;

static size_t page_sz;

static struct tcase {
	char **addr;
	int flags;
	int exp_errno;
} tcases[] = {
	{&addr1, MS_INVALIDATE, EBUSY},
	{&addr1, MS_ASYNC | MS_SYNC, EINVAL},
	{&addr1, INV_SYNC, EINVAL},
	{&addr2, MS_SYNC, EINVAL},
	{&addr3, MS_SYNC, EINVAL},
	{&addr4, MS_SYNC, ENOMEM},
};

static void setup(void)
{
	size_t nwrite = 0;
	char write_buf[BUF_SIZE];
	struct rlimit rl;

	page_sz = getpagesize();

	fd = SAFE_OPEN(TEMPFILE, O_RDWR | O_CREAT, 0666);

	memset(write_buf, 'a', BUF_SIZE);
	while (nwrite < page_sz) {
		SAFE_WRITE(SAFE_WRITE_ALL, fd, write_buf, BUF_SIZE);
		nwrite += BUF_SIZE;
	}

	addr1 = SAFE_MMAP(NULL, page_sz, PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_LOCKED, fd, 0);

	addr2 = addr1 + 1;

	SAFE_GETRLIMIT(RLIMIT_DATA, &rl);
	addr3 = (char *)rl.rlim_max;

	addr4 = sbrk(0) + (4 * page_sz);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	TST_EXP_FAIL(msync(*tc->addr, page_sz, tc->flags), tc->exp_errno);
}

static void cleanup(void)
{
	if (addr1)
		SAFE_MUNMAP(addr1, page_sz);

	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.needs_root = 1,
};
