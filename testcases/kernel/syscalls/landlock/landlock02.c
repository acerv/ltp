// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test verifies that landlock_add_rule syscall fails with the right
 * error codes:
 *
 * - EINVAL flags is not 0, or the rule accesses are inconsistent
 * - ENOMSG Empty accesses (i.e., rule_attr->allowed_access is 0)
 * - EBADF ruleset_fd  is  not  a  file  descriptor  for  the  current  thread,
 *   or a member of rule_attr is not a file descriptor as expected
 * - EBADFD ruleset_fd is not a ruleset file descriptor, or a member of
 *   rule_attr is not the expected file descriptor type
 * - EFAULT rule_attr was not a valid address
 */

#include "landlock_common.h"

static struct tst_landlock_ruleset_attr *ruleset_attr;
static struct landlock_path_beneath_attr *path_beneath_attr;
static struct landlock_path_beneath_attr *rule_null;
static struct landlock_net_port_attr *net_port_attr;
static int ruleset_fd;
static int invalid_fd = -1;

static struct tcase {
	int *fd;
	int rule_type;
	struct landlock_path_beneath_attr **path_attr;
	struct landlock_net_port_attr **net_attr;
	int access;
	int parent_fd;
	int net_port;
	uint32_t flags;
	int exp_errno;
	char *msg;
} tcases[] = {
	{
		.fd = &ruleset_fd,
		.path_attr = &path_beneath_attr,
		.net_attr = NULL,
		.access = LANDLOCK_ACCESS_FS_EXECUTE,
		.flags = 1,
		.exp_errno = EINVAL,
		.msg = "Invalid flags"
	},
	{
		.fd = &ruleset_fd,
		.path_attr = &path_beneath_attr,
		.net_attr = NULL,
		.access = LANDLOCK_ACCESS_FS_EXECUTE,
		.exp_errno = EINVAL,
		.msg = "Invalid rule type"
	},
	{
		.fd = &ruleset_fd,
		.rule_type = LANDLOCK_RULE_PATH_BENEATH,
		.path_attr = &path_beneath_attr,
		.net_attr = NULL,
		.exp_errno = ENOMSG,
		.msg = "Empty accesses"
	},
	{
		.fd = &invalid_fd,
		.path_attr = &path_beneath_attr,
		.net_attr = NULL,
		.access = LANDLOCK_ACCESS_FS_EXECUTE,
		.exp_errno = EBADF,
		.msg = "Invalid file descriptor"
	},
	{
		.fd = &ruleset_fd,
		.rule_type = LANDLOCK_RULE_PATH_BENEATH,
		.path_attr = &path_beneath_attr,
		.net_attr = NULL,
		.access = LANDLOCK_ACCESS_FS_EXECUTE,
		.parent_fd = -1,
		.exp_errno = EBADF,
		.msg = "Invalid parent fd"
	},
	{
		.fd = &ruleset_fd,
		.rule_type = LANDLOCK_RULE_PATH_BENEATH,
		.path_attr = &rule_null,
		.net_attr = NULL,
		.exp_errno = EFAULT,
		.msg = "Invalid rule attr"
	},
	{
		.fd = &ruleset_fd,
		.rule_type = LANDLOCK_RULE_NET_PORT,
		.path_attr = NULL,
		.net_attr = &net_port_attr,
		.access = LANDLOCK_ACCESS_FS_EXECUTE,
		.net_port = 448,
		.exp_errno = EINVAL,
		.msg = "Invalid access rule for network type"
	},
	{
		.fd = &ruleset_fd,
		.rule_type = LANDLOCK_RULE_NET_PORT,
		.path_attr = NULL,
		.net_attr = &net_port_attr,
		.access = LANDLOCK_ACCESS_NET_BIND_TCP,
		.net_port = INT16_MAX + 1,
		.exp_errno = EINVAL,
		.msg = "Socket port greater than 65535"
	},
};

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	if (tc->path_attr) {
		if (*tc->path_attr) {
			(*tc->path_attr)->allowed_access = tc->access;
			(*tc->path_attr)->parent_fd = tc->parent_fd;
		}

		TST_EXP_FAIL(tst_syscall(__NR_landlock_add_rule,
			*tc->fd, tc->rule_type, *tc->path_attr, tc->flags),
			tc->exp_errno, "%s", tc->msg);
	} else if (tc->net_attr) {
		if (tc->net_attr) {
			(*tc->net_attr)->allowed_access = tc->access;
			(*tc->net_attr)->port = tc->net_port;
		}

		TST_EXP_FAIL(tst_syscall(__NR_landlock_add_rule,
			*tc->fd, tc->rule_type, *tc->net_attr, tc->flags),
			tc->exp_errno, "%s", tc->msg);
	}
}

static void setup(void)
{
	verify_landlock_is_enabled();

	ruleset_attr->base.handled_access_fs = LANDLOCK_ACCESS_FS_EXECUTE;

	ruleset_fd = TST_EXP_FD_SILENT(tst_syscall(__NR_landlock_create_ruleset,
		&ruleset_attr->base, sizeof(struct tst_landlock_ruleset_attr), 0));
}

static void cleanup(void)
{
	if (ruleset_fd != -1)
		SAFE_CLOSE(ruleset_fd);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.bufs = (struct tst_buffers []) {
		{&ruleset_attr, .size = sizeof(struct tst_landlock_ruleset_attr)},
		{&path_beneath_attr, .size = sizeof(struct landlock_path_beneath_attr)},
		{&net_port_attr, .size = sizeof(struct landlock_net_port_attr)},
		{},
	},
	.caps = (struct tst_cap []) {
		TST_CAP(TST_CAP_REQ, CAP_SYS_ADMIN),
		{}
	},
};
