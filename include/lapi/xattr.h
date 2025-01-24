// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Linux Test Project
 */

#ifndef LAPI_XATTR_H__
#define LAPI_XATTR_H__

#include <stdint.h>
#include <stddef.h>
#include "lapi/syscalls.h"

#ifndef STRUCT_XATTR_ARGS
struct xattr_args {
	void *value;
	uint32_t size;
	uint32_t flags;
};
#endif

int safe_setxattrat(const char *file, const int lineno,
		int dfd, const char *path, int at_flags,
		const char *key, struct xattr_args *args, size_t size)
{
	int rval;

	rval = tst_syscall(__NR_setxattrat,
		    dfd, path, at_flags, key, args, size);

	if (rval == -1) {
		if (errno == ENOTSUP) {
			tst_brk_(file, lineno, TCONF,
				"no xattr support in fs, mounted without user_xattr option "
				"or invalid namespace/name format");
			return rval;
		}

		tst_brk_(file, lineno, TBROK | TERRNO,
			"setxattrat(%d, %s, %d, %s, %p, %zu) failed",
			dfd, path, at_flags, key, args, size);
	} else if (rval) {
		tst_brk_(file, lineno, TBROK | TERRNO,
			"Invalid setxattrat(%d, %s, %d, %s, %p, %zu) return value %d",
			dfd, path, at_flags, key, args, size, rval);
	}

	return rval;
}

#define SAFE_SETXATTRAT(dfd, path, at_flags, key, args, size) \
	safe_setxattrat(__FILE__, __LINE__, \
		 (dfd), (path), (at_flags), (key), (args), (size))

int safe_getxattrat(const char *file, const int lineno,
		int dfd, const char *path, int at_flags,
		const char *key, struct xattr_args *args, size_t size)
{
	int rval;

	rval = tst_syscall(__NR_getxattrat,
		    dfd, path, at_flags, key, args, size);

	if (rval == -1) {
		if (errno == ENOTSUP) {
			tst_brk_(file, lineno, TCONF,
				"no xattr support in fs, mounted without user_xattr option "
				"or invalid namespace/name format");
			return rval;
		}

		tst_brk_(file, lineno, TBROK | TERRNO,
			"getxattrat(%d, %s, %d, %s, %p, %zu) failed",
			dfd, path, at_flags, key, args, size);
	} else if (rval < 0) {
		tst_brk_(file, lineno, TBROK | TERRNO,
			"Invalid getxattrat(%d, %s, %d, %s, %p, %zu) return value %d",
			dfd, path, at_flags, key, args, size, rval);
	}

	return rval;
}

#define SAFE_GETXATTRAT(dfd, path, at_flags, key, args, size) \
	safe_getxattrat(__FILE__, __LINE__, \
		 (dfd), (path), (at_flags), (key), (args), (size))

#endif /* LAPI_XATTR_H__ */
