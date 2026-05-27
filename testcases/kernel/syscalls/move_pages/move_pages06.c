// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2008 Vijay Kumar B. <vijaykumar@bravegnu.org>
 * Copyright (c) International Business Machines Corp., 2001
 * Copyright (c) Linux Test Project, 2008-2025
 */

/*\
 * Verify that :manpage:`move_pages(2)` returns -1 with errno set to ENODEV
 * when a non-existent NUMA node is specified as target.
 */

#include "config.h"
#include "tst_test.h"

#ifdef HAVE_NUMA_V2
#include <errno.h>
#include <numaif.h>
#include <numa.h>

#include "move_pages_support.h"
#include "numa_helper.h"

#define TEST_PAGES 2
#define TEST_NODES 2

static void run(void)
{
	void *pages[TEST_PAGES] = { 0 };
	int nodes[TEST_PAGES];
	int status[TEST_PAGES];
	unsigned int i;
	unsigned int from_node;
	unsigned int to_node;
	int ret;

	ret = get_allowed_nodes(NH_MEMS, 1, &from_node);
	if (ret < 0)
		tst_brk(TBROK | TERRNO, "get_allowed_nodes: %d", ret);

	ret = alloc_pages_on_node(pages, TEST_PAGES, from_node);
	if (ret == -1)
		tst_brk(TBROK, "alloc_pages_on_node failed");

	to_node = numa_max_node() + 1;
	for (i = 0; i < TEST_PAGES; i++)
		nodes[i] = to_node;

	TST_EXP_FAIL(numa_move_pages(0, TEST_PAGES, pages, nodes,
			status, MPOL_MF_MOVE), ENODEV);

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
TST_TEST_TCONF("test requires libnuma >= 2");
#endif
