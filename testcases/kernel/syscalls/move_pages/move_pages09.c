// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2008 Vijay Kumar B. <vijaykumar@bravegnu.org>
 * Copyright (c) International Business Machines Corp., 2001
 */

/*\
 * Verify that :manpage:`move_pages(2)` succeeds when all pages are already on
 * the requested node.
 *
 * Pass the actual NUMA node number for each page to move_pages() and check
 * that it succeeds.
 */

#include <errno.h>
#include <sys/mman.h>
#include "tst_test.h"
#include "move_pages_support.h"

#ifdef HAVE_NUMA_V2

#define TEST_PAGES 2
#define TEST_NODES 2

static void run(void)
{
	unsigned int i;
	unsigned int from_node;
	int ret;
	void *pages[TEST_PAGES] = { 0 };
	int nodes[TEST_PAGES];
	int status[TEST_PAGES];

	ret = get_allowed_nodes(NH_MEMS, 1, &from_node);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "get_allowed_nodes: %d", ret);

	ret = alloc_pages_on_node(pages, TEST_PAGES, from_node);
	if (ret == -1)
		tst_brk(TBROK, "failed allocating pages on node %u", from_node);

	for (i = 0; i < TEST_PAGES; i++)
		nodes[i] = from_node;

	ret = numa_move_pages(0, TEST_PAGES, pages, nodes,
			status, MPOL_MF_MOVE);

	if (ret >= 0)
		tst_res(TPASS, "move_pages succeeded");
	else
		tst_res(TFAIL | TERRNO, "move_pages failed");

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
TST_TEST_TCONF("test requires libnuma >= 2 and target NUMA hardware");
#endif
