# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>

import os
import re
import sys

if len(sys.argv) == 1:
    print("ERROR: no output file is given")
    exit(1)

architectures = [
    'aarch64',
    'arc',
    'arm',
    'hppa',
    'i386',
    'ia64',
    'loongarch',
    'mips_n32',
    'mips_n64',
    'mips_o32',
    'powerpc64',
    'powerpc',
    's390x',
    's390',
    'sh',
    'sparc64',
    'sparc',
    'x86_64',
]

syscalls_h = ['''/************************************************
 * GENERATED FILE: DO NOT EDIT/PATCH THIS FILE  *
 *  change your arch specific .in file instead  *
 ************************************************/

/*
 * Here we stick all the ugly *fallback* logic for linux
 * system call numbers (those __NR_ thingies).
 *
 * Licensed under the GPLv2 or later, see the COPYING file.
 */

#ifndef LAPI_SYSCALLS_H__
#define LAPI_SYSCALLS_H__

#include <errno.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include "cleanup.c"

#ifdef TST_TEST_H__
#define TST_SYSCALL_BRK__(NR, SNR) ({ \\
	tst_brk(TCONF, \\
		"syscall(%d) " SNR " not supported on your arch", NR); \\
})
#else
#define TST_SYSCALL_BRK__(NR, SNR) ({ \\
	tst_brkm(TCONF, CLEANUP, \\
		"syscall(%d) " SNR " not supported on your arch", NR); \\
})
#endif

#define tst_syscall(NR, ...) ({ \\
	intptr_t tst_ret; \\
	if (NR == __LTP__NR_INVALID_SYSCALL) { \\
		errno = ENOSYS; \\
		tst_ret = -1; \\
	} else { \\
		tst_ret = syscall(NR, ##__VA_ARGS__); \\
	} \\
	if (tst_ret == -1 && errno == ENOSYS) { \\
		TST_SYSCALL_BRK__(NR, #NR); \\
	} \\
	tst_ret; \\
})

#define __LTP__NR_INVALID_SYSCALL -1

''']

curr_dir = os.path.dirname(os.path.abspath(__file__))
common_stubs = []

for arch in architectures:
    if arch == 'sparc64':
        syscalls_h.append("#if defined(__sparc__) && defined(__arch64__)\n")
    elif arch == 'sparc':
        syscalls_h.append("#if defined(__sparc__) && !defined(__arch64__)\n")
    elif arch == 's390':
        syscalls_h.append("#if defined(__s390__) && !defined(__s390x__)\n")
    elif arch == 'mips_n32':
        syscalls_h.append("#if defined(__mips__) && defined(_ABIN32)\n")
    elif arch == 'mips_n64':
        syscalls_h.append("#if defined(__mips__) && defined(_ABI64)\n")
    elif arch == 'mips_o32':
        syscalls_h.append("#if defined(__mips__) && defined(_ABIO32) && _MIPS_SZLONG == 32\n")
    else:
        syscalls_h.append(f"#ifdef __{arch}__\n")

    syscalls = []
    myfile = os.path.join(curr_dir, f"{arch}.in")

    with open(myfile, 'r') as data:
        syscalls = data.read().split('\n')

    for line in syscalls:
        syscall = line.split(' ', maxsplit=1)
        if len(syscall) < 2:
            continue

        syscall_nr = f"__NR_{syscall[0]}"
        syscall_val = syscall[1]

        syscalls_h.append(f"# ifndef {syscall_nr}\n")
        syscalls_h.append(f"#  define {syscall_nr} {syscall_val}\n")
        syscalls_h.append(f"# endif\n")

        if syscall_nr not in common_stubs:
            common_stubs.append(syscall_nr)

    syscalls_h.append("#endif\n\n")

syscalls_h.append("/* Common stubs */\n")

for stub in common_stubs:
    syscalls_h.append(f"# ifndef {stub}\n")
    syscalls_h.append(f"#  define {stub} __LTP__NR_INVALID_SYSCALL\n")
    syscalls_h.append(f"# endif\n")

syscalls_h.append("#endif\n\n")

with open(sys.argv[1], 'w') as fdata:
    fdata.write(''.join(syscalls_h))
