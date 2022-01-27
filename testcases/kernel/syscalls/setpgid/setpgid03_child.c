// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 Oracle and/or its affiliates. All Rights Reserved.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#include "tst_test.h"

static void run(void)
{
	TST_CHECKPOINT_WAKE_AND_WAIT(0);
}

static struct tst_test test = {
	.test_all = run,
	.needs_checkpoints = 1,
};
