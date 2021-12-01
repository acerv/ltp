// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2004 Daniel McNeil <daniel@osdl.org>
 *               2004 Open Source Development Lab
 *               2004  Marty Ridgeway <mridge@us.ibm.com>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Append zeroed data to a file using libaio while other processes are doing
 * buffered reads and check if the buffer reads always see zero.
 */

#define _GNU_SOURCE

#include "tst_test.h"

#ifdef HAVE_LIBAIO
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libaio.h>
#include "common.h"

#define NUM_CHILDREN 8
#define NUM_AIO 16
#define AIO_SIZE (64 * 1024)
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

/*
 * append to the end of a file using AIO DIRECT.
 */
static void aiodio_append(char *filename, int bcount)
{
	int fd;
	void *bufptr;
	int i;
	int w;
	struct iocb iocb_array[NUM_AIO];
	struct iocb *iocbs[NUM_AIO];
	off_t offset = 0;
	io_context_t myctx;
	struct io_event event;
	struct timespec timeout;

	fd = SAFE_OPEN(filename, O_DIRECT | O_WRONLY | O_CREAT, 0666);

	memset(&myctx, 0, sizeof(myctx));
	io_queue_init(NUM_AIO, &myctx);

	for (i = 0; i < NUM_AIO; i++) {
		bufptr = SAFE_MEMALIGN(getpagesize(), AIO_SIZE);
		memset(bufptr, 0, AIO_SIZE);
		io_prep_pwrite(&iocb_array[i], fd, bufptr, AIO_SIZE, offset);
		iocbs[i] = &iocb_array[i];
		offset += AIO_SIZE;
	}

	/*
	 * Start the 1st NUM_AIO requests
	 */
	w = io_submit(myctx, NUM_AIO, iocbs);
	if (w < 0)
		tst_brk(TBROK, "io_submit: %s", tst_strerrno(-w));

	/*
	 * As AIO requests finish, keep issuing more AIOs.
	 */
	for (; i < bcount; i++) {
		int n = 0;
		struct iocb *iocbp;

		n = io_getevents(myctx, 1, 1, &event, &timeout);
		if (n > 0) {
			iocbp = (struct iocb *)event.obj;

			if (n > 0) {
				io_prep_pwrite(iocbp, fd, iocbp->u.c.buf, AIO_SIZE, offset);
				offset += AIO_SIZE;
				w = io_submit(myctx, 1, &iocbp);
				if (w < 0)
					tst_brk(TBROK, "io_submit: %s", tst_strerrno(-w));
			}
		}
	}
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

	tst_res(TINFO, "Parent append to file");

	aiodio_append(filename, 1000);

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
#else
static void run(void)
{
	tst_res(TCONF, "test requires libaio and it's development packages");
}

static struct tst_test test = {
	.test_all = run,
};
#endif
