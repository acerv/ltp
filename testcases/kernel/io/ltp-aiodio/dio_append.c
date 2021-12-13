// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2004 Daniel McNeil <daniel@osdl.org>
 *				 2004 Open Source Development Lab
 *				 2004  Marty Ridgeway <mridge@us.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * dio_append - append zeroed data to a file using O_DIRECT while
 *	a 2nd process is doing buffered reads and check if the buffer
 *	reads always see zero.
 */
#define _GNU_SOURCE

#include "tst_test.h"
#include "common.h"

static int *run_child;

static char *str_numchildren;
static char *str_filesize;

static int numchildren;
static long long filesize;

static void setup(void)
{
	numchildren = 16;
	filesize = 64 * 1024;

	if (tst_parse_int(str_numchildren, &numchildren, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of children '%s'", str_numchildren);

	if (tst_parse_filesize(str_filesize, &filesize, 1, LLONG_MAX))
		tst_brk(TBROK, "Invalid file size '%s'", str_filesize);

	run_child = SAFE_MMAP(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	SAFE_MUNMAP(run_child, sizeof(int));
}

static void run(void)
{
	char *filename = "dio_append";
	int status;
	int i;

	*run_child = 1;

	for (i = 0; i < numchildren; i++) {
		if (!SAFE_FORK()) {
			io_read(filename, filesize, run_child);
			return;
		}
	}

	tst_res(TINFO, "Parent append to file");

	io_append(filename, 0, O_DIRECT | O_WRONLY | O_CREAT, filesize, 1000);

	if (SAFE_WAITPID(-1, &status, WNOHANG))
		tst_res(TFAIL, "Non zero bytes read");
	else
		tst_res(TPASS, "All bytes read were zeroed");

	*run_child = 0;
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{"n:", &str_numchildren, "Number of threads (default 1000)"},
		{"s:", &str_filesize, "Size of file (default 64K)"},
		{}
	},
};
