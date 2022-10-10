// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that edge triggered behavior is correctly handled by epoll.
 *
 * [Algorithm]
 * 
 * 1. The file descriptor that represents the read side of a pipe (rfd) is
 *    registered on the epoll instance.
 * 2. A pipe writer writes 2 kB of data on the write side of the pipe.
 * 3. A call to epoll_wait(2) is done that will return rfd as a ready file
 *    descriptor.
 * 4. The pipe reader reads 1 kB of data from rfd.
 * 5. A call to epoll_wait(2) should fail because there's data left to read.
 */

#include <poll.h>
#include <sys/epoll.h>
#include "tst_test.h"

#define WRITE_SIZE 2048
#define READ_SIZE 1024

static int fds[2];
static int epfd;

static void cleanup(void)
{
	if (epfd > 0)
		SAFE_CLOSE(epfd);

	if (fds[0] > 0)
		SAFE_CLOSE(fds[0]);

	if (fds[1] > 0)
		SAFE_CLOSE(fds[1]);
}

static void run(void)
{
	int res;
	char buff[WRITE_SIZE];
	struct epoll_event evt_receive;
	struct epoll_event evt_request;

	SAFE_PIPE2(fds, O_NONBLOCK);

	evt_request.events = EPOLLIN | EPOLLET;
	evt_request.data.fd = fds[0];

	epfd = epoll_create(2);
	if (epfd == -1)
		tst_brk(TBROK | TERRNO, "fail to create epoll instance");

	tst_res(TINFO, "Polling on channel with EPOLLET");

	res = epoll_ctl(epfd, EPOLL_CTL_ADD, fds[0], &evt_request);
	if (res == -1)
		tst_brk(TBROK | TERRNO, "epoll_ctl failure");

	tst_res(TINFO, "Write bytes on channel");

	memset(buff, 'a', WRITE_SIZE);
	SAFE_WRITE(0, fds[1], buff, WRITE_SIZE);

	res = epoll_wait(epfd, &evt_receive, 1, 2000);
	if (res <= 0) {
		tst_res(TFAIL | TERRNO, "epoll_wait() returned %i", res);
		return;
	}

	if ((evt_receive.events & EPOLLIN) == 0) {
		tst_brk(TFAIL, "Didn't receive any data");
		return;
	}

	tst_res(TINFO, "Received EPOLLIN event. Read half bytes from channel");

	memset(buff, 0, READ_SIZE);
	SAFE_READ(1, evt_receive.data.fd, buff, READ_SIZE);

	res = epoll_wait(epfd, &evt_receive, 1, 200);

	tst_res(TINFO, "epoll_wait returned %d", res);

	SAFE_CLOSE(fds[0]);
	SAFE_CLOSE(fds[1]);
}

static struct tst_test test = {
	.cleanup = cleanup,
	.test_all = run,
};
