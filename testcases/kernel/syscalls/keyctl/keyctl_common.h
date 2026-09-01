// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef KEYCTL_COMMON_H__
#define KEYCTL_COMMON_H__

#include <stdint.h>
#include <string.h>

#include "tst_test.h"
#include "lapi/keyctl.h"

#define KEY_PERM_ALL		(KEY_POS_ALL | KEY_USR_ALL | KEY_GRP_ALL | KEY_OTH_ALL)
#define KEY_PERM_SET		(KEY_POS_ALL | KEY_USR_ALL)

#define KEY_VIEW_BITS		(KEY_POS_VIEW | KEY_USR_VIEW | KEY_GRP_VIEW | KEY_OTH_VIEW)
#define KEY_PERM_NO_VIEW	(KEY_PERM_ALL & ~KEY_VIEW_BITS)

#define KEY_WRITE_BITS		(KEY_POS_WRITE | KEY_USR_WRITE | KEY_GRP_WRITE | KEY_OTH_WRITE)
#define KEY_PERM_NO_WRITE	(KEY_PERM_ALL & ~KEY_WRITE_BITS)

#define KEY_SETATTR_BITS	(KEY_POS_SETATTR | KEY_USR_SETATTR | KEY_GRP_SETATTR | KEY_OTH_SETATTR)
#define KEY_PERM_NO_SETATTR	(KEY_PERM_ALL & ~KEY_SETATTR_BITS)

static inline key_serial_t new_ring(const char *desc)
{
	TEST(add_key("keyring", desc, NULL, 0, KEY_SPEC_PROCESS_KEYRING));
	if (TST_RET < 0)
		tst_brk(TBROK | TTERRNO, "failed to create keyring '%s'", desc);

	return TST_RET;
}

static inline key_serial_t new_user_key(const char *desc, const void *payload,
					size_t plen, key_serial_t ring)
{
	TEST(add_key("user", desc, payload, plen, ring));
	if (TST_RET < 0)
		tst_brk(TBROK | TTERRNO, "failed to add user key '%s'", desc);

	return TST_RET;
}

static inline key_serial_t search_ring(key_serial_t ring, const char *type,
				       const char *desc)
{
	return keyctl(KEYCTL_SEARCH, ring, (unsigned long)type,
		      (unsigned long)desc, 0);
}

static inline key_serial_t add_asymmetric_key_or_tconf(const char *desc,
						       const void *payload,
						       size_t plen,
						       const char *parser_kconfig)
{
	TEST(add_key("asymmetric", desc, payload, plen, KEY_SPEC_PROCESS_KEYRING));
	if (TST_RET >= 0)
		return TST_RET;

	if (TST_ERR == ENODEV)
		tst_brk(TCONF, "kernel does not support asymmetric keys");
	if (TST_ERR == EBADMSG)
		tst_brk(TCONF, "missing asymmetric parser (%s)", parser_kconfig);
	if (TST_ERR == ENOENT)
		tst_brk(TCONF, "missing crypto RSA / SHA256 algorithms");

	tst_brk(TBROK | TTERRNO, "failed to add asymmetric key '%s'", desc);
	return -1;
}

#endif /* KEYCTL_COMMON_H__ */
