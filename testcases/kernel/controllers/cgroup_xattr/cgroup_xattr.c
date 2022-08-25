// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013-2015 Oracle and/or its affiliates.
 *                         Alexey Kodanev <alexey.kodanev@oracle.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test checks if it is possible to set extended attributes to
 * cgroup files.
 */

#include <stdlib.h>
#include "tst_test.h"
#include "tst_safe_stdio.h"

#define MAX_SUBSYS		16
#define MAX_DIR_NAME		64
#define CGROUP_ROOT		"/sys/fs/cgroup"

static char *cgroup_names[MAX_SUBSYS];
static int cgroup_num;

static void setup(void)
{
	struct stat sb;
	FILE *file;
	char line[MAX_DIR_NAME];
	char name[MAX_DIR_NAME];
	int hier, num, enabled;

	if (stat(CGROUP_ROOT, &sb) != 0 || !S_ISDIR(sb.st_mode))
		tst_brk(TCONF, CGROUP_ROOT " is not available");

	file = SAFE_FOPEN("/proc/cgroups", "r");

	cgroup_num = 0;

	/* skip the first line */
	if (!fgets(line, MAX_DIR_NAME, file))
		tst_brk(TBROK, "/proc/cgroups is empty");

	while (fgets(line, MAX_DIR_NAME, file) != NULL) {
		if (sscanf(line, "%s\t%d\t%d\t%d", name, &hier, &num, &enabled) != 4)
			tst_brk(TBROK, "Can't parse /proc/cgroups");

		if (!enabled)
			continue;

		cgroup_names[cgroup_num] = SAFE_MALLOC(sizeof(char) * MAX_DIR_NAME);
		strncpy(cgroup_names[cgroup_num], name, MAX_DIR_NAME);
		cgroup_num++;
	}

	SAFE_FCLOSE(file);
}

static void cleanup(void)
{
	int i;
	char path[1024];
	struct stat sb;

	for (i = 0; i < cgroup_num; i++) {
		memset(path, 0, 1024);
		sprintf(path, "%s/%s/ltp", CGROUP_ROOT, cgroup_names[i]);

		if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
			SAFE_RMDIR(path);
	}
}

static void run(void)
{
	int i;
	char path[1024];
	char tasks[1024];
	char buff[3];
	ssize_t size;

	for (i = 0; i < cgroup_num; i++) {
		memset(path, 0, 1024);
		memset(tasks, 0, 1024);

		sprintf(path, "%s/%s/ltp", CGROUP_ROOT, cgroup_names[i]);
		sprintf(tasks, "%s/tasks", path);

		tst_res(TINFO, "Checking xattr support in %s/%s", CGROUP_ROOT, cgroup_names[i]);

		SAFE_MKDIR(path, 0777);

		SAFE_SETXATTR(tasks, "trusted.test", "ltp", 3, 0);
		size = SAFE_GETXATTR(tasks, "trusted.test", buff, 3);
		TST_EXP_EQ_SSZ(size, 3);
		TST_EXP_PASS(strcmp(buff, "ltp") == 0);

		SAFE_RMDIR(path);
	}
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.min_kver = "3.7",
	.needs_root = 1,
};
