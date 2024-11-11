// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that lsm_list_modules syscall is correctly recognizing LSM(s) enabled
 * inside the system.
 *
 * [Algorithm]
 *
 * - read enabled LSM(s) inside /sys/kernel/security/lsm file
 * - collect LSM IDs using lsm_list_modules syscall
 * - compare the results, verifying that LSM(s) IDs are correct
 */

#include "lsm_common.h"

#define MAX_LSM_NUM 32

static char lsm_list[BUFSIZ];
static uint32_t page_size;
static uint64_t *ids;
static uint32_t *size;

static void run(void)
{
	uint32_t lsm_num;
	uint32_t counter = 0;

	memset(ids, 0, sizeof(uint64_t) * MAX_LSM_NUM);
	*size = page_size;

	lsm_num = TST_EXP_POSITIVE(lsm_list_modules(ids, size, 0));

	for (uint32_t i = 0; i < lsm_num; i++) {
		char *name = NULL;

		switch (ids[i]) {
		case LSM_ID_CAPABILITY:
			name = "capability";
			counter++;
			break;
		case LSM_ID_SELINUX:
			name = "selinux";
			counter++;
			break;
		case LSM_ID_SMACK:
			name = "smack";
			counter++;
			break;
		case LSM_ID_TOMOYO:
			name = "tomoyo";
			counter++;
			break;
		case LSM_ID_APPARMOR:
			name = "apparmor";
			counter++;
			break;
		case LSM_ID_YAMA:
			name = "yama";
			counter++;
			break;
		case LSM_ID_LOADPIN:
			name = "loadpin";
			counter++;
			break;
		case LSM_ID_SAFESETID:
			name = "safesetid";
			counter++;
			break;
		case LSM_ID_LOCKDOWN:
			name = "lockdown";
			counter++;
			break;
		case LSM_ID_BPF:
			name = "bpf";
			counter++;
			break;
		case LSM_ID_LANDLOCK:
			name = "landlock";
			counter++;
			break;
		case LSM_ID_IMA:
			name = "ima";
			counter++;
			break;
		case LSM_ID_EVM:
			name = "evm";
			counter++;
			break;
		case LSM_ID_IPE:
			name = "ipe";
			counter++;
			break;
		default:
			break;
		}

		if (!name)
			tst_brk(TBROK, "Unsupported LSM: %lu", ids[i]);

		if (strstr(name, lsm_list))
			tst_res(TFAIL, "'%s' has not been found", name);
		else
			tst_res(TPASS, "'%s' is enabled", name);
	}

	TST_EXP_EQ_LI(*size, counter * sizeof(uint64_t));
	TST_EXP_EQ_LI(lsm_num, counter);
}

static void setup(void)
{
	int fd;

	page_size = SAFE_SYSCONF(_SC_PAGESIZE);
	fd = SAFE_OPEN("/sys/kernel/security/lsm", O_RDONLY);
	SAFE_READ(0, fd, lsm_list, BUFSIZ);
	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.bufs = (struct tst_buffers []) {
		{&ids, .size = sizeof(uint64_t) * MAX_LSM_NUM},
		{&size, .size = sizeof(uint32_t)},
		{},
	},
};
