// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2001
 * Ported by Wayne Boyer
 */

/*\
 * Verify that :manpage:`listen(2)` fails with the correct errno for
 * various error conditions:
 *
 * - EBADF when the socket argument is not a valid file descriptor
 * - ENOTSOCK when the socket argument is not a socket
 * - EOPNOTSUPP when the socket does not support listen (UDP)
 */

#include <sys/socket.h>
#include <netinet/in.h>

#include "tst_test.h"

static int bad_fd = 400;
static int dev_null_fd = -1;
static int udp_fd = -1;

static struct tcase {
	int *fd;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{&bad_fd, EBADF, "bad file descriptor"},
	{&dev_null_fd, ENOTSOCK, "not a socket"},
	{&udp_fd, EOPNOTSUPP, "UDP listen"},
};

static void setup(void)
{
	dev_null_fd = SAFE_OPEN("/dev/null", O_WRONLY);
	udp_fd = SAFE_SOCKET(PF_INET, SOCK_DGRAM, 0);
}

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	TST_EXP_FAIL(listen(*tc->fd, 0), tc->exp_errno, "%s", tc->desc);
}

static void cleanup(void)
{
	if (dev_null_fd != -1)
		SAFE_CLOSE(dev_null_fd);

	if (udp_fd != -1)
		SAFE_CLOSE(udp_fd);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
};
