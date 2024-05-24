// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies the following shutdown() functionalities:
 * * SHUT_RD should enable send() ops but disable recv() ops
 * * SHUT_WR should enable recv() ops but disable send() ops
 * * SHUT_RDWR should disable both recv() and send() ops
 */

#include "tst_test.h"

#define MSGSIZE 4
#define SOCKETFILE "socket"

static struct sockaddr_un *sock_addr;

static void run_server(void)
{
	int server_sock;

	server_sock = SAFE_SOCKET(sock_addr->sun_family, SOCK_STREAM, 0);

	SAFE_BIND(server_sock,
		(struct sockaddr *)sock_addr,
		sizeof(struct sockaddr_un));
	SAFE_LISTEN(server_sock, 10);

	tst_res(TINFO, "Running server on socket file");

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_CLOSE(server_sock);
	SAFE_UNLINK(SOCKETFILE);
}

static int start_test(void)
{
	int client_sock;

	if (!SAFE_FORK()) {
		run_server();
		_exit(0);
	}

	TST_CHECKPOINT_WAIT(0);

	tst_res(TINFO, "Connecting to the server");

	client_sock = SAFE_SOCKET(sock_addr->sun_family, SOCK_STREAM, 0);
	SAFE_CONNECT(client_sock,
		(struct sockaddr *)sock_addr,
		sizeof(struct sockaddr_un));

	return client_sock;
}

static void test_shutdown_recv(void)
{
	int client_sock;
	char buff[MSGSIZE] = {0};

	client_sock = start_test();

	tst_res(TINFO, "Testing SHUT_RD flag");

	TST_EXP_PASS(shutdown(client_sock, SHUT_RD));
	TST_EXP_EQ_LI(recv(client_sock, buff, MSGSIZE, 0), 0);
	TST_EXP_EQ_LI(send(client_sock, buff, MSGSIZE, 0), MSGSIZE);

	SAFE_CLOSE(client_sock);
	TST_CHECKPOINT_WAKE(0);
}

static void test_shutdown_send(void)
{
	int client_sock;
	char buff[MSGSIZE] = {0};

	client_sock = start_test();

	tst_res(TINFO, "Testing SHUT_WR flag");

	TST_EXP_PASS(shutdown(client_sock, SHUT_WR));
	TST_EXP_FAIL(recv(client_sock, buff, MSGSIZE, MSG_DONTWAIT), EWOULDBLOCK);
	TST_EXP_FAIL(send(client_sock, buff, MSGSIZE, MSG_NOSIGNAL), EPIPE);

	SAFE_CLOSE(client_sock);
	TST_CHECKPOINT_WAKE(0);
}

static void test_shutdown_both(void)
{
	int client_sock;
	char buff[MSGSIZE] = {0};

	client_sock = start_test();

	tst_res(TINFO, "Testing SHUT_RDWR flag");

	TST_EXP_PASS(shutdown(client_sock, SHUT_RDWR));
	TST_EXP_EQ_LI(recv(client_sock, buff, MSGSIZE, 0), 0);
	TST_EXP_FAIL(send(client_sock, buff, MSGSIZE, MSG_NOSIGNAL), EPIPE);

	SAFE_CLOSE(client_sock);
	TST_CHECKPOINT_WAKE(0);
}

static void run(void)
{
	test_shutdown_recv();
	test_shutdown_send();
	test_shutdown_both();
}

static void setup(void)
{
	sock_addr->sun_family = AF_UNIX;
	memcpy(sock_addr->sun_path, SOCKETFILE, sizeof(SOCKETFILE));
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.forks_child = 1,
	.needs_checkpoints = 1,
	.needs_tmpdir = 1,
	.bufs = (struct tst_buffers []) {
		{&sock_addr, .size = sizeof(struct sockaddr_un)},
		{}
	}
};
