// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *	07/2001 Ported by Wayne Boyer
 * Copyright (c) 2025 SUSE LLC Ricardo B. Marlière <rbm@suse.com>
 */

/*\
 * Verify that after writing an invalid base address into a segment entry,
 * a subsequent segment entry read will raise SIGSEV.
 */

#include "tst_test.h"

#ifdef __i386__
#include "common.h"

int read_segment(unsigned int index)
{
	int res;

	__asm__ __volatile__("\n\
			push    $0x0007;\n\
			pop     %%fs;\n\
			movl    %%fs:(%1), %0"
			     : "=r"(res)
			     : "r"(index * sizeof(int)));

	return res;
}

void run(void)
{
	int pid, status, seg[4];

	seg[0] = 12345;

	create_segment(seg, sizeof(seg));

	TST_EXP_EQ_LI(read_segment(0), seg[0]);

	create_segment(0, 10);

	pid = SAFE_FORK();
	if (!pid) {
		signal(SIGSEGV, SIG_DFL);
		read_segment(0);
		exit(1);
	}

	SAFE_WAITPID(pid, &status, 0);
	if (WEXITSTATUS(status) != 0) {
		tst_res(TFAIL, "Did not generate SEGV, child returned "
			       "unexpected status");
	} else {
		if (WIFSIGNALED(status) && (WTERMSIG(status) == SIGSEGV))
			tst_res(TPASS, "generate SEGV as expected");
		else
			tst_res(TFAIL, "Did not generate SEGV");
	}
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
};

#else
TST_TEST_TCONF("Test supported only on i386");
#endif /* __i386__ */
