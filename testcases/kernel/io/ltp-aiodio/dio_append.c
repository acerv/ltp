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
 * Append zeroed data to a file using O_DIRECT while other processes are doing
 * buffered reads and check if the buffer reads always see zero.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "tst_test.h"
#include "common.h"

#define NUM_CHILDREN 16
#define FILE_SIZE (64 * 1024)

static int *run_child;

static void read_eof(const char *filename, size_t bs)
{
	int fd;
	int r;
	char *bufptr;

	while ((fd = open(filename, O_RDONLY, 0666)) < 0)
		usleep(100);

	bufptr = SAFE_MEMALIGN(getpagesize(), bs);

	tst_res(TINFO, "child %i reading file", getpid());
	while (*run_child) {
		off_t offset;
		char *bufoff;

		offset = SAFE_LSEEK(fd, 0, SEEK_END);
		do {
			r = SAFE_READ(0, fd, bufptr, bs);
			if (r > 0) {
				bufoff = check_zero(bufptr, r);
				if (bufoff) {
					tst_res(TINFO, "non-zero read at offset %zu",
						offset + (bufoff - bufptr));
					free(bufptr);
					SAFE_CLOSE(fd);
					return;
				}
				offset += r;
			}
		} while (r > 0);
	}

	free(bufptr);
	SAFE_CLOSE(fd);
}

static void setup(void)
{
	run_child = SAFE_MMAP(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	SAFE_MUNMAP(run_child, sizeof(int));
}

static void run(void)
{
	char *filename = "file";
	int filesize = FILE_SIZE;
	int num_children = NUM_CHILDREN;
	int status;
	int i;

	*run_child = 1;

	for (i = 0; i < num_children; i++) {
		if (!SAFE_FORK()) {
			read_eof(filename, filesize);
			return;
		}
	}

	tst_res(TINFO, "parent append to file");

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
};
