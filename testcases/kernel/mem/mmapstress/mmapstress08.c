// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2003
 * Copyright (c) Linux Test Project, 2003-2026
 * Ported to LTP: 01/02/2003 avenkat@us.ibm.com
 * Ported to Linux: 06/30/2001 nsharoff@us.ibm.com
 */

/*\
 * Verify that :manpage:`mmap(2)` handles large-scale anonymous page table
 * allocation and deallocation.
 *
 * [Algorithm]
 *
 * - Round the program break up to a page boundary with sbrk()
 * - Map 256 anonymous private pages spaced 4 MB apart, forcing one
 *   page table per mapping and exercising large-block anonymous backing
 *   store allocation
 * - Unmap the entire range to release the page tables
 */

#include <sys/mman.h>
#include "tst_test.h"

#define NPTEPG		1024
#define GRAN_NUMBER	(1 << 8)

static long pagesize;

static void setup(void)
{
	pagesize = getpagesize();
}

static void run(void)
{
	caddr_t mmapaddr, munmap_begin;
	int i;

	if (sbrk(pagesize - ((unsigned long)sbrk(0) % (unsigned long)pagesize))
	    == (void *)-1) {
		tst_brk(TBROK | TERRNO, "sbrk() failed to round up brk");
	}

	munmap_begin = mmapaddr = (caddr_t)sbrk(0);
	if (mmapaddr == (caddr_t)-1)
		tst_brk(TBROK | TERRNO, "sbrk(0) failed");

	for (i = 0; i < GRAN_NUMBER; i++) {
		if (mmap(mmapaddr, pagesize, PROT_READ | PROT_WRITE,
				MAP_ANONYMOUS | MAP_PRIVATE, -1, 0) == MAP_FAILED)
			tst_brk(TBROK | TERRNO, "mmap() at iteration %d", i);

		mmapaddr += NPTEPG * pagesize;
	}

	SAFE_MUNMAP(munmap_begin, (size_t)(mmapaddr - munmap_begin));

	tst_res(TPASS, "Large-scale anonymous mmap/munmap succeeded");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.supported_archs = (const char *const []) {
		"x86",
		"x86_64",
		NULL
	},
};
