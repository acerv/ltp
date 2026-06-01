// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 Linux Test Project
 * Copyright (c) 2025 Linux Test Project
 */

/*\
 * Reproducer for a race condition in unix_release() triggered by
 * :manpage:`sendmsg(2)` that caused a kernel NULL pointer dereference in
 * selinux_socket_unix_may_send().
 *
 * [Algorithm]
 *
 * - Fork multiple client/server pairs that rapidly create, bind, sendmsg,
 *   and close AF_UNIX DGRAM sockets in a tight loop.
 * - A shared atomic flag controls the loop duration.
 * - If the race condition exists, the kernel will crash with a NULL pointer
 *   dereference. If the test completes, it passes.
 *
 * Each pair uses a pipe to pace the client and server loops. This is a
 * deliberate exception to the usual preference for the TST_CHECKPOINT_*
 * API: the checkpoint wake path busy-retries at 1ms granularity and offers
 * no EOF, which would both throttle this timing-sensitive race and
 * complicate clean loop teardown. A raw pipe keeps the loop tight and exits
 * cleanly via EPIPE when a peer goes away.
 */

#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "tst_test.h"
#include "tst_atomic.h"

#define STRESS_SECONDS 5

static tst_atomic_t *running;

static void client(int id, int pipefd)
{
	int fd;
	char data[] = "123456789";
	struct iovec w;
	struct sockaddr_un sa;
	struct msghdr mh;
	char sync_byte = 1;

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	snprintf(sa.sun_path, sizeof(sa.sun_path), "socket_test%d", id);

	w.iov_base = data;
	w.iov_len = sizeof(data);

	memset(&mh, 0, sizeof(mh));
	mh.msg_name = &sa;
	mh.msg_namelen = sizeof(sa);
	mh.msg_iov = &w;
	mh.msg_iovlen = 1;

	while (tst_atomic_load(running)) {
		fd = socket(AF_UNIX, SOCK_DGRAM, 0);
		if (fd < 0)
			continue;

		if (write(pipefd, &sync_byte, 1) < 1) {
			close(fd);
			break;
		}

		sendmsg(fd, &mh, MSG_NOSIGNAL);
		close(fd);
	}
}

static void server(int id, int pipefd)
{
	int fd;
	struct sockaddr_un sa;
	char sync_byte;

	memset(&sa, 0, sizeof(sa));
	sa.sun_family = AF_UNIX;
	snprintf(sa.sun_path, sizeof(sa.sun_path), "socket_test%d", id);

	while (tst_atomic_load(running)) {
		fd = socket(AF_UNIX, SOCK_DGRAM, 0);
		if (fd < 0)
			continue;

		unlink(sa.sun_path);
		if (bind(fd, (struct sockaddr *)&sa,
			 sizeof(struct sockaddr_un))) {
			close(fd);
			continue;
		}

		if (read(pipefd, &sync_byte, 1) < 1) {
			close(fd);
			break;
		}

		close(fd);
	}

	unlink(sa.sun_path);
}

static void run(void)
{
	int i, status;
	int child_pairs = sysconf(_SC_NPROCESSORS_ONLN) * 4;
	int child_count = 0;
	int pipefd[2];
	int failed = 0;
	pid_t pid;
	pid_t *child_pids;

	child_pids = SAFE_MALLOC(sizeof(pid_t) * child_pairs * 2);
	tst_atomic_store(1, running);

	for (i = 0; i < child_pairs; i++) {
		SAFE_PIPE(pipefd);

		pid = SAFE_FORK();
		if (!pid) {
			SAFE_CLOSE(pipefd[1]);
			server(i, pipefd[0]);
			exit(0);
		}
		child_pids[child_count++] = pid;

		pid = SAFE_FORK();
		if (!pid) {
			SAFE_CLOSE(pipefd[0]);
			client(i, pipefd[1]);
			exit(0);
		}
		child_pids[child_count++] = pid;

		SAFE_CLOSE(pipefd[0]);
		SAFE_CLOSE(pipefd[1]);
	}

	sleep(STRESS_SECONDS);
	tst_atomic_store(0, running);

	for (i = 0; i < child_count; i++) {
		SAFE_WAITPID(child_pids[i], &status, 0);
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
			tst_res(TFAIL, "child %d exited abnormally", i);
			failed = 1;
		}
	}

	free(child_pids);

	if (!failed)
		tst_res(TPASS, "sendmsg() race in unix_release() not reproduced");
}

static void setup(void)
{
	running = SAFE_MMAP(NULL, sizeof(*running), PROT_READ | PROT_WRITE,
			    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	SAFE_SIGNAL(SIGPIPE, SIG_IGN);
}

static void cleanup(void)
{
	if (running)
		SAFE_MUNMAP(running, sizeof(*running));
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.tags = (const struct tst_tag[]) {
		{"linux-git", "ded34e0fe8fe"},
		{}
	},
};
