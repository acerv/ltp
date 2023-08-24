// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
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
 * 4. The pipe reader reads 1 kB (half) of data from rfd.
 * 5. A call to epoll_wait(2) should hang because there's data left to read.
 */

#include "tst_test.h"
#include "tst_epoll.h"

#define WRITE_SIZE 2048
#define READ_SIZE (WRITE_SIZE / 2)

static int fds[2];
static int epfd;

static void setup(void)
{
	SAFE_PIPE2(fds, O_NONBLOCK);
}

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
	char buff[WRITE_SIZE];
	struct epoll_event evt_receive;

	tst_res(TINFO, "Polling on channel with EPOLLET");

	epfd = SAFE_EPOLL_CREATE1(0);

	SAFE_EPOLL_CTL(epfd, EPOLL_CTL_ADD, fds[0], &((struct epoll_event) {
		.events = EPOLLIN | EPOLLET,
		.data.fd = fds[0],
	}));
	SAFE_EPOLL_CTL(epfd, EPOLL_CTL_ADD, fds[1], &((struct epoll_event) {
		.events = EPOLLOUT | EPOLLET,
		.data.fd = fds[1],
	}));

	/* we obtain EPOLLOUT when pipe is ready to be written */
	TST_EXP_EQ_LI(SAFE_EPOLL_WAIT(epfd, &evt_receive, 1, 0), 1);
	TST_EXP_EQ_LI(evt_receive.data.fd, fds[1]);
	TST_EXP_EQ_LI(evt_receive.events & EPOLLOUT, EPOLLOUT);

	tst_res(TINFO, "Write bytes on channel: %d bytes", WRITE_SIZE);

	memset(buff, 'a', WRITE_SIZE);
	SAFE_WRITE(SAFE_WRITE_ANY, fds[1], buff, WRITE_SIZE);
	TST_EXP_EQ_LI(SAFE_EPOLL_WAIT(epfd, &evt_receive, 1, 0), 1);
	TST_EXP_EQ_LI(evt_receive.data.fd, fds[0]);
	TST_EXP_EQ_LI(evt_receive.events & EPOLLIN, EPOLLIN);

	tst_res(TINFO, "Read half bytes from channel: %d bytes", READ_SIZE);

	memset(buff, 0, WRITE_SIZE);
	SAFE_READ(1, fds[0], buff, READ_SIZE);
	TST_EXP_EQ_LI(SAFE_EPOLL_WAIT(epfd, &evt_receive, 1, 0), 0);

	tst_res(TINFO, "Read remaining bytes from channel: %d bytes", READ_SIZE);

	SAFE_READ(1, fds[0], buff + READ_SIZE, READ_SIZE);
	TST_EXP_EQ_LI(SAFE_EPOLL_WAIT(epfd, &evt_receive, 1, 0), 1);
	TST_EXP_EQ_LI(evt_receive.data.fd, fds[1]);
	TST_EXP_EQ_LI(evt_receive.events & EPOLLOUT, EPOLLOUT);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
};
