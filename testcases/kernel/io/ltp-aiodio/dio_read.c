// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *   Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Create a file using buffered writes while other processes are doing
 * O_DIRECT reads and check if the buffer reads always see zero.
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "tst_test.h"
#include "common.h"

static char *str_numchildren;
static char *str_writesize;
static char *str_readsize;
static char *str_filesize;

static char *filename = "file.bin";
static int numchildren = 100;
static long long writesize = 32 * 1024 * 1024;
static long long readsize = 32 * 1024 * 1024;
static long long filesize = 128 * 1024 * 1024;
static int *children_completed;
static char *iobuf;
static int filedesc;

static void do_buffered_writes(int fd, char *bufptr, long long fsize, long long wsize, int pattern)
{
	long long offset;
	long long w;

	memset(bufptr, pattern, wsize);

	tst_res(TINFO, "child %i writing file", getpid());

	for (offset = 0; offset + wsize <= fsize; offset += wsize) {
		w = pwrite(fd, bufptr, wsize, offset);
		if (w < 0)
			tst_brk(TBROK, "pwrite: %s", tst_strerrno(-w));
		if (w != wsize)
			tst_brk(TBROK, "pwrite: wrote %lld bytes out of %lld", w, wsize);

		SAFE_FSYNC(fd);
	}
}

static int do_direct_reads(char *filename, char *bufptr, long long fsize, long long rsize)
{
	int fd;
	long long offset;
	long long w;
	int fail = 0;
	int keep_reading = 1;

	while ((fd = open(filename, O_RDONLY | O_DIRECT, 0666)) < 0)
		usleep(100);

	while (keep_reading) {
		for (offset = 0; offset + rsize < fsize; offset += rsize) {
			char *bufoff;

			keep_reading = *children_completed < numchildren;
			if (!keep_reading)
				break;

			w = pread(fd, bufptr, rsize, offset);
			if (w < 0)
				tst_brk(TBROK, "pread: %s", tst_strerrno(-w));
			if (w != rsize)
				tst_brk(TBROK, "pread: read %lld bytes out of %lld", w, rsize);

			bufoff = check_zero(bufptr, rsize);
			if (bufoff) {
				fail = 1;
				break;
			}
		}
	}

	SAFE_CLOSE(fd);

	return fail;
}

static void setup(void)
{
	struct stat sb;
	long long buffsize;
	long long alignment;

	if (tst_parse_int(str_numchildren, &numchildren, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of children '%s'", str_numchildren);

	if (tst_parse_filesize(str_filesize, &filesize, 1, LLONG_MAX))
		tst_brk(TBROK, "Invalid file size '%s'", str_filesize);

	if (tst_parse_filesize(str_writesize, &writesize, 1, filesize))
		tst_brk(TBROK, "Invalid write blocks size '%s'", str_writesize);

	if (tst_parse_filesize(str_readsize, &readsize, 1, filesize))
		tst_brk(TBROK, "Invalid read blocks size '%s'", str_readsize);

	SAFE_STAT(".", &sb);
	alignment = sb.st_blksize;

	buffsize = readsize;
	if (writesize > readsize)
		buffsize = writesize;

	iobuf = SAFE_MEMALIGN(alignment, buffsize);

	children_completed = SAFE_MMAP(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	filedesc = SAFE_OPEN(filename, O_CREAT | O_TRUNC | O_RDWR, 0666);
}

static void cleanup(void)
{
	SAFE_CLOSE(filedesc);
}

static void run(void)
{
	int i;
	int fail;

	// Fill the file with a known pattern so that the blocks
	// on disk can be detected if they become exposed
	do_buffered_writes(filedesc, iobuf, filesize, writesize, 1);
	SAFE_FSYNC(filedesc);
	SAFE_FTRUNCATE(filedesc, 0);
	SAFE_FSYNC(filedesc);

	SAFE_FTRUNCATE(filedesc, filesize);

	*children_completed = 0;

	for (i = 0; i < numchildren; i++) {
		if (!SAFE_FORK()) {
			do_buffered_writes(filedesc, iobuf, filesize, writesize, 0);
			(*children_completed)++;
			return;
		}
	}

	fail = do_direct_reads(filename, iobuf, filesize, readsize);

	if (fail)
		tst_res(TFAIL, "Non zero bytes read");
	else
		tst_res(TPASS, "All bytes read were zeroed");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{"n:", &str_numchildren, "Number of threads (default 100)"},
		{"w:", &str_writesize, "Size of writing blocks (default 32M)"},
		{"r:", &str_readsize, "Size of reading blocks (default 32M)"},
		{"s:", &str_filesize, "File size (default 128M)"},
		{}
	},
};
