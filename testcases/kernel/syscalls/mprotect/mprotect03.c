// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`mprotect(2)` correctly removes write permission
 * from a shared mapped region. A child process attempts to write to
 * the read-only protected region and is expected to receive SIGSEGV.
 */

#include <sys/mman.h>
#include <sys/wait.h>

#include "tst_test.h"

static void run(void)
{
	char *addr;
	int fd, status;
	char *buf = "abcdefghijklmnopqrstuvwxyz";

	fd = SAFE_OPEN("testfile", O_RDWR | O_CREAT, 0777);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, buf, strlen(buf));

	addr = SAFE_MMAP(NULL, strlen(buf), PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);

	TST_EXP_PASS(mprotect(addr, strlen(buf), PROT_READ));
	if (!TST_PASS)
		goto out;

	pid_t pid = SAFE_FORK();

	if (!pid) {
		memcpy(addr, buf, strlen(buf));
		tst_res(TINFO, "memcpy() did not generate SIGSEGV");
		exit(1);
	}

	SAFE_WAITPID(pid, &status, 0);

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV)
		tst_res(TPASS, "SIGSEGV generated as expected");
	else
		tst_res(TFAIL, "child: %s", tst_strstatus(status));

out:
	SAFE_MUNMAP(addr, strlen(buf));
	SAFE_CLOSE(fd);
	SAFE_UNLINK("testfile");
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
	.needs_tmpdir = 1,
};
