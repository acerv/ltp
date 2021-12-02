// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2004 Daniel McNeil <daniel@osdl.org>
 *               2004 Open Source Development Lab
 *
 * Copyright (c) 2004 Marty Ridgeway <mridge@us.ibm.com>
 *
 * Copyright (c) 2011 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Create a sparse file using libaio while other processes are doing
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

#define NUM_CHILDREN 1000
#define NUM_AIO 16
#define WRITE_SIZE (64 * 1024)
#define FILE_SIZE (100 * 1024 * 1024)

static int *run_child;

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

static void aiodio_sparse(char *filename, int writesize, int filesize, int num_aio)
{
	int fd;
	int i, w;
	struct iocb **iocbs;
	off_t offset;
	io_context_t myctx;
	struct io_event event;
	int aio_inflight;

	if ((num_aio * writesize) > filesize)
		num_aio = filesize / writesize;

	fd = SAFE_OPEN(filename, O_DIRECT | O_WRONLY | O_CREAT, 0666);
	SAFE_FTRUNCATE(fd, filesize);

	memset(&myctx, 0, sizeof(myctx));
	io_queue_init(num_aio, &myctx);

	iocbs = SAFE_MALLOC(sizeof(struct iocb *) * num_aio);
	for (i = 0; i < num_aio; i++)
		iocbs[i] = SAFE_MALLOC(sizeof(struct iocb));

	/*
	 * allocate the iocbs array and iocbs with buffers
	 */
	offset = 0;
	for (i = 0; i < num_aio; i++) {
		void *bufptr;

		bufptr = SAFE_MEMALIGN(getpagesize(), writesize);
		memset(bufptr, 0, writesize);
		io_prep_pwrite(iocbs[i], fd, bufptr, writesize, offset);
		offset += writesize;
	}

	/*
	 * start the 1st num_aio write requests
	 */
	w = io_submit(myctx, num_aio, iocbs);
	if (w < 0)
		tst_brk(TBROK, "io_submit: %s", tst_strerrno(-w));

	/*
	 * As AIO requests finish, keep issuing more AIO until done.
	 */
	aio_inflight = num_aio;

	while (offset < filesize) {
		int n;
		struct iocb *iocbp;

		n = io_getevents(myctx, 1, 1, &event, 0);
		if (n != 1 && -n != EINTR)
			tst_brk(TBROK, "io_getevents: %s", tst_strerrno(-n));

		aio_inflight--;

		/*
		 * check if write succeeded.
		 */
		iocbp = (struct iocb *)event.obj;
		if (event.res2 != 0 || event.res != iocbp->u.c.nbytes)
			tst_brk(TBROK,
					"AIO write offset %lld expected %ld got %ld",
					iocbp->u.c.offset, iocbp->u.c.nbytes,
					event.res);

		/* start next write */
		io_prep_pwrite(iocbp, fd, iocbp->u.c.buf, writesize, offset);
		offset += writesize;
		w = io_submit(myctx, 1, &iocbp);
		if (w < 0)
			tst_brk(TBROK, "io_submit: %s", tst_strerrno(-w));

		aio_inflight++;
	}

	/*
	 * wait for AIO requests in flight.
	 */
	while (aio_inflight > 0) {
		int n;
		struct iocb *iocbp;

		n = io_getevents(myctx, 1, 1, &event, 0);
		if (n != 1)
			tst_brk(TBROK, "io_getevents failed");

		aio_inflight--;

		/*
		 * check if write succeeded.
		 */
		iocbp = (struct iocb *)event.obj;
		if (event.res2 != 0 || event.res != iocbp->u.c.nbytes)
			tst_brk(TBROK,
					"AIO write offset %lld expected %ld got %ld",
					iocbp->u.c.offset, iocbp->u.c.nbytes,
					event.res);
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
	int writesize = WRITE_SIZE;
	int num_children = NUM_CHILDREN;
	int status;
	int i;

	*run_child = 1;

	for (i = 0; i < num_children; i++) {
		if (!SAFE_FORK()) {
			read_sparse(filename, filesize);
			return;
		}
	}

	tst_res(TINFO, "Parent create a sparse file");

	aiodio_sparse(filename, writesize, filesize, NUM_AIO);

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
