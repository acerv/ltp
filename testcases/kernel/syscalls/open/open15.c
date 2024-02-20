// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 *    Author: David Fenner
 *    Copilot: Jon Hendrickson
 * Copyright (C) 2024 Andrea Cervesato andrea.cervesato@suse.com
 */

/*\
 * [Description]
 *
 * This test verifies that open() is working correctly on symlink()
 * generated files.
 */

#include "tst_test.h"

#define FILENAME "myfile.txt"
#define SYMBNAME "myfile_symlink"
#define BIG_STRING "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"

static char buff_file[128];
static char buff_symlink[128];
static int str_size;

static void run(void)
{
	int fd_file, fd_symlink;

	memset(buff_file, 0, sizeof(buff_file));
	memset(buff_symlink, 0, sizeof(buff_symlink));

	fd_file = SAFE_OPEN(FILENAME, O_CREAT | O_RDWR, 0777);
	SAFE_WRITE(SAFE_WRITE_ALL, fd_file, BIG_STRING, str_size);

	SAFE_SYMLINK(FILENAME, SYMBNAME);

	SAFE_LSEEK(fd_file, 0, 0);
	SAFE_READ(1, fd_file, buff_file, str_size);

	fd_symlink = SAFE_OPEN(SYMBNAME, O_RDWR, 0777);
	SAFE_LSEEK(fd_symlink, 0, 0);
	SAFE_READ(1, fd_symlink, buff_symlink, str_size);

	TST_EXP_EXPR(!strncmp(buff_file, buff_symlink, str_size),
		"file data is the equivalent to symlink generated file data");

	SAFE_CLOSE(fd_file);
	SAFE_CLOSE(fd_symlink);

	SAFE_UNLINK(SYMBNAME);
	SAFE_UNLINK(FILENAME);
}

static void setup(void)
{
	str_size = strlen(BIG_STRING);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_tmpdir = 1,
};
