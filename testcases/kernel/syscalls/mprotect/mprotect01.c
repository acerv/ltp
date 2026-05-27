// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`mprotect(2)` fails with the expected errno for
 * various error conditions:
 *
 * - ENOMEM when address is NULL and length exceeds the page size
 * - EINVAL when address is not page-aligned
 * - EACCES when setting PROT_WRITE on a read-only mmap of /dev/zero
 */

#include <sys/mman.h>

#include "tst_test.h"

static size_t page_sz;
static void *addr_base;
static void *addr_unaligned;
static void *addr_mmap;
static int fd = -1;

static struct tcase {
	void **addr;
	int prot;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{NULL, PROT_READ, ENOMEM, "NULL addr"},
	{&addr_unaligned, PROT_READ, EINVAL, "unaligned addr"},
	{&addr_mmap, PROT_WRITE, EACCES, "PROT_WRITE on read-only mmap"},
};

static void setup(void)
{
	page_sz = getpagesize();

	addr_base = SAFE_MALLOC(page_sz);
	addr_unaligned = addr_base + 1;

	fd = SAFE_OPEN("/dev/zero", O_RDONLY);
	addr_mmap = SAFE_MMAP(NULL, page_sz, PROT_READ, MAP_SHARED, fd, 0);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];
	void *addr = tc->addr ? *tc->addr : NULL;
	size_t len = (tc->exp_errno == ENOMEM) ? page_sz + 1 : page_sz;

	TST_EXP_FAIL(mprotect(addr, len, tc->prot), tc->exp_errno,
			"%s", tc->desc);
}

static void cleanup(void)
{
	if (addr_mmap)
		SAFE_MUNMAP(addr_mmap, page_sz);

	free(addr_base);

	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
};
