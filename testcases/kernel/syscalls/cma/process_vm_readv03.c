// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2012
 * Copyright (c) Linux Test Project, 2012
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/* \
 * [Description]
 *
 * Fork two children, one child mallocs randomly sized trunks of memory
 * and initializes them; the other child calls process_vm_readv with
 * the remote iovecs initialized to the original process memory
 * locations and the local iovecs initialized to randomly sized and
 * allocated local memory locations. The second child then verifies
 * that the data copied is correct.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include "tst_test.h"
#include "lapi/syscalls.h"

static uintptr_t *data_ptr;

static char *str_buffsize;
static char *str_nr_iovecs;

static int bufsize = 100000;
static int nriovecs = 10;

static void create_data_size(int *arr, int arr_sz, int buffsize)
{
	long bufsz_left, bufsz_single;
	int i;

	bufsz_left = buffsize;
	for (i = 0; i < arr_sz - 1; i++) {
		bufsz_single = rand() % ((bufsz_left / 2) + 1);
		arr[i] = bufsz_single;
		bufsz_left -= bufsz_single;
	}

	arr[arr_sz - 1] = bufsz_left;
}

static void child_alloc(const int *sizes, int nr_iovecs)
{
	char **foo;
	int i, j;
	long count;

	foo = SAFE_MALLOC(nr_iovecs * sizeof(char *));

	count = 0;
	for (i = 0; i < nr_iovecs; i++) {
		foo[i] = SAFE_MALLOC(sizes[i]);
		for (j = 0; j < sizes[i]; j++) {
			foo[i][j] = count % 256;
			count++;
		}
	}

	*data_ptr = (uintptr_t)foo;

	tst_res(TINFO, "child 0: memory allocated and initialized");

	/* wake and wait until child_invoke is done reading from our VM */
	TST_CHECKPOINT_WAKE_AND_WAIT(0);
}

static long *fetch_remote_addrs(int nr_iovecs, pid_t pid_alloc)
{
	long *bar;
	long len;
	struct iovec local, remote;

	len = nr_iovecs * sizeof(long);
	bar = SAFE_MALLOC(len);
	local.iov_base = bar;
	local.iov_len = len;
	remote.iov_base = (void *)*data_ptr;
	remote.iov_len = len;

	TEST(tst_syscall(__NR_process_vm_readv, pid_alloc, &local, 1UL, &remote,
					 1UL, 0UL));
	if (TST_RET != len)
		tst_brk(TBROK, "process_vm_readv");

	return local.iov_base;
}

static void child_invoke(const int *sizes, int nr_iovecs, pid_t pid_alloc,
						 int buffsize)
{
	const int num_local_vecs = 4;
	struct iovec local[num_local_vecs];
	struct iovec remote[nr_iovecs];
	int i, j;
	int count;
	int nr_error;
	int *local_sizes;
	unsigned char expect, actual;
	long *addrs;

	addrs = fetch_remote_addrs(nr_iovecs, pid_alloc);

	for (i = 0; i < nr_iovecs; i++) {
		remote[i].iov_base = (void *)addrs[i];
		remote[i].iov_len = sizes[i];
	}

	/* use different buffer sizes for local memory */
	local_sizes = SAFE_MALLOC(num_local_vecs * sizeof(int));
	create_data_size(local_sizes, num_local_vecs, buffsize);
	for (i = 0; i < num_local_vecs; i++) {
		local[i].iov_base = SAFE_MALLOC(local_sizes[i]);
		local[i].iov_len = local_sizes[i];
	}

	tst_res(TINFO, "child 1: reading string from same memory location");

	TEST(tst_syscall(__NR_process_vm_readv, pid_alloc, local, num_local_vecs,
					 remote, nr_iovecs, 0UL));

	if (TST_RET < 0)
		tst_brk(TBROK, "process_vm_readv: %s", tst_strerrno(-TST_RET));

	if (TST_RET != buffsize)
		tst_brk(TBROK, "process_vm_readv: expected %d bytes but got %ld",
				buffsize, TST_RET);

	/* verify every byte */
	count = 0;
	nr_error = 0;
	for (i = 0; i < num_local_vecs; i++) {
		for (j = 0; j < (int)local[i].iov_len; j++) {
			expect = count % 256;
			actual = ((unsigned char *)local[i].iov_base)[j];
			if (expect != actual)
				nr_error++;

			count++;
		}
	}

	if (nr_error)
		tst_brk(TFAIL, "child 1: %d incorrect bytes received", nr_error);
	else
		tst_res(TPASS, "child 1: all bytes are correctly received");
}

static void setup(void)
{
	int iov_max;

	/* Just a sanity check of the existence of syscall */
	tst_syscall(__NR_process_vm_readv, getpid(), NULL, 0UL, NULL, 0UL, 0UL);

	if (tst_parse_int(str_buffsize, &bufsize, 4, INT_MAX))
		tst_brk(TBROK, "Invalid buffer size '%s'", str_buffsize);

	iov_max = SAFE_SYSCONF(_SC_IOV_MAX);
	if (tst_parse_int(str_nr_iovecs, &nriovecs, 1, iov_max))
		tst_brk(TBROK, "Invalid IO vectors '%s'", str_nr_iovecs);

	data_ptr = SAFE_MMAP(NULL, sizeof(uintptr_t), PROT_READ | PROT_WRITE,
						 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	if (data_ptr)
		SAFE_MUNMAP(data_ptr, sizeof(uintptr_t));
}

static void run(void)
{
	pid_t pid_alloc;
	pid_t pid_invoke;
	int status;
	int *sizes;

	/* create random iovec data size */
	sizes = SAFE_MALLOC(nriovecs * sizeof(int));
	create_data_size(sizes, nriovecs, bufsize);

	pid_alloc = SAFE_FORK();
	if (!pid_alloc) {
		child_alloc(sizes, nriovecs);
		return;
	}

	TST_CHECKPOINT_WAIT(0);

	pid_invoke = SAFE_FORK();
	if (!pid_invoke) {
		child_invoke(sizes, nriovecs, pid_alloc, bufsize);
		return;
	}

	/* wait until child_invoke reads from child_alloc's VM */
	SAFE_WAITPID(pid_invoke, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		tst_res(TFAIL, "child 1: returns %d", status);

	/* child_alloc is free to exit now */
	TST_CHECKPOINT_WAKE(0);

	SAFE_WAITPID(pid_alloc, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		tst_res(TFAIL, "child 0: returns %d", status);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.forks_child = 1,
	.needs_checkpoints = 1,
	.options = (struct tst_option[]) {
		{"s:", &str_buffsize, "Total buffer size (default 100000)"},
		{"n:", &str_nr_iovecs, "Number of iovecs to be allocated (default 10)"},
		{},
	},
};
