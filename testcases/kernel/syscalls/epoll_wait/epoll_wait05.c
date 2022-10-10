// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that epoll receives EPOLLHUP/EPOLLRDHUP event when we hang a reading
 * half-socket we are polling on.
 */

#include <poll.h>
#include <sys/epoll.h>
#include "tst_test.h"

static int sockfd;
static int epfd;

static void cleanup(void)
{
	if (epfd > 0)
		SAFE_CLOSE(epfd);

	if (sockfd > 0)
		SAFE_CLOSE(sockfd);
}

static void run(void)
{
	int res;
	struct epoll_event evt_req;
	struct epoll_event evt_rec;

	sockfd = SAFE_SOCKET(AF_INET, SOCK_STREAM, 0);

	epfd = epoll_create1(0);
	if (epfd == -1)
		tst_brk(TBROK | TERRNO, "fail to create epoll instance");

	tst_res(TINFO, "Polling on socket");

	evt_req.events = EPOLLRDHUP;
	res = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &evt_req);
	if (res == -1)
		tst_brk(TBROK | TERRNO, "epoll_ctl failure");

	tst_res(TINFO, "Hang reading half-socket");
	shutdown(sockfd, SHUT_RD);

	res = epoll_wait(epfd, &evt_rec, 1, 2000);
	if (res <= 0) {
		tst_res(TFAIL | TERRNO, "epoll_wait() returned %i", res);
		return;
	}

	if (evt_rec.events & EPOLLHUP)
		tst_res(TPASS, "Received EPOLLHUP");
	else
		tst_res(TFAIL, "EPOLLHUP has not been received");

	if (evt_rec.events & EPOLLRDHUP)
		tst_res(TPASS, "Received EPOLLRDHUP");
	else
		tst_res(TFAIL, "EPOLLRDHUP has not been received");

	SAFE_CLOSE(epfd);
}

static struct tst_test test = {
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1,
};
