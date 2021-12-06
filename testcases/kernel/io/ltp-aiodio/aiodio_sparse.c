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

static int *run_child;

static char *str_numchildren;
static char *str_writesize;
static char *str_filesize;
static char *str_numaio;

static int numchildren;
static long long writesize;
static long long filesize;
static long long alignment;
static int numaio;

static void aiodio_sparse(char *filename, long long align, long long ws, long long fs, int naio)
{
	int fd;
	int i, w;
	struct iocb **iocbs;
	off_t offset;
	io_context_t myctx;
	struct io_event event;
	int aio_inflight;

	if ((naio * ws) > fs)
		naio = fs / ws;

	fd = SAFE_OPEN(filename, O_DIRECT | O_WRONLY | O_CREAT, 0666);
	SAFE_FTRUNCATE(fd, fs);

	memset(&myctx, 0, sizeof(myctx));
	io_queue_init(naio, &myctx);

	iocbs = SAFE_MALLOC(sizeof(struct iocb *) * naio);
	for (i = 0; i < naio; i++)
		iocbs[i] = SAFE_MALLOC(sizeof(struct iocb));

	/*
	 * allocate the iocbs array and iocbs with buffers
	 */
	offset = 0;
	for (i = 0; i < naio; i++) {
		void *bufptr;

		bufptr = SAFE_MEMALIGN(align, ws);
		memset(bufptr, 0, ws);
		io_prep_pwrite(iocbs[i], fd, bufptr, ws, offset);
		offset += ws;
	}

	/*
	 * start the 1st naio write requests
	 */
	w = io_submit(myctx, naio, iocbs);
	if (w < 0)
		tst_brk(TBROK, "io_submit: %s", tst_strerrno(-w));

	/*
	 * As AIO requests finish, keep issuing more AIO until done.
	 */
	aio_inflight = naio;

	while (offset < fs) {
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
		io_prep_pwrite(iocbp, fd, iocbp->u.c.buf, ws, offset);
		offset += ws;
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
	struct stat sb;

	numchildren = 1000;
	writesize = 1024;
	filesize = 100 * 1024 * 1024;
	numaio = 16;
	alignment = 512;

	if (tst_parse_int(str_numchildren, &numchildren, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of children '%s'", str_numchildren);

	if (tst_parse_filesize(str_writesize, &writesize, 1, LONG_LONG_MAX))
		tst_brk(TBROK, "Invalid write blocks size '%s'", str_writesize);

	if (tst_parse_filesize(str_filesize, &filesize, 1, LONG_LONG_MAX))
		tst_brk(TBROK, "Invalid file size '%s'", str_filesize);

	if (tst_parse_int(str_numaio, &numaio, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of AIO control blocks '%s'", str_numaio);

	SAFE_STAT(".", &sb);
	alignment = sb.st_blksize;

	run_child = SAFE_MMAP(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	*run_child = 0;
	SAFE_MUNMAP(run_child, sizeof(int));
}

static void run(void)
{
	char *filename = "aiodio_sparse";
	int status;
	int i;

	*run_child = 1;

	for (i = 0; i < numchildren; i++) {
		if (!SAFE_FORK()) {
			io_read(filename, filesize, run_child);
			return;
		}
	}

	tst_res(TINFO, "Parent create a sparse file");

	aiodio_sparse(filename, alignment, writesize, filesize, numaio);

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
		{"w:", &str_writesize, "Size of writing blocks (default 1K)"},
		{"s:", &str_filesize, "Size of file (default 100M)"},
		{"o:", &str_numaio, "Number of AIO control blocks (default 16)"},
		{}
	},
};
#else
TST_TEST_TCONF("test requires libaio and its development packages");
#endif
