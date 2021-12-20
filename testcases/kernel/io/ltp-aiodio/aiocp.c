// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Copy file by using a async I/O state machine.
 * 1. Start read request
 * 2. When read completes turn it into a write request
 * 3. When write completes decrement counter and free resources
 */

#define _GNU_SOURCE

#include "tst_test.h"

#ifdef HAVE_LIBAIO
#include <libaio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "common.h"

static char *str_aio_blksize;
static char *str_filesize;
static char *str_aionum;
static char *str_oflag;

static long long aio_blksize;
static long long filesize;
static long long alignment;
static int aionum;
static int srcflags;
static int dstflags;

static int srcfd;
static int dstfd;
static long long busy;
static long long tocopy;
static struct iocb **iocb_free;

static void fill_with_rand_data(int fd, long long size)
{
	int lower = 97;
	int upper = 100;
	int bufsize = 256 * 1024;
	char buf[bufsize];
	long long i = 0;
	int j;

	srand(time(NULL));

	while (i < size) {
		for (j = 0; j < bufsize; j++) {
			buf[j] = (rand() % (upper - lower + 1)) + lower;
			i++;

			if (i > size)
				break;
		}

		SAFE_WRITE(0, fd, buf, j);
	}

	SAFE_FSYNC(fd);
}

static void async_init(void)
{
	int i;
	char *buff;

	iocb_free = SAFE_MALLOC(aionum * sizeof(struct iocb *));
	for (i = 0; i < aionum; i++) {
		iocb_free[i] = SAFE_MALLOC(sizeof(struct iocb));
		buff = SAFE_MEMALIGN(alignment, aio_blksize);

		io_prep_pread(iocb_free[i], -1, buff, aio_blksize, 0);
	}
}

static void async_write_done(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
	int iosize = iocb->u.c.nbytes;

	if (res != iosize)
		tst_brk(TBROK, "Write missing bytes expect %d got %ld", iosize, res);

	if (res2 != 0)
		tst_brk(TBROK, "Write error: %s", tst_strerrno(-res2));

	--busy;
	--tocopy;

	if (dstflags & O_DIRECT)
		SAFE_FSYNC(dstfd);

	//tst_res(TINFO, "write done. offset = %lld", iocb->u.c.offset);
}

static void async_copy(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
	int iosize = iocb->u.c.nbytes;
	char *buf = iocb->u.c.buf;
	off_t offset = iocb->u.c.offset;
	int w;

	if (res != iosize)
		tst_brk(TBROK, "Read missing bytes expect %d got %ld", iosize, res);

	if (res2 != 0)
		tst_brk(TBROK, "Read error: %s", tst_strerrno(-res2));

	io_prep_pwrite(iocb, dstfd, buf, iosize, offset);
	io_set_callback(iocb, async_write_done);

	w = io_submit(ctx, 1, &iocb);
	if (w < 0)
		tst_brk(TBROK, "io_submit error: %s", tst_strerrno(-w));
}

static void async_run(io_context_t ctx, int fd, io_callback_t cb)
{
	long long offset = 0;
	int w, i, n;
	int iosize;

	tocopy = howmany(filesize, aio_blksize);
	busy = 0;

	while (tocopy > 0) {
		/* Submit as many reads as once as possible up to aionum */
		n = MIN(aionum - busy, aionum);
		if (n > 0) {
			for (i = 0; i < n; i++) {
				struct iocb *io = iocb_free[i];

				iosize = MIN(filesize - offset, aio_blksize);

				/* If we don't have any byte to write, exit */
				if (iosize <= 0)
					break;

				io_prep_pread(io, fd, io->u.c.buf, iosize, offset);
				io_set_callback(io, cb);

				offset += iosize;
			}

			w = io_submit(ctx, i, iocb_free);
			if (w < 0)
				tst_brk(TBROK, "io_submit write error: %s", tst_strerrno(-w));

			busy += n;
		}

		io_queue_run(ctx);
	}
}

static void setup(void)
{
	struct stat sb;
	int maxaio;

	aio_blksize = 64 * 1024;
	filesize = 1 * 1024 * 1024;
	aionum = 16;
	alignment = 512;
	srcflags = O_RDONLY;
	dstflags = O_WRONLY;

	if (tst_parse_int(str_aionum, &aionum, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of I/O '%s'", str_aionum);

	SAFE_FILE_SCANF("/proc/sys/fs/aio-max-nr", "%d", &maxaio);
	tst_res(TINFO, "Maximum AIO blocks: %d", maxaio);

	if (aionum > maxaio)
		tst_res(TCONF, "Number of async IO blocks passed the maximum (%d)", maxaio);

	if (tst_parse_filesize(str_aio_blksize, &aio_blksize, 1, LLONG_MAX))
		tst_brk(TBROK, "Invalid write blocks size '%s'", str_aio_blksize);

	SAFE_STAT(".", &sb);
	alignment = sb.st_blksize;

	if (tst_parse_filesize(str_filesize, &filesize, 1, LLONG_MAX))
		tst_brk(TBROK, "Invalid file size '%s'", str_filesize);

	if (str_oflag) {
		if (strncmp(str_oflag, "SYNC", 4) == 0) {
			dstflags |= O_SYNC;
		} else if (strncmp(str_oflag, "DIRECT", 6) == 0) {
			srcflags |= O_DIRECT;
			dstflags |= O_DIRECT;
		}
	}
}

static void run(void)
{
	char *srcname = "srcfile.bin";
	char *dstname = "dstfile.bin";
	io_context_t myctx;
	struct stat st;
	int buffsize = 4096;
	char srcbuff[buffsize];
	char dstbuff[buffsize];
	int fail = 0;
	int reads = 0;
	int i, r;

	while ((srcfd = open(srcname, srcflags | O_RDWR | O_CREAT, 0666)) < 0)
		usleep(100);

	tst_res(TINFO, "Fill %s with random data", srcname);
	fill_with_rand_data(srcfd, filesize);

	while ((dstfd = open(dstname, dstflags | O_WRONLY | O_CREAT, 0666)) < 0)
		usleep(100);

	tst_res(TINFO, "Copy %s -> %s", srcname, dstname);

	memset(&myctx, 0, sizeof(myctx));
	io_queue_init(aionum, &myctx);

	async_init();
	async_run(myctx, srcfd, async_copy);

	io_destroy(myctx);
	SAFE_CLOSE(srcfd);
	SAFE_CLOSE(dstfd);

	/* check if file has been copied correctly */
	tst_res(TINFO, "Comparing %s with %s", srcname, dstname);

	SAFE_STAT(dstname, &st);
	if (st.st_size != filesize) {
		tst_res(TFAIL, "Expected destination file size %lld but it's %ld", filesize, st.st_size);
		/* no need to compare files */
		return;
	}

	while ((srcfd = open(srcname, srcflags | O_RDONLY, 0666)) < 0)
		usleep(100);

	while ((dstfd = open(dstname, srcflags | O_RDONLY, 0666)) < 0)
		usleep(100);

	reads = howmany(filesize, buffsize);

	for (i = 0; i < reads; i++) {
		r = SAFE_READ(0, srcfd, srcbuff, buffsize);
		SAFE_READ(0, dstfd, dstbuff, buffsize);
		if (memcmp(srcbuff, dstbuff, r)) {
			fail = 1;
			break;
		}
	}

	SAFE_CLOSE(srcfd);
	SAFE_CLOSE(dstfd);

	if (fail)
		tst_res(TFAIL, "Files are not identical");
	else
		tst_res(TPASS, "Files are identical");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_tmpdir = 1,
	.options = (struct tst_option[]) {
		{"b:", &str_aio_blksize, "-b\t Size of writing blocks (default 1K)"},
		{"s:", &str_filesize, "-s\t Size of file (default 10M)"},
		{"n:", &str_aionum, "-n\t Number of Async IO blocks (default 16)"},
		{"f:", &str_oflag, "-f\t Open flag: SYNC | DIRECT (default O_CREAT only)"},
		{},
	},
};
#else
TST_TEST_TCONF("test requires libaio and its development packages");
#endif
