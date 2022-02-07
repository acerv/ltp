// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*
 * These functions helps you wait till a thread with given tpid changes state.
 */

#ifndef TST_THREAD_STATE__
#define TST_THREAD_STATE__

#include <unistd.h>

/*
 * Waits for thread state change.
 *
 * The state is one of the following:
 *
 * R - running
 * S - sleeping
 * D - disk sleep
 * T - stopped
 * t - tracing stopped
 * Z - zombie
 * X - dead
 */
#define TST_THREAD_STATE_WAIT(tid, state, msec_timeout) \
	tst_thread_state_wait((tid), (state), (msec_timeout))

int tst_thread_state_wait(pid_t tid, const char state, unsigned int msec_timeout);

#endif /* TST_THREAD_STATE__ */
