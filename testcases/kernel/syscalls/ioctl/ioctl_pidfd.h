/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef IOCTL_PIDFD_H
#define IOCTL_PIDFD_H

#include "tst_test.h"
#include "lapi/pidfd.h"

static inline int ioctl_pidfd_info_exit_supported(void)
{
	int ret;
	pid_t pid;
	int pidfd;
	struct pidfd_info info;

	if (tst_kvercmp(6, 15, 0) >= 0)
		return 1;

	memset(&info, 0, sizeof(struct pidfd_info));
	info.mask = PIDFD_INFO_EXIT;

	pid = SAFE_FORK();
	if (!pid)
		exit(100);

	pidfd = SAFE_PIDFD_OPEN(pid, 0);
	SAFE_WAITPID(pid, NULL, 0);

	ret = ioctl(pidfd, PIDFD_GET_INFO, &info);
	if (ret == -1) {
		if (errno != ENOTTY)
			tst_brk(TBROK | TERRNO, "ioctl error");
	} else {
		if (info.mask & PIDFD_INFO_EXIT)
			return 1;
	}

	SAFE_CLOSE(pidfd);
	return 0;
}

#endif
