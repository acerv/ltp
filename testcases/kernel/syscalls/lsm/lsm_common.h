/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef LSM_GET_SELF_ATTR_H
#define LSM_GET_SELF_ATTR_H

#include "tst_test.h"
#include "lapi/lsm.h"

static inline struct lsm_ctx *next_ctx(struct lsm_ctx *tctx)
{
	return (struct lsm_ctx *)((void *)tctx + sizeof(*tctx) + tctx->ctx_len);
}

static inline void read_proc_attr(const char *attr, char *val, const size_t size)
{
	int fd;
	char *ptr;
	char path[BUFSIZ];

	memset(val, 0, size);
	memset(path, 0, BUFSIZ);

	snprintf(path, BUFSIZ, "/proc/self/attr/%s", attr);

	tst_res(TINFO, "Reading %s", path);

	fd = SAFE_OPEN(path, O_RDONLY);

	if (read(fd, val, size) > 0) {
		ptr = strchr(val, '\n');
		if (ptr)
			*ptr = '\0';
	}

	SAFE_CLOSE(fd);
}

static inline int verify_enabled_lsm(const char *name)
{
	int fd;
	char data[BUFSIZ];

	fd = SAFE_OPEN("/sys/kernel/security/lsm", O_RDONLY);
	SAFE_READ(0, fd, data, BUFSIZ);
	SAFE_CLOSE(fd);

	if (!strstr(data, name)) {
		tst_res(TINFO, "%s is running", name);
		return 1;
	}

	return 0;
}
#endif
