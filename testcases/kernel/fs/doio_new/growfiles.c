// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * 		Author: Richard Logan
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 */

#include "tst_test.h"
#include "tlibio.h"
#include "datafill.h"
#include "open_flags.h"

enum {
	IO_TYPE_SYNC = 1,
	IO_TYPE_ASYNC,
	IO_TYPE_LISTIO_SYNC,
	IO_TYPE_LISTIO_ASYNC,
	IO_TYPE_POLLED_ASYNC,
	IO_TYPE_RANDOM,
};

enum {
	PATTERN_ASCII = 1,	/* repeating alphabet letter pattern */
						/* allows multiple writers and to be checked */
	PATTERN_PID,		/* <pid><words byte offset><pid> */
						/* Assumes 64 bit word. Only allows single */
						/* process to write and check */
	PATTERN_OFFSET, 	/* Like PATTERN_PID but has a fixed number */
						/* (STATIC_NUM) instead of pid. */
						/* Allows multiple processes to write/read */
	PATTERN_ALT,		/* alternating bit pattern (i.e. 0x5555555...) */
	PATTERN_CHKER,		/* checkerboard pattern (i.e. 0xff00ff00ff00...) */
	PATTERN_CNTING,		/* counting pattern (i.e. 0 - 07, 0 - 07, ...) */
	PATTERN_ONES,		/* all bits set (i.e. 0xffffffffffffff...) */
	PATTERN_ZEROS,		/* all bits cleared (i.e. 0x000000000...) */
	PATTERN_RANDOM,		/* random integers - can not be checked */
};

static char *sync_mode;
static char *str_bytes_to_consume;
static char *str_num_check_file;
static char *str_num_check_write;
static char *str_num_errors;
static char *str_grow_incr;
static char *str_io_type;
static char *lock_file;
static char *str_delay_iter;
static char *str_open_flags;
static char *str_pattern;
static char *str_num_rand_lseek;
static char *str_num_files;
static char *str_trunck_incr;
static char *str_num_growfiles;
static char *use_lseek;

static int bytes_to_consume;
static int num_check_file;
static int num_check_write;
static int num_errors;
static int grow_incr;
static int io_type;
static int delay_iter;
static int open_flags;
static int pattern;
static int num_rand_lseek;
static int num_files;
static int trunck_incr;
static int num_growfiles;
static int *fds;

static void shrink_file(void)
{
}

static void growfile(void)
{
}

static void check_write(void)
{
}

static void check_file(void)
{
}

static void setup(void)
{
	if (tst_parse_int(str_bytes_to_consume, &bytes_to_consume, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of bytes to consume '%s'", str_bytes_to_consume);

	if (tst_parse_int(str_num_check_file, &num_check_file, 0, INT_MAX))
		tst_brk(TBROK, "Invalid number files check '%s'", str_num_check_file);

	if (tst_parse_int(str_num_check_write, &num_check_write, 0, INT_MAX))
		tst_brk(TBROK, "Invalid number write check '%s'", str_num_check_write);

	if (tst_parse_int(str_num_errors, &num_errors, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of errors '%s'", str_num_errors);

	if (tst_parse_int(str_grow_incr, &grow_incr, 1, INT_MAX))
		tst_brk(TBROK, "Invalid grow increment '%s'", str_grow_incr);

	if (tst_parse_int(str_delay_iter, &delay_iter, 1, INT_MAX))
		tst_brk(TBROK, "Invalid delay before next iteration '%s'", str_delay_iter);

	if (tst_parse_int(str_num_rand_lseek, &num_rand_lseek, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of random lseek() '%s'", str_num_rand_lseek);

	if (tst_parse_int(str_num_files, &num_files, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of files '%s'", str_num_files);

	if (tst_parse_int(str_num_growfiles, &num_growfiles, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of growfiles before shrink '%s'", str_num_growfiles);

	if (tst_parse_int(str_trunck_incr, &trunck_incr, 1, INT_MAX))
		tst_brk(TBROK, "Invalid truncate increment '%s'", str_trunck_incr);

	if (str_io_type) {
		if (strlen(str_io_type) != 1)
			tst_brk(TBROK, "Invalid I/O type '%s'", str_io_type);

		switch (str_io_type[0]) {
		case 's':
			tst_res(TINFO, "Using synchronized I/O");
			io_type = IO_TYPE_SYNC;
			break;
		case 'p':
			tst_res(TINFO, "Using polled asynchronized I/O");
			io_type = IO_TYPE_POLLED_ASYNC;
			break;
		case 'a':
			tst_res(TINFO, "Using asynchronized I/O");
			io_type = IO_TYPE_ASYNC;
			break;
		case 'l':
			tst_res(TINFO, "Using synchronized list I/O");
			io_type = IO_TYPE_LISTIO_SYNC;
			break;
		case 'L':
			tst_res(TINFO, "Using asynchronized list I/O");
			io_type = IO_TYPE_LISTIO_ASYNC;
			break;
		case 'r':
			tst_res(TINFO, "Using randomized I/O");
			io_type = IO_TYPE_RANDOM;
			break;
		default:
			tst_brk(TBROK, "Invalid I/O type '%s'", str_io_type);
			break;
		}
	}

	if (str_open_flags)
		open_flags = parse_open_flags(str_open_flags, NULL);

	if (str_pattern) {
		if (strlen(str_pattern) != 1) {
			tst_brk(TBROK,
				"Invalid file pattern '%s'. "
				"Supported: A, a, p, o, c, C, r, z, or 0",
				str_pattern);
		}

		switch (str_pattern[0]) {
		case 'A':
			pattern = PATTERN_ALT;
			break;
		case 'a':
			pattern = PATTERN_ASCII;
			break;
		case 'p':
			pattern = PATTERN_PID;
			break;
		case 'o':
			pattern = PATTERN_OFFSET;
			break;
		case 'c':
			pattern = PATTERN_CHKER;
			break;
		case 'C':
			pattern = PATTERN_CNTING;
			break;
		case 'r':
			pattern = PATTERN_RANDOM;
			break;
		case 'z':
			pattern = PATTERN_ZEROS;
			break;
		case 'O':
			pattern = PATTERN_ONES;
			break;
		default:
			tst_brk(TBROK,
				"Invalid file pattern '%s'. "
				"Supported: A, a, p, o, c, C, r, z, or 0",
				str_pattern);
			break;
		}
	}
}

static void cleanup(void)
{
	if (fds) {
		for (int i = 0; i < num_files; i++) {
			if (fds[i])
				SAFE_CLOSE(fds[i]);
		}
	}
}

static void run(void)
{
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.options = (struct tst_option[]) {
		{"b", &sync_mode, "If defined, sync mode will be used instead of async"},
		{"B:", &str_bytes_to_consume, "Bytes to consume by all files"},
		{"c:", &str_num_check_file, "Times to check the file (default: 0)"},
		{"e:", &str_num_errors, "Errors before stopping (default: 1)"},
		{"g:", &str_grow_incr, "Grow files increment (default: 4096)"},
		{"l", &lock_file, "File locking before read/write/trunc (default: 0)"},
		{"L:", &str_delay_iter, "Delay in seconds after each iteration (default: 0)"},
		{"o:", &str_open_flags, "Arguments used to open() file"},
		{"q:", &str_pattern, "Pattern when writing (default: random)"},
		{"R:", &str_num_rand_lseek, "Number of random lseek() before write/trunc"},
		{"S:", &str_num_files, "Number of files to generate for iteration (default: 1)"},
		{"O:", &str_io_type, "Specify I/O type. s: sync, p: polled async, a: async, l: listio sync, L: listio async, r: random"},
		{"t:", &str_trunck_incr, "Increment during shrink"},
		{"T:", &str_num_growfiles, "Number of files grows before shrink"},
		{"w:", &use_lseek, "Grow using lseek() instead of write()"},
		{"W:", &str_num_check_write, "Times to check last write (default: 1)"},
	},
};
