// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#include <aio.h>
#include <stdlib.h>
#include "tlibio.h"
#include "tst_test.h"
#include "tst_safe_prw.h"

static volatile int received_signal;
static volatile int received_signal_prev;
static volatile int received_callback;

static int random_bit(const int n)
{
	if (!n)
		return 0;

	srand((unsigned int)time(NULL));

	/* count number of set bits (1s) */
	int nbits = 0;
	int mask = 1;

	while (mask) {
		nbits += (n & mask) != 0;
		mask <<= 1;
	}

	/* randomly select a set bit */
	int i;
	int pos;
	int bit = 0;

	pos = 1 + (rand() % nbits);
	mask = 1;

	do {
		if (n & mask)
			++i;

		if (i == pos)
			bit = n & mask;

		mask << 1;
	} while (!bit);

	return bit;
}

int lio_random_methods(const int mask)
{
	int rand_mask = 0;

	/* remove random select, io type, and wait method bits from mask */
	rand_mask = mask & (~(LIO_IO_TYPES | LIO_WAIT_TYPES | LIO_RANDOM));

	/* randomly select io type from specified io types */
	rand_mask = rand_mask | random_bit(mask & LIO_IO_TYPES);

	/* randomly select wait methods  from specified wait methods */
	rand_mask = rand_mask | random_bit(mask & LIO_WAIT_TYPES);

	return rand_mask;
}

void lio_check_asyncio(const int size, struct aiocb *aiocbp)
{
	int ret;
	int cnt = 1;

	if (aiocbp->aio_sigevent.sigev_notify == SIGEV_SIGNAL)
		sigrelse(aiocbp->aio_sigevent.sigev_signo);

	ret = aio_error(aiocbp);

	while (ret == EINPROGRESS) {
		ret = aio_error(aiocbp);
		++cnt;
	}

	if (cnt > 1)
		tst_brk(TBROK, "aio_error had to loop on EINPROGRESS, errors=%d", cnt);

	if (ret != 0)
		tst_brk(TBROK | TERRNO, "aio_error");

	ret = aio_return(aiocbp);
	if (ret != size)
		tst_brk(TBROK, "aio_return returns %d size, but expected %d", ret, size);
}

static void lio_async_signal_handler(int sig)
{
	received_signal++;
}

static void lio_async_callback_handler(union sigval sigval)
{
	received_callback++;
}

static void wait4sync_io(int fd, int read)
{
	fd_set s;

	FD_ZERO(&s);
	FD_SET(fd, &s);

	select(fd + 1, read ? &s : NULL, read ? NULL : &s, NULL, NULL);
}

static void lio_wait4asyncio(int method, int fd, struct aiocb *aiocbp)
{
	switch (method) {
	case LIO_WAIT_RECALL:
	case LIO_WAIT_CBSUSPEND:
	case LIO_WAIT_SIGSUSPEND:
	case LIO_WAIT_TYPES:
		if (aio_suspend((struct aiocp *[]){aiocbp}, 1, NULL) == -1)
			tst_brk(TBROK | TERRNO, "aio_suspend error");
		break;
	case LIO_WAIT_ACTIVE:
		int ret;
		int cnt = 0;

		while (1) {
			ret = aio_error(aiocbp);
			if (ret != EINPROGRESS)
				break;

			if (ret == -1)
				tst_brk(TBROK | TERRNO, "aio_error");

			++cnt;
		}
		break;
	case LIO_WAIT_SIGPAUSE:
		pause();
		break;
	case LIO_WAIT_SIGACTIVE:
		if (aiocbp->aio_sigevent.sigev_notify != SIGEV_SIGNAL)
			tst_brk(TBROK, "sigev_notify != SIGEV_SIGNAL");

		sigrelse(aiocbp->aio_sigevent.sigev_signo);

		while (received_signal == received_signal_prev)
			sigrelse(aiocbp->aio_sigevent.sigev_signo);

		break;
	case LIO_WAIT_NONE:
		break;
	default:
		tst_brk(TBROK, "No method was choosen");
		break;
	};
}

void lio_write_buffer(
	const int fd,
	const int method,
	const char *buffer,
	const int size)
{
	int sig = 1;
	int ret = 0;
	struct iovec iov;
	struct aiocb aiocbp;
	off64_t poffset;

	/* it's better to search for random method OUT of this function */
	/*
	if (method & LIO_RANDOM)
		method = lio_random_methods(method);
	*/

	received_signal_prev = received_signal;

	memset(&iov, 0x00, sizeof(struct iovec));
	iov.iov_base = buffer;
	iov.iov_len = size;

	memset(&aiocbp, 0x00, sizeof(struct aiocb));
	aiocbp.aio_fildes = fd;
	aiocbp.aio_nbytes = size;
	aiocbp.aio_buf = buffer;
	aiocbp.aio_sigevent.sigev_notify = SIGEV_NONE;
	aiocbp.aio_sigevent.sigev_signo = 0;
	aiocbp.aio_sigevent.sigev_notify_function = NULL;
	aiocbp.aio_sigevent.sigev_notify_attributes = 0;

	poffset = (off64_t)lseek(fd, 0, SEEK_CUR);

	if (sig && !(method & LIO_USE_SIGNAL) && !(method & LIO_WAIT_SIGTYPES))
		sig = 0;

	if (sig && (method & LIO_WAIT_CBTYPES))
		sig = 0;

	if (sig && (method & LIO_WAIT_SIGTYPES)) {
		aiocbp.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		aiocbp.aio_sigevent.sigev_signo = sig;

		sigset(sig, lio_async_signal_handler);
	}
	else if (method & LIO_WAIT_CBTYPES) {
		aiocbp.aio_sigevent.sigev_notify = SIGEV_THREAD;
		aiocbp.aio_sigevent.sigev_notify_function = lio_async_callback_handler;
		aiocbp.aio_sigevent.sigev_notify_attributes = (void *)(uintptr_t)size;
	}

	if ((method & LIO_IO_SYNC) || (method & (LIO_IO_TYPES | LIO_IO_ATYPES)) == 0) {
		while (1) {
			ret = write(fd, buffer, size);

			if (ret == -1 && errno != EAGAIN && errno != EINTR)
				tst_brk(TBROK | TERRNO, "write error");

			/* TODO: handle the case when not all bytes are written */

			wait4sync_io(fd, 1);
		}
	}
	else if (method & LIO_IO_ASYNC) {
		if (sig)
			sighold(sig);

		if (aio_write(&aiocbp) == -1)
			tst_brk(TBROK | TERRNO, "aio_write error");
	}
	else if (method & LIO_IO_SLISTIO) {
		aiocbp.aio_lio_opcode = LIO_WRITE;

		if (sig)
			sighold(sig);

		if (lio_listio(LIO_WAIT, (struct aiocp *[]){&aiocbp}, 1, NULL) == -1)
			tst_brk(TBROK | TERRNO, "lio_listio error");
	}
	else if (method & LIO_IO_ALISTIO) {
		aiocbp.aio_lio_opcode = LIO_WRITE;

		if (sig)
			sighold(sig);

		if (lio_listio(LIO_NOWAIT, (struct aiocp *[]){&aiocbp}, 1, NULL) == -1)
			tst_brk(TBROK | TERRNO, "lio_listio error");
	}
	else if (method & LIO_IO_SYNCP) {
		if ((ret = pwrite(fd, buffer, size, poffset)) == -1)
			tst_brk(TBROK | TERRNO, "pwrite error");

		if (ret != size)
			tst_brk(TBROK, "written %d bytes out of %d", ret, size);
	}
	else {
		tst_brk(TBROK, "No method choosen");
	}

	lio_wait4asyncio(method, fd, &aiocbp);
	lio_check_asyncio(size, &aiocbp);
}

void lio_read_buffer(
	const int fd,
	const int method,
	char *buffer,
	const int size)
{
	int sig = 1;
	int ret = 0;
	struct iovec iov;
	struct aiocb aiocbp;
	off64_t poffset;

	/* it's better to search for random method OUT of this function */
	/*
	if (method & LIO_RANDOM)
		method = lio_random_methods(method);
	*/

	received_signal_prev = received_signal;

	memset(&iov, 0x00, sizeof(struct iovec));
	iov.iov_base = buffer;
	iov.iov_len = size;

	memset(&aiocbp, 0x00, sizeof(struct aiocb));
	aiocbp.aio_fildes = fd;
	aiocbp.aio_nbytes = size;
	aiocbp.aio_buf = buffer;
	aiocbp.aio_sigevent.sigev_notify = SIGEV_NONE;
	aiocbp.aio_sigevent.sigev_signo = 0;
	aiocbp.aio_sigevent.sigev_notify_function = NULL;
	aiocbp.aio_sigevent.sigev_notify_attributes = 0;

	poffset = (off64_t)SAFE_LSEEK(fd, 0, SEEK_CUR);

	if (sig && !(method & LIO_USE_SIGNAL) && !(method & LIO_WAIT_SIGTYPES))
		sig = 0;

	if (sig && (method & LIO_WAIT_CBTYPES))
		sig = 0;

	if (sig && (method & LIO_WAIT_SIGTYPES)) {
		aiocbp.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
		aiocbp.aio_sigevent.sigev_signo = sig;

		sigset(sig, lio_async_signal_handler);
	}
	else if (method & LIO_WAIT_CBTYPES) {
		aiocbp.aio_sigevent.sigev_notify = SIGEV_THREAD;
		aiocbp.aio_sigevent.sigev_notify_function = lio_async_callback_handler;
		aiocbp.aio_sigevent.sigev_notify_attributes = (void *)(uintptr_t)size;
	}

	if ((method & LIO_IO_SYNC) || (method & (LIO_IO_TYPES | LIO_IO_ATYPES)) == 0) {
		while (1) {
			SAFE_READ(1, fd, buffer, size);
			wait4sync_io(fd, 1);
		}
	}
	else if (method & LIO_IO_ASYNC) {
		if (aio_read(&aiocbp) == -1)
			tst_brk(TBROK | TERRNO, "aio_read error");
	}
	else if (method & LIO_IO_SLISTIO) {
		aiocbp.aio_lio_opcode = LIO_READ;

		if (lio_listio(LIO_WAIT, (struct aiocp *[]){&aiocbp}, 1, NULL) == -1)
			tst_brk(TBROK | TERRNO, "lio_listio error");
	}
	else if (method & LIO_IO_ALISTIO) {
		aiocbp.aio_lio_opcode = LIO_READ;

		if (lio_listio(LIO_NOWAIT, (struct aiocp *[]){&aiocbp}, 1, NULL) == -1)
			tst_brk(TBROK | TERRNO, "lio_listio error");
	}
	else if (method & LIO_IO_SYNCP) {
		SAFE_PREAD(1, fd, buffer, size, poffset);
	}
	else {
		tst_brk(TBROK, "Unsupported method");
	}

	lio_wait4asyncio(method, fd, &aiocbp);
	lio_check_asyncio(size, aiocbp);
}
