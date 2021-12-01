// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *   Copyright (c) 2004 Daniel McNeil <daniel@osdl.org>
 *                 2004 Open Source Development Lab
 *
 *   Copyright (c) 2004 Marty Ridgeway <mridge@us.ibm.com>
 *
 *   Copyright (c) 2011 Cyril Hrubis <chrubis@suse.cz>
 *
 *   Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Create a sparse file using O_DIRECT while other processes are doing
 * buffered reads and check if the buffer reads always see zero.
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "tst_test.h"
#include "common.h"

#define NUM_CHILDREN 1000
#define FILE_SIZE (100 * 1024 * 1024)
#define WRITE_SIZE 1024

static int *run_child;

static void dio_sparse(int fd, int filesize, int writesize)
{
	void *bufptr = NULL;
	int i;
	int w;

	bufptr = SAFE_MEMALIGN(getpagesize(), writesize);

	memset(bufptr, 0, writesize);
	SAFE_LSEEK(fd, 0, SEEK_SET);

	for (i = 0; i < filesize;) {
		w = SAFE_WRITE(0, fd, bufptr, writesize);
		i += w;
	}
}

static void read_sparse(const char *filename, int filesize)
{
	char buff[4096];
	int fd;
	int i;
	int r;

	while ((fd = open(filename, O_RDONLY, 0666)) < 0)
		usleep(100);

	tst_res(TINFO, "child %i reading file", getpid());

	SAFE_LSEEK(fd, SEEK_SET, 0);
	while (*run_child) {
		off_t offset = 0;
		char *bufoff;

		for (i = 0; i < filesize + 1; i += sizeof(buff)) {
			r = SAFE_READ(0, fd, buff, sizeof(buff));
			if (r > 0) {
				bufoff = check_zero(buff, r);
				if (bufoff) {
					tst_res(TINFO, "non-zero read at offset %zu",
						offset + (bufoff - buff));
					break;
				}
				offset += r;
			}
		}
	}

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
	char *filename = "sparse_file";
	int filesize = FILE_SIZE;
	int num_children = NUM_CHILDREN;
	int writesize = WRITE_SIZE;
	int status;
	int fd;
	int i;

	tst_res(TINFO, "Dirtying free blocks");
	dirty_freeblocks(filesize);

	fd = SAFE_OPEN(filename, O_DIRECT | O_WRONLY | O_CREAT, 0666);
	SAFE_FTRUNCATE(fd, filesize);

	*run_child = 1;

	for (i = 0; i < num_children; i++) {
		if (!SAFE_FORK()) {
			read_sparse(filename, filesize);
			return;
		}
	}

	dio_sparse(fd, filesize, writesize);

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
