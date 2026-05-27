// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Fujitsu Ltd.
 * Author: Guangwen Feng <fenggw-fnst@cn.fujitsu.com>
 */

/*\
 * Verify that :manpage:`umount2(2)` with MNT_DETACH performs a lazy
 * unmount: the mount point becomes unavailable for new accesses but
 * existing open file descriptors remain usable. Data written via the
 * old fd persists after remounting.
 */

#include "tst_test.h"
#include "lapi/mount.h"

#define MNTPOINT	"mntpoint"

static int fd = -1;
static int mounted;

static void run(void)
{
	int ret;
	char buf[256];
	const char *str = "abcdefghijklmnopqrstuvwxyz";

	SAFE_MOUNT(tst_device->dev, MNTPOINT, tst_device->fs_type, 0, NULL);
	mounted = 1;

	fd = SAFE_CREAT(MNTPOINT "/file", 0644);

	TST_EXP_PASS(umount2(MNTPOINT, MNT_DETACH));
	if (!TST_PASS)
		goto out;

	mounted = 0;

	ret = access(MNTPOINT "/file", F_OK);
	if (ret != -1) {
		tst_res(TFAIL, "file still accessible after lazy unmount");
		goto out;
	}

	SAFE_WRITE(SAFE_WRITE_ALL, fd, str, strlen(str));
	SAFE_CLOSE(fd);

	SAFE_MOUNT(tst_device->dev, MNTPOINT, tst_device->fs_type, 0, NULL);
	mounted = 1;

	fd = SAFE_OPEN(MNTPOINT "/file", O_RDONLY);

	memset(buf, 0, sizeof(buf));
	SAFE_READ(1, fd, buf, strlen(str));

	if (strcmp(str, buf))
		tst_res(TFAIL, "data mismatch after remount");
	else
		tst_res(TPASS, "umount2 MNT_DETACH works correctly");

out:
	if (fd != -1)
		SAFE_CLOSE(fd);

	if (mounted && !tst_umount(MNTPOINT))
		mounted = 0;
}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);

	if (mounted)
		tst_umount(MNTPOINT);
}

static struct tst_test test = {
	.test_all = run,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.format_device = 1,
	.mntpoint = MNTPOINT,
	.needs_device = 1,
};
