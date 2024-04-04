// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2000 Silicon Graphics, Inc.  All Rights Reserved.
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef _TLIBIO_H_
#define _TLIBIO_H_

#define LIO_IO_SYNC             00001   /* read/write */
#define LIO_IO_ASYNC            00002   /* reada/writea/aio_write/aio_read */
#define LIO_IO_SLISTIO          00004   /* single stride sync listio */
#define LIO_IO_ALISTIO          00010   /* single stride async listio */
#define LIO_IO_SYNCV            00020   /* single-buffer readv/writev */
#define LIO_IO_SYNCP            00040   /* pread/pwrite */
#define LIO_IO_TYPES            00061   /* all io types */
#define LIO_IO_ATYPES           00077   /* all aio types */

#define LIO_WAIT_NONE           00010000 /* return asap -- use with care */
#define LIO_WAIT_ACTIVE         00020000 /* spin looking at iosw fields, or EINPROGRESS */
#define LIO_WAIT_RECALL         00040000 /* call recall(2)/aio_suspend(3) */
#define LIO_WAIT_SIGPAUSE       00100000 /* call pause */
#define LIO_WAIT_SIGACTIVE      00200000 /* spin waiting for signal */
#define LIO_WAIT_CBSUSPEND      00400000 /* aio_suspend waiting for callback */
#define LIO_WAIT_SIGSUSPEND     01000000 /* aio_suspend waiting for signal */
#define LIO_WAIT_ATYPES         01760000 /* all async wait types, except nowait */
#define LIO_WAIT_TYPES          00020000 /* all sync wait types (sorta) */

/* all callback wait types */
# define LIO_WAIT_CBTYPES	(LIO_WAIT_CBSUSPEND)

/* all signal wait types */
# define LIO_WAIT_SIGTYPES	(LIO_WAIT_SIGPAUSE | LIO_WAIT_SIGACTIVE | LIO_WAIT_SIGSUSPEND)

/* all aio_{read,write} or lio_listio */
# define LIO_IO_ASYNC_TYPES	(LIO_IO_ASYNC | LIO_IO_SLISTIO | LIO_IO_ALISTIO)

/*
 * This bit provides a way to randomly pick an io type and wait method.
 * lio_read_buffer() and lio_write_buffer() functions will call
 * lio_random_methods() with the given method.
 */
#define LIO_RANDOM              010000000

/*
 * This bit provides a way for the programmer to use async i/o with
 * signals and to use their own signal handler.  By default,
 * the signal will only be given to the system call if the wait
 * method is LIO_WAIT_SIGPAUSE or LIO_WAIT_SIGACTIVE.
 * Whenever these wait methods are used, libio signal handler
 * will be used.
 */
#define LIO_USE_SIGNAL          020000000

/*
 * This function will randomly choose an io type and wait method from set of io
 * types and wait methods. Since this information is stored in a bitmask, it
 * randomly chooses an io type from the io type bits specified and does the same
 * for wait methods.
 *
 * It will return a value with all non choosen io type and wait method bits
 * cleared. The LIO_RANDOM bit is also cleared. All other bits are left
 * unchanged.
 */
int lio_random_methods(const int mask);

/*
 * This function can be used to do a write using write(2), writea(2),
 * aio_write(3), writev(2), pwrite(2) or single stride listio(2)/lio_listio(3).
 * By setting the desired bits in the method bitmask, the caller can control
 * the type of write and the wait method that will be used.
 * If no io type bits are set, write will be used.
 *
 * If async io was attempted and no wait method bits are set then the
 * wait method is: recall(2) for writea(2) and listio(2); aio_suspend(3) for
 * aio_write(3) and lio_listio(3).
 *
 * If multiple wait methods are specified, only one wait method will be used.
 * The order is predetermined.
 *
 * If the call specifies a signal and one of the two signal wait methods,
 * a signal handler for the signal is set. This will reset an already
 * set handler for this signal.
 *
 * If the LIO_RANDOM method bit is set, this function will randomly
 * choose a io type and wait method from bits in the method argument.
 */
void lio_write_buffer(
	const int fd,
	const int method,
	const char *buffer,
	const int size);

/*
 * This function can be used to do a read using read(2), reada(2),
 * aio_read(3), readv(2), pread(2), or single stride listio(2)/lio_listio(3).
 * By setting the desired bits in the method bitmask, the caller can control
 * the type of read and the wait method that will be used.
 * If no io type bits are set, read will be used.
 *
 * If async io was attempted and no wait method bits are set then the
 * wait method is: recall(2) for reada(2) and listio(2); aio_suspend(3) for
 * aio_read(3) and lio_listio(3).
 *
 * If multiple wait methods are specified, only one wait method will be used.
 * The order is predetermined.
 *
 * If the call specifies a signal and one of the two signal wait methods,
 * a signal handler for the signal is set.  This will reset an already
 * set handler for this signal.
 *
 * If the LIO_RANDOM method bit is set, this function will randomly
 * choose a io type and wait method from bits in the method argument.
 */
void lio_read_buffer(
	const int fd,
	const int method,
	char *buffer,
	const int size);

#endif /* _TLIBIO_H_ */
