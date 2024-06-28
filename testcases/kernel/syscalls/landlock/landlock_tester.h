/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef LANDLOCK_TESTER_H

#include "tst_test.h"
#include "lapi/landlock.h"
#include <sys/sysmacros.h>

#define PERM_MODE 0700

#define SANDBOX_FOLDER	"sandbox"
#define TESTAPP			"landlock_exec"

#define FILE_EXEC		SANDBOX_FOLDER"/"TESTAPP
#define FILE_READ		SANDBOX_FOLDER"/file_read"
#define FILE_WRITE		SANDBOX_FOLDER"/file_write"
#define FILE_REMOVE		SANDBOX_FOLDER"/file_remove"
#define FILE_UNLINK		SANDBOX_FOLDER"/file_unlink"
#define FILE_UNLINKAT	SANDBOX_FOLDER"/file_unlinkat"
#define FILE_TRUNCATE	SANDBOX_FOLDER"/file_truncate"
#define FILE_REGULAR	SANDBOX_FOLDER"/regular0"
#define FILE_SOCKET		SANDBOX_FOLDER"/socket0"
#define FILE_FIFO		SANDBOX_FOLDER"/fifo0"
#define FILE_FIFO1		SANDBOX_FOLDER"/fifo1"
#define FILE_SYM0		SANDBOX_FOLDER"/symbolic0"
#define FILE_SYM1		SANDBOX_FOLDER"/symbolic1"
#define DIR_READDIR		SANDBOX_FOLDER"/dir_readdir"
#define DIR_RMDIR		SANDBOX_FOLDER"/dir_rmdir"
#define DEV_CHAR0		SANDBOX_FOLDER"/chardev0"
#define DEV_BLK0		SANDBOX_FOLDER"/blkdev0"

#define ALL_RULES (\
	LANDLOCK_ACCESS_FS_EXECUTE | \
	LANDLOCK_ACCESS_FS_WRITE_FILE | \
	LANDLOCK_ACCESS_FS_READ_FILE | \
	LANDLOCK_ACCESS_FS_READ_DIR | \
	LANDLOCK_ACCESS_FS_REMOVE_DIR | \
	LANDLOCK_ACCESS_FS_REMOVE_FILE | \
	LANDLOCK_ACCESS_FS_MAKE_CHAR | \
	LANDLOCK_ACCESS_FS_MAKE_DIR | \
	LANDLOCK_ACCESS_FS_MAKE_REG | \
	LANDLOCK_ACCESS_FS_MAKE_SOCK | \
	LANDLOCK_ACCESS_FS_MAKE_FIFO | \
	LANDLOCK_ACCESS_FS_MAKE_BLOCK | \
	LANDLOCK_ACCESS_FS_MAKE_SYM | \
	LANDLOCK_ACCESS_FS_REFER | \
	LANDLOCK_ACCESS_FS_TRUNCATE | \
	LANDLOCK_ACCESS_NET_BIND_TCP | \
	LANDLOCK_ACCESS_NET_CONNECT_TCP | \
	LANDLOCK_ACCESS_FS_IOCTL_DEV)

static char *readdir_files[] = {
	DIR_READDIR"/file0",
	DIR_READDIR"/file1",
	DIR_READDIR"/file2",
};

static int dev_chr;
static int dev_blk;

static int tester_get_all_rules(void)
{
	int abi;
	int all_rules = ALL_RULES;

	abi = SAFE_LANDLOCK_CREATE_RULESET(
		NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);

	if (abi < 2)
		all_rules &= ~LANDLOCK_ACCESS_FS_REFER;

	if (abi < 3)
		all_rules &= ~LANDLOCK_ACCESS_FS_TRUNCATE;

	if (abi < 4) {
		all_rules &= ~(LANDLOCK_ACCESS_NET_BIND_TCP |
			LANDLOCK_ACCESS_NET_CONNECT_TCP);
	}

	if (abi < 5)
		all_rules &= ~LANDLOCK_ACCESS_FS_IOCTL_DEV;

	return all_rules;
}

static void tester_create_tree(void)
{
	if (access(SANDBOX_FOLDER, F_OK) == -1)
		SAFE_MKDIR(SANDBOX_FOLDER, PERM_MODE);

	/* folders */
	SAFE_MKDIR(DIR_RMDIR, PERM_MODE);
	SAFE_MKDIR(DIR_READDIR, PERM_MODE);
	for (size_t i = 0; i < ARRAY_SIZE(readdir_files); i++)
		SAFE_TOUCH(readdir_files[i], PERM_MODE, NULL);

	/* files */
	tst_fill_file(FILE_READ, 'a', getpagesize(), 1);
	SAFE_TOUCH(FILE_WRITE, PERM_MODE, NULL);
	SAFE_TOUCH(FILE_REMOVE, PERM_MODE, NULL);
	SAFE_TOUCH(FILE_UNLINK, PERM_MODE, NULL);
	SAFE_TOUCH(FILE_UNLINKAT, PERM_MODE, NULL);
	SAFE_TOUCH(FILE_TRUNCATE, PERM_MODE, NULL);
	SAFE_TOUCH(FILE_SYM0, PERM_MODE, NULL);
	SAFE_MKFIFO(FILE_FIFO1, PERM_MODE);
	SAFE_CP(TESTAPP, FILE_EXEC);

	/* devices */
	dev_chr = makedev(1, 3);
	dev_blk = makedev(7, 0);
}

static void _test_exec(const int result)
{
	int status;
	pid_t pid;
	char *const args[] = {(char *)FILE_EXEC, NULL};

	tst_res(TINFO, "Test binary execution");

	pid = SAFE_FORK();
	if (!pid) {
		int rval;

		if (result == TPASS) {
			rval = execve(FILE_EXEC, args, NULL);
			if (rval == -1)
				tst_res(TFAIL | TERRNO, "Failed to execute test binary");
		} else {
			TST_EXP_FAIL(execve(FILE_EXEC, args, NULL), EACCES);
		}

		_exit(1);
	}

	SAFE_WAITPID(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		return;

	tst_res(result, "Test binary has been executed");
}

static void _test_write(const int result)
{
	tst_res(TINFO, "Test writing file");

	if (result == TPASS)
		TST_EXP_FD(open(FILE_WRITE, O_WRONLY, 0640));
	else
		TST_EXP_FAIL(open(FILE_WRITE, O_WRONLY, 0640), EACCES);

	if (TST_RET != -1)
		SAFE_CLOSE(TST_RET);
}

static void _test_read(const int result)
{
	tst_res(TINFO, "Test reading file");

	if (result == TPASS)
		TST_EXP_FD(open(FILE_READ, O_RDONLY, 0640));
	else
		TST_EXP_FAIL(open(FILE_READ, O_RDONLY, 0640), EACCES);

	if (TST_RET != -1)
		SAFE_CLOSE(TST_RET);
}

static void _test_readdir(const int result)
{
	tst_res(TINFO, "Test reading directory");

	DIR *dir;
	struct dirent *de;
	int files_counted = 0;

	dir = opendir(DIR_READDIR);
	if (!dir) {
		tst_res(result == TPASS ? TFAIL : TPASS,
			"Can't read '%s' directory", DIR_READDIR);

		return;
	}

	tst_res(result, "Can read '%s' directory", DIR_READDIR);
	if (result == TFAIL)
		return;

	while ((de = readdir(dir)) != NULL) {
		if (de->d_type != DT_REG)
			continue;

		for (size_t i = 0; i < ARRAY_SIZE(readdir_files); i++) {
			if (readdir_files[i] == NULL)
				continue;

			if (strstr(readdir_files[i], de->d_name) != NULL)
				files_counted++;
		}
	}

	SAFE_CLOSEDIR(dir);

	TST_EXP_EQ_LI(files_counted, ARRAY_SIZE(readdir_files));
}

static void _test_rmdir(const int result)
{
	tst_res(TINFO, "Test removing directory");

	if (result == TPASS)
		TST_EXP_PASS(rmdir(DIR_RMDIR));
	else
		TST_EXP_FAIL(rmdir(DIR_RMDIR), EACCES);
}

static void _test_rmfile(const int result)
{
	tst_res(TINFO, "Test removing file");

	if (result == TPASS) {
		TST_EXP_PASS(unlink(FILE_UNLINK));
		TST_EXP_PASS(remove(FILE_REMOVE));
	} else {
		TST_EXP_FAIL(unlink(FILE_UNLINK), EACCES);
		TST_EXP_FAIL(remove(FILE_REMOVE), EACCES);
	}
}

static void _test_make(
	const char *path,
	const int type,
	const int dev,
	const int result)
{
	tst_res(TINFO, "Test normal or special files creation");

	if (result == TPASS)
		TST_EXP_PASS(mknod(path, type | 0400, dev));
	else
		TST_EXP_FAIL(mknod(path, type | 0400, dev), EACCES);
}

static void _test_symbolic(const int result)
{
	tst_res(TINFO, "Test symbolic links");

	if (result == TPASS)
		TST_EXP_PASS(symlink(FILE_SYM0, FILE_SYM1));
	else
		TST_EXP_FAIL(symlink(FILE_SYM0, FILE_SYM1), EACCES);
}

static void _test_truncate(const int result)
{
	int fd;

	tst_res(TINFO, "Test truncating file");

	if (result == TPASS) {
		TST_EXP_PASS(truncate(FILE_TRUNCATE, 10));

		fd = TST_EXP_FD(open(FILE_TRUNCATE, O_WRONLY, 0640));
		if (fd != -1) {
			TST_EXP_PASS(ftruncate(fd, 10));
			SAFE_CLOSE(fd);
		}

		fd = TST_EXP_FD(open(FILE_TRUNCATE, O_WRONLY | O_TRUNC, 0640));
		if (fd != -1)
			SAFE_CLOSE(fd);
	} else {
		TST_EXP_FAIL(truncate(FILE_TRUNCATE, 10), EACCES);

		fd = open(FILE_TRUNCATE, O_WRONLY, 0640);
		if (fd != -1) {
			TST_EXP_FAIL(ftruncate(fd, 10), EACCES);
			SAFE_CLOSE(fd);
		}

		TST_EXP_FAIL(open(FILE_TRUNCATE, O_WRONLY | O_TRUNC, 0640),
			EACCES);

		if (TST_RET != -1)
			SAFE_CLOSE(TST_RET);
	}
}

static void _test_ioctl(const int result)
{
	int fd;
	size_t sz = 0;

	tst_res(TINFO, "Test I/O control");

	fd = open(FILE_FIFO1, O_RDONLY, PERM_MODE);
	if (result == TFAIL) {
		tst_res(TINFO, "Skip test: can't open fifo due to rules restriction.");
		return;
	}

	if (result == TPASS)
		TST_EXP_PASS(ioctl(fd, FIONREAD, &sz));
	else
		TST_EXP_FAIL(ioctl(fd, FIONREAD, &sz), EACCES);
}

static void tester_run_rules(const int rules, const int result)
{
	if (rules & LANDLOCK_ACCESS_FS_EXECUTE)
		_test_exec(result);

	if (rules & LANDLOCK_ACCESS_FS_WRITE_FILE)
		_test_write(result);

	if (rules & LANDLOCK_ACCESS_FS_READ_FILE)
		_test_read(result);

	if (rules & LANDLOCK_ACCESS_FS_READ_DIR)
		_test_readdir(result);

	if (rules & LANDLOCK_ACCESS_FS_REMOVE_DIR)
		_test_rmdir(result);

	if (rules & LANDLOCK_ACCESS_FS_REMOVE_FILE)
		_test_rmfile(result);

	if (rules & LANDLOCK_ACCESS_FS_MAKE_CHAR)
		_test_make(DEV_CHAR0, S_IFCHR, dev_chr, result);

	if (rules & LANDLOCK_ACCESS_FS_MAKE_BLOCK)
		_test_make(DEV_BLK0, S_IFBLK, dev_blk, result);

	if (rules & LANDLOCK_ACCESS_FS_MAKE_REG)
		_test_make(FILE_REGULAR, S_IFREG, 0, result);

	if (rules & LANDLOCK_ACCESS_FS_MAKE_SOCK)
		_test_make(FILE_SOCKET, S_IFSOCK, 0, result);

	if (rules & LANDLOCK_ACCESS_FS_MAKE_FIFO)
		_test_make(FILE_FIFO, S_IFIFO, 0, result);

	if (rules & LANDLOCK_ACCESS_FS_MAKE_SYM)
		_test_symbolic(result);

	if (rules & LANDLOCK_ACCESS_FS_TRUNCATE)
		_test_truncate(result);

	if (rules & LANDLOCK_ACCESS_FS_IOCTL_DEV)
		_test_ioctl(result);
}

static inline void tester_run_all_rules(const int pass_rules)
{
	int fail_rules;
	int all_rules;

	all_rules = tester_get_all_rules();
	fail_rules = all_rules & ~pass_rules;

	tester_run_rules(pass_rules, TPASS);
	tester_run_rules(fail_rules, TFAIL);
}

#endif
