// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 Oracle and/or its affiliates. All Rights Reserved.
 *	Alexey Kodanev <alexey.kodanev@oracle.com>
 * Copyright (c) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verifies that the kernel firmware loader (``CONFIG_FW_LOADER``)
 * can find and load firmware files from a custom firmware search
 * path.
 *
 * A helper kernel module (``ltp_fw_load.ko``) registers a virtual
 * device that calls :kernel_doc:`request_firmware` for a set of
 * numbered firmware blobs. Each blob is verified in-kernel against
 * its expected size and byte pattern.
 *
 * The kernel firmware loader is pointed at the writable LTP temporary
 * directory through ``/sys/module/firmware_class/parameters/path``.
 * This avoids writing into ``/lib/firmware`` and therefore works on
 * read-only or immutable root filesystems. The original value is
 * saved and restored automatically.
 *
 * [Algorithm]
 *
 * - Set the firmware search path to the LTP temporary directory
 * - Load the helper module with ``fw_size`` matching the blob size
 * - Create ``FW_NUM - 1`` firmware files there, each named
 *   ``n<i>_load_tst.fw`` and filled with a known byte pattern
 * - Add one fake firmware entry that has no file on disk
 * - Write the firmware count to ``/sys/devices/ltp_fw_load/fwnum``
 *   to trigger :kernel_doc:`request_firmware` calls in-kernel
 * - Read the result bitmask from ``/sys/devices/ltp_fw_load/result``
 * - Verify that every real firmware file was loaded successfully
 *   and that the fake entry was correctly rejected
 */

#include "tst_test.h"
#include "tst_module.h"
#include "fw_load.h"

#define MAX_NAME 64

static int module_loaded;
static int fw_count;

static struct fw_data {
	char file[PATH_MAX];
	char name[MAX_NAME];
	int fake;
} firmware[FW_NUM];

static void create_firmware(const char *dir)
{
	struct fw_data *fw = &firmware[fw_count];
	char buf[FW_SIZE];
	int fd;

	snprintf(fw->name, sizeof(fw->name), "n%d_%s", fw_count, FW_NAME);
	snprintf(fw->file, sizeof(fw->file), "%s/n%d_%s", dir, fw_count, FW_NAME);
	memset(buf, fw_count, FW_SIZE);

	fd = SAFE_OPEN(fw->file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, buf, FW_SIZE);
	SAFE_CLOSE(fd);

	fw_count++;
}

static void run(void)
{
	struct fw_data *fw;
	int result = 0;
	int fail;

	SAFE_FILE_PRINTF(DEV_FWNUM, "%d", fw_count);
	SAFE_FILE_SCANF(DEV_RESULT, "%d", &result);

	for (int i = 0; i < fw_count; i++) {
		fw = &firmware[i];

		if (fw->fake) {
			tst_res(result & (1 << i) ? TFAIL : TPASS,
				"Firmware '%s' correctly not loaded", fw->name);
		} else {
			fail = (result & (1 << i)) == 0;
			tst_res(fail ? TFAIL : TPASS,
				"Firmware '%s' loaded", fw->name);
		}
	}
}

static void setup(void)
{
	char fw_size_param[32];
	char *tmpdir = tst_tmpdir_path();
	struct tst_path_val fw_path = {
		.path = FW_PATH,
		.val = tmpdir,
		.flags = TST_SR_TCONF,
	};

	tst_requires_module_signature_disabled();

	/*
	 * Point the kernel firmware loader at our writable tmpdir so the
	 * test does not depend on a writable /lib/firmware. The previous
	 * value is restored automatically during cleanup.
	 */
	tst_sys_conf_save(&fw_path);

	snprintf(fw_size_param, sizeof(fw_size_param), "fw_size=%d", FW_SIZE);
	char *const mod_params[] = {fw_size_param, NULL};

	tst_module_load(MNAME_KO, mod_params);
	module_loaded = 1;

	for (int i = 0; i < FW_NUM - 1; i++)
		create_firmware(tmpdir);

	/* add a fake file that is never created on disk */
	snprintf(firmware[fw_count].name, sizeof(firmware[fw_count].name),
		 "n%d_%s", fw_count, FW_NAME);
	firmware[fw_count].fake = 1;
	fw_count++;
}

static void cleanup(void)
{
	if (module_loaded)
		tst_module_unload(MNAME_KO);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_FW_LOADER=y|CONFIG_FW_LOADER=m",
		NULL,
	},
};
