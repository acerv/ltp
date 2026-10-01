/*
 * Copyright (c) 2004, Bull SA. All rights reserved.
 * Created by:  Laurent.Vivier@bull.net
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.
 */

/*
 * Test that aio_suspend() with a NULL timeout waits for pending I/O.
 *
 * Steps:
 * 1. Queue enough asynchronous writes to fill a socket buffer.
 * 2. Check that the last write is still pending.
 * 3. Have a second thread wait for the main thread to sleep before
 *    draining the socket.
 * 4. Call aio_suspend() and check that the write has completed on return.
 */

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "posixtest.h"
#include "aio_test.h"
#include "proc.h"

#define TNAME "aio_suspend/1-1.c"
#define WRITE_COUNT 8
#define WAIT_FOR_AIOCB (WRITE_COUNT - 1)

/* Shared with the draining thread; result is read after pthread_join(). */
static int fds[2];
static struct aiocb aiocbs[WRITE_COUNT];
static sem_t start;
static int drain_result;

static void *drain_socket(void *arg PTS_ATTRIBUTE_UNUSED)
{
	char buf;
	int i;
	ssize_t ret;

	while (sem_wait(&start) == -1) {
		if (errno == EINTR)
			continue;
		perror(TNAME " sem_wait()");
		exit(PTS_UNRESOLVED);
	}

	/* getpid() identifies the main thread, not this draining thread. */
	if (tst_process_state_wait3(getpid(), 'S', 10))
		drain_result = PTS_UNRESOLVED;

	/* Leave one datagram for cleanup_aio(); discard the other payloads. */
	for (i = 0; i < WRITE_COUNT - 1; i++) {
		do {
			ret = read(fds[1], &buf, sizeof(buf));
		} while (ret == -1 && errno == EINTR);

		if (ret != sizeof(buf)) {
			printf(TNAME " Error reading socket: %zd (%s)\n",
			       ret, ret == -1 ? strerror(errno) : "short read");
			exit(PTS_UNRESOLVED);
		}
	}

	return NULL;
}

int test_main(int argc PTS_ATTRIBUTE_UNUSED, char **argv PTS_ATTRIBUTE_UNUSED)
{
	const struct aiocb *list[] = {NULL, &aiocbs[WAIT_FOR_AIOCB]};
	pthread_t thread;
	int i, ret, err;
	int result = PTS_PASS;
	ssize_t len;

	if (sysconf(_SC_ASYNCHRONOUS_IO) < 200112L) {
		printf(TNAME " Test UNSUPPORTED: asynchronous I/O\n");
		return PTS_UNSUPPORTED;
	}

	if (setup_aio(TNAME, fds, aiocbs, WRITE_COUNT))
		return PTS_UNRESOLVED;

	if (sem_init(&start, 0, 0) == -1) {
		perror(TNAME " sem_init()");
		exit(PTS_UNRESOLVED);
	}

	ret = pthread_create(&thread, NULL, drain_socket, NULL);
	if (ret) {
		printf(TNAME " pthread_create(): %s\n", strerror(ret));
		exit(PTS_UNRESOLVED);
	}

	for (i = 0; i < WRITE_COUNT; i++) {
		if (aio_write(&aiocbs[i]) == -1) {
			perror(TNAME " aio_write()");
			exit(PTS_UNRESOLVED);
		}
	}

	err = aio_error(&aiocbs[WAIT_FOR_AIOCB]);
	if (err != EINPROGRESS) {
		printf(TNAME " Expected pending write, got status %d\n", err);
		exit(PTS_UNRESOLVED);
	}

	/* After releasing the drainer, only aio_suspend() should block us. */
	if (sem_post(&start) == -1) {
		perror(TNAME " sem_post()");
		exit(PTS_UNRESOLVED);
	}

	ret = aio_suspend(list, 2, NULL);
	if (ret) {
		perror(TNAME " aio_suspend()");
		exit(PTS_FAIL);
	}

	err = aio_error(&aiocbs[WAIT_FOR_AIOCB]);
	if (err != 0) {
		printf(TNAME " Test FAILED: write status after suspend: %d\n", err);
		exit(PTS_FAIL);
	}

	ret = pthread_join(thread, NULL);
	if (ret) {
		printf(TNAME " pthread_join(): %s\n", strerror(ret));
		exit(PTS_UNRESOLVED);
	}
	if (drain_result != PTS_PASS)
		result = drain_result;

	/* All writes can now complete, regardless of their execution order. */
	for (i = 0; i < WRITE_COUNT; i++) {
		const struct aiocb *req = &aiocbs[i];

		while ((err = aio_error(req)) == EINPROGRESS) {
			if (aio_suspend(&req, 1, NULL) == -1 && errno != EINTR) {
				perror(TNAME " aio_suspend() during cleanup");
				exit(PTS_UNRESOLVED);
			}
		}
		if (err) {
			printf(TNAME " Test FAILED: write %d status %d\n", i, err);
			exit(PTS_FAIL);
		}
	}

	/* Only one datagram remains; the helper frees its buffer and sockets. */
	cleanup_aio(fds, &aiocbs[WAIT_FOR_AIOCB], 1);
	for (i = 0; i < WRITE_COUNT; i++) {
		len = aio_return(&aiocbs[i]);
		if (len != (ssize_t)aiocbs[i].aio_nbytes) {
			printf(TNAME " Test FAILED: write %d returned %zd, expected %zu\n",
			       i, len, aiocbs[i].aio_nbytes);
			result = PTS_FAIL;
		}
		if (i != WAIT_FOR_AIOCB)
			free((void *)aiocbs[i].aio_buf);
	}

	if (sem_destroy(&start) == -1) {
		perror(TNAME " sem_destroy()");
		result = PTS_UNRESOLVED;
	}
	if (result == PTS_PASS)
		printf("Test PASSED\n");

	return result;
}
