// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2008 Vijay Kumar B. <vijaykumar@bravegnu.org>
 * Copyright (c) International Business Machines Corp., 2001
 */

/*\
 * Verify that :manpage:`move_pages(2)` correctly retrieves the NUMA node
 * of allocated pages.
 *
 * [Algorithm]
 *
 * - Allocate pages across NUMA nodes in an interleaved fashion.
 * - Use move_pages() with NULL nodes to retrieve the NUMA node of each page.
 * - Check if the reported NUMA nodes match the expected allocation.
 */

#include <errno.h>
#include "tst_test.h"
#include "move_pages_support.h"

#ifdef HAVE_NUMA_V2

#define TEST_PAGES 2
#define TEST_NODES TEST_PAGES

static void run(void)
{
	void *pages[TEST_PAGES] = { 0 };
	int status[TEST_PAGES];
	int ret;

	ret = alloc_pages_linear(pages, TEST_PAGES);
	if (ret == -1)
		tst_brk(TBROK, "alloc_pages_linear failed");

	ret = numa_move_pages(0, TEST_PAGES, pages, NULL, status, 0);
	if (ret < 0) {
		tst_res(TFAIL | TERRNO, "move_pages failed");
		free_pages(pages, TEST_PAGES);
		return;
	} else if (ret > 0) {
		tst_res(TINFO, "move_pages() returned %d", ret);
	}

	verify_pages_linear(pages, status, TEST_PAGES);

	free_pages(pages, TEST_PAGES);
}

static void setup(void)
{
	check_config(TEST_NODES);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
};

#else
TST_TEST_TCONF("test requires NUMA support");
#endif
