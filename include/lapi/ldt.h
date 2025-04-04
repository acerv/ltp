// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 SUSE LLC Ricardo B. Marlière <rbm@suse.com>
 */

#ifndef LAPI_LDT_H__
#define LAPI_LDT_H__

#include "config.h"
#include "lapi/syscalls.h"
#include <asm/ldt.h>

static inline int modify_ldt(int func, const struct user_desc *ptr,
			     unsigned long bytecount)
{
	return tst_syscall(__NR_modify_ldt, func, ptr, bytecount);
}

static inline int safe_modify_ldt(const char *file, const int lineno, int func,
				  const struct user_desc *ptr,
				  unsigned long bytecount)
{
	int rval;

	rval = modify_ldt(func, ptr, bytecount);
	if (rval == -1)
		tst_brk_(file, lineno, TBROK | TERRNO,
			 "modify_ldt(%d, %p, %lu)", func, ptr, bytecount);

	return rval;
}

#define SAFE_MODIFY_LDT(func, ptr, bytecount) \
	safe_modify_ldt(__FILE__, __LINE__, (func), (ptr), (bytecount))

static inline int set_thread_area(const struct user_desc *u_info)
{
	return tst_syscall(__NR_set_thread_area, u_info);
}

static inline int get_thread_area(const struct user_desc *u_info)
{
	return tst_syscall(__NR_get_thread_area, u_info);
}

#endif /* LAPI_LDT_H__ */
