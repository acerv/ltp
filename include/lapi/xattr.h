// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Linux Test Project
 */

#ifndef LAPI_XATTR_H__
#define LAPI_XATTR_H__

#include <stdint.h>

#ifndef STRUCT_XATTR_ARGS
struct xattr_args {
	void *value;
	uint32_t size;
	uint32_t flags;
};
#endif

#endif /* LAPI_XATTR_H__ */
