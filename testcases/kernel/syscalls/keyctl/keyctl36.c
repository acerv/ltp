// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test ``KEYCTL_CAPABILITIES`` buffer sizing and padding of :manpage:`keyctl(2)`.
 *
 * [Algorithm]
 *
 * - verify a 1-byte buffer receives 1 byte while the full size is returned
 *   and canary bytes beyond 1 byte remain untouched
 * - verify an oversized buffer is zero-filled beyond the capability size
 */

#include "keyctl_common.h"

#define CANARY		0xaa
#define OVERSIZE	16

static long caps_len;
static unsigned char short_buf[4];
static unsigned char large_buf[OVERSIZE];

static void setup(void)
{
	TEST(keyctl(KEYCTL_CAPABILITIES, (unsigned long)NULL, 0, 0, 0));
	if (TST_RET < 0)
		tst_brk(TBROK | TTERRNO, "KEYCTL_CAPABILITIES size query failed");

	caps_len = TST_RET;
}

static void run(void)
{
	size_t i;
	int zero_padded = 1;

	memset(short_buf, CANARY, sizeof(short_buf));
	TST_EXP_EQ_LI_SILENT(keyctl(KEYCTL_CAPABILITIES,
				    (unsigned long)short_buf, 1, 0, 0),
			     caps_len);
	if (!TST_PASS)
		return;

	if (short_buf[0] == CANARY) {
		tst_res(TFAIL, "short buffer was not populated");
		return;
	}

	for (i = 1; i < sizeof(short_buf); i++) {
		if (short_buf[i] != CANARY) {
			tst_res(TFAIL, "copy overran short buffer at offset %zu", i);
			return;
		}
	}

	tst_res(TPASS, "short buffer copied 1 byte without overrun");

	memset(large_buf, CANARY, sizeof(large_buf));
	TST_EXP_EQ_LI_SILENT(keyctl(KEYCTL_CAPABILITIES,
				    (unsigned long)large_buf, sizeof(large_buf), 0, 0),
			     caps_len);
	if (!TST_PASS)
		return;

	for (i = caps_len; i < sizeof(large_buf); i++) {
		if (large_buf[i] != 0) {
			zero_padded = 0;
			break;
		}
	}

	if (!zero_padded)
		tst_res(TFAIL, "oversized buffer was not zero-filled at offset %zu", i);
	else
		tst_res(TPASS, "oversized buffer zero-filled trailing space");
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.min_kver = "5.3",
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KEYS=y",
		NULL
	},
};
