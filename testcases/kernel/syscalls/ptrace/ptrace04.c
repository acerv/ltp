// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2008 Analog Devices Inc.
 * Copyright (c) Linux Test Project, 2008-2026
 */

/*\
 * Verify that :manpage:`ptrace(2)` PTRACE_PEEKUSER returns the same register
 * values as PTRACE_GETREGS for a stopped child process.
 *
 * A child is forked and ptraced. The parent reads each register via
 * PTRACE_PEEKUSER and compares with the corresponding field obtained via
 * PTRACE_GETREGS. The comparison is done twice with different memset poison
 * bytes (0x00 and 0xff) to detect partial-read issues. The sequence is
 * repeated after advancing the child past a syscall boundary with
 * PTRACE_SYSCALL.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "tst_test.h"

#if defined(HAVE_STRUCT_PTRACE_REGS) && defined(PTRACE_GETREGS)

#define R(r) { .name = "PT_" #r, .off = PT_##r },
static struct {
	const char *name;
	long off;
} regs[] = {
#ifdef __bfin__
	R(ORIG_R0) R(ORIG_P0)
	    R(R0) R(R1) R(R2) R(R3) R(R4) R(R5) R(R6) R(R7)
	    R(P0) R(P1) R(P2) R(P3) R(P4) R(P5) R(FP) R(USP)
	    R(I0) R(I1) R(I2) R(I3)
	    R(M0) R(M1) R(M2) R(M3)
	    R(L0) R(L1) R(L2) R(L3)
	    R(B0) R(B1) R(B2) R(B3)
	    R(A0X) R(A0W) R(A1X) R(A1W)
	    R(LC0) R(LC1) R(LT0) R(LT1) R(LB0) R(LB1)
	    R(ASTAT)
	    R(RETS) R(PC) R(RETX) R(RETN) R(RETE)
	    R(SEQSTAT) R(IPEND) R(SYSCFG)
#endif
};

static void compare_registers(pid_t child, unsigned char poison)
{
	ptrace_regs pt_regs;
	size_t i;
	long ret;
	bool failed = false;

	memset(&pt_regs, poison, sizeof(pt_regs));
	errno = 0;
	ret = ptrace(PTRACE_GETREGS, child, NULL, &pt_regs);
	if (ret && errno) {
		tst_res(TFAIL | TERRNO, "PTRACE_GETREGS failed");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(regs); ++i) {
		errno = 0;
		ret = ptrace(PTRACE_PEEKUSER, child,
				(void *)regs[i].off, NULL);
		if (ret && errno) {
			tst_res(TFAIL | TERRNO,
				"PTRACE_PEEKUSER: register %s (offset %li) failed",
				regs[i].name, regs[i].off);
			failed = true;
			continue;
		}

		long *pt_val = (void *)&pt_regs + regs[i].off;

		if (*pt_val != ret) {
			tst_res(TFAIL,
				"register %s (offset %li) did not match: GETREGS: 0x%08lx PEEKUSER: 0x%08lx",
				regs[i].name, regs[i].off, *pt_val, ret);
			failed = true;
		}
	}

	tst_res(failed ? TFAIL : TPASS,
		"PTRACE PEEKUSER/GETREGS (poison 0x%02x)", poison);
}

static void run(void)
{
	pid_t child;
	int status;

	if (ARRAY_SIZE(regs) == 0)
		tst_brk(TCONF, "test not supported for this arch");

	child = SAFE_FORK();

	if (!child) {
		SAFE_PTRACE(PTRACE_TRACEME, 0, NULL, NULL);
		raise(SIGSTOP);

		/* keep the child alive doing harmless syscalls */
		int i = 60;

		while (i--)
			close(-100);

		exit(0);
	}

	SAFE_WAITPID(child, &status, WUNTRACED);
	if (!WIFSTOPPED(status)) {
		tst_brk(TBROK, "child was not stopped: %s",
			tst_strstatus(status));
	}

	tst_res(TINFO, "Child stopped, comparing registers");
	compare_registers(child, 0x00);
	compare_registers(child, 0xff);

	/* advance child past one syscall boundary */
	errno = 0;
	if (ptrace(PTRACE_SYSCALL, child, NULL, NULL) && errno)
		tst_brk(TBROK | TERRNO, "PTRACE_SYSCALL failed");

	SAFE_WAITPID(child, &status, 0);
	if (!WIFSTOPPED(status)) {
		tst_brk(TBROK, "child not stopped after PTRACE_SYSCALL: %s",
			tst_strstatus(status));
	}

	tst_res(TINFO, "After syscall in child");
	compare_registers(child, 0x00);
	compare_registers(child, 0xff);

	SAFE_PTRACE(PTRACE_KILL, child, NULL, NULL);
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
};

#else
TST_TEST_TCONF("system does not have ptrace_regs structure or PTRACE_GETREGS");
#endif
