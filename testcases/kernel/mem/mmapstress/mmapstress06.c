// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2003
 * 01/02/2003	Port to LTP	avenkat@us.ibm.com
 * 06/30/2001	Port to Linux	nsharoff@us.ibm.com
 */

/*\
 * Mmap a large shared anonymous region to exercise the kernel mmap
 * handling for regions larger than ANON_GRAN_PAGES_MAX. The test holds
 * the mapping for the duration of its runtime so that it can be run
 * alongside a swap-inducing workload.
 *
 * Verify that :manpage:`mmap(2)` succeeds for a large anonymous shared
 * mapping and that the mapping remains stable while held.
 */

#include <sys/mman.h>
#include <unistd.h>
#include "tst_test.h"

#define ANON_GRAN_PAGES_MAX	(32U)
#define NMFPTEPG		(1024)

static size_t pagesize;
static size_t map_size;

static void setup(void)
{
	pagesize = getpagesize();
	map_size = (ANON_GRAN_PAGES_MAX * NMFPTEPG + 1) * pagesize;
}

static void run(void)
{
	void *addr;

	addr = SAFE_MMAP(NULL, map_size, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_SHARED, -1, 0);

	sleep(tst_remaining_runtime());

	SAFE_MUNMAP(addr, map_size);

	tst_res(TPASS, "large anonymous shared mmap held successfully");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.runtime = 20,
};
