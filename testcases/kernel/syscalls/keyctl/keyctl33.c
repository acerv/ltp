// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Negative and boundary test cases for ``KEYCTL_PKEY_*`` of :manpage:`keyctl(2)`.
 *
 * Requires root (CAP_SYS_MODULE) to load the ``x509_key_parser`` and
 * ``pkcs8_key_parser`` modules.
 *
 * [Algorithm]
 *
 * - verify ``KEYCTL_PKEY_QUERY`` with non-zero ``arg3`` fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_QUERY`` with bogus key serial fails with ``ENOKEY``
 * - verify ``KEYCTL_PKEY_QUERY`` with non-asymmetric key fails with ``EOPNOTSUPP``
 * - verify ``KEYCTL_PKEY_QUERY`` with invalid info string fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_ENCRYPT`` with bogus key serial fails with ``ENOKEY``
 * - verify ``KEYCTL_PKEY_ENCRYPT`` with non-asymmetric key fails with ``EOPNOTSUPP``
 * - verify ``KEYCTL_PKEY_ENCRYPT`` with invalid info string fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_ENCRYPT`` with ``in_len`` exceeding limit fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_DECRYPT`` on public key certificate fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_DECRYPT`` with ``in_len`` exceeding limit fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_SIGN`` on public key certificate fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_SIGN`` with ``in_len`` exceeding limit fails with ``EINVAL``
 * - verify ``KEYCTL_PKEY_VERIFY`` with non-asymmetric key fails with ``EOPNOTSUPP``
 * - verify ``KEYCTL_PKEY_VERIFY`` with ``in2_len`` exceeding limit fails with ``EINVAL``
 */

#include "keyctl_common.h"
#include "keyctl_pkey_data.h"
#include "tst_module.h"

static key_serial_t cert_key, priv_key, user_key;
static key_serial_t bogus_key = INT32_MAX;

static struct keyctl_pkey_query *query_buf;
static struct keyctl_pkey_params *params;

static unsigned char in_buf[512];
static unsigned char out_buf[512];

static struct tcase {
	int op;
	key_serial_t *key;
	unsigned long arg3;
	const char *info;
	uint32_t in_len;
	uint32_t out_in2_len;
	int exp_errno;
	const char *desc;
} tcases[] = {
	{
		.op = KEYCTL_PKEY_QUERY,
		.key = &cert_key,
		.arg3 = 1,
		.info = "enc=pkcs1",
		.exp_errno = EINVAL,
		.desc = "PKEY_QUERY with non-zero arg3",
	},
	{
		.op = KEYCTL_PKEY_QUERY,
		.key = &bogus_key,
		.info = "enc=pkcs1",
		.exp_errno = ENOKEY,
		.desc = "PKEY_QUERY with bogus key serial",
	},
	{
		.op = KEYCTL_PKEY_QUERY,
		.key = &user_key,
		.info = "enc=pkcs1",
		.exp_errno = EOPNOTSUPP,
		.desc = "PKEY_QUERY with non-asymmetric key",
	},
	{
		.op = KEYCTL_PKEY_QUERY,
		.key = &cert_key,
		.info = "bogus_opt",
		.exp_errno = EINVAL,
		.desc = "PKEY_QUERY with invalid info string",
	},
	{
		.op = KEYCTL_PKEY_ENCRYPT,
		.key = &bogus_key,
		.info = "enc=pkcs1",
		.in_len = 32,
		.out_in2_len = 256,
		.exp_errno = ENOKEY,
		.desc = "PKEY_ENCRYPT with bogus key serial",
	},
	{
		.op = KEYCTL_PKEY_ENCRYPT,
		.key = &user_key,
		.info = "enc=pkcs1",
		.in_len = 32,
		.out_in2_len = 256,
		.exp_errno = EOPNOTSUPP,
		.desc = "PKEY_ENCRYPT with non-asymmetric key",
	},
	{
		.op = KEYCTL_PKEY_ENCRYPT,
		.key = &cert_key,
		.info = "invalid_info",
		.in_len = 32,
		.out_in2_len = 256,
		.exp_errno = EINVAL,
		.desc = "PKEY_ENCRYPT with invalid info string",
	},
	{
		.op = KEYCTL_PKEY_ENCRYPT,
		.key = &cert_key,
		.info = "enc=pkcs1",
		.in_len = 500,
		.out_in2_len = 256,
		.exp_errno = EINVAL,
		.desc = "PKEY_ENCRYPT with in_len exceeding limit",
	},
	{
		.op = KEYCTL_PKEY_DECRYPT,
		.key = &cert_key,
		.info = "enc=pkcs1",
		.in_len = 256,
		.out_in2_len = 256,
		.exp_errno = EINVAL,
		.desc = "PKEY_DECRYPT on public key certificate",
	},
	{
		.op = KEYCTL_PKEY_DECRYPT,
		.key = &priv_key,
		.info = "enc=pkcs1",
		.in_len = 500,
		.out_in2_len = 256,
		.exp_errno = EINVAL,
		.desc = "PKEY_DECRYPT with in_len exceeding limit",
	},
	{
		.op = KEYCTL_PKEY_SIGN,
		.key = &cert_key,
		.info = "enc=pkcs1 hash=sha256",
		.in_len = 32,
		.out_in2_len = 256,
		.exp_errno = EINVAL,
		.desc = "PKEY_SIGN on public key certificate",
	},
	{
		.op = KEYCTL_PKEY_SIGN,
		.key = &priv_key,
		.info = "enc=pkcs1 hash=sha256",
		.in_len = 500,
		.out_in2_len = 256,
		.exp_errno = EINVAL,
		.desc = "PKEY_SIGN with in_len exceeding limit",
	},
	{
		.op = KEYCTL_PKEY_VERIFY,
		.key = &user_key,
		.info = "enc=pkcs1 hash=sha256",
		.in_len = 32,
		.out_in2_len = 256,
		.exp_errno = EOPNOTSUPP,
		.desc = "PKEY_VERIFY with non-asymmetric key",
	},
	{
		.op = KEYCTL_PKEY_VERIFY,
		.key = &cert_key,
		.info = "enc=pkcs1 hash=sha256",
		.in_len = 32,
		.out_in2_len = 500,
		.exp_errno = EINVAL,
		.desc = "PKEY_VERIFY with in2_len exceeding limit",
	},
};

static void setup(void)
{
	SAFE_KEYCTL(KEYCTL_JOIN_SESSION_KEYRING, 0, 0, 0, 0);

	tst_modprobe("x509_key_parser", NULL);
	tst_modprobe("pkcs8_key_parser", NULL);

	user_key = new_user_key("ltp_user", "data", 4,
				KEY_SPEC_PROCESS_KEYRING);

	cert_key = add_asymmetric_key_or_tconf("cert", rsa2048_cert,
					       sizeof(rsa2048_cert),
					       "CONFIG_X509_CERTIFICATE_PARSER");
	priv_key = add_asymmetric_key_or_tconf("priv", rsa2048_pkcs8,
					       sizeof(rsa2048_pkcs8),
					       "CONFIG_PKCS8_PRIVATE_KEY_PARSER");
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	if (tc->op == KEYCTL_PKEY_QUERY) {
		TST_EXP_FAIL(keyctl(KEYCTL_PKEY_QUERY, (unsigned long)*tc->key,
				    tc->arg3, (unsigned long)tc->info,
				    (unsigned long)query_buf),
			     tc->exp_errno,
			     "%s", tc->desc);
		return;
	}

	memset(params, 0, sizeof(*params));
	params->key_id = *tc->key;
	params->in_len = tc->in_len;
	params->out_len = tc->out_in2_len;

	if (tc->op == KEYCTL_PKEY_VERIFY) {
		TST_EXP_FAIL(keyctl(tc->op, (unsigned long)params,
				    (unsigned long)tc->info,
				    (unsigned long)in_buf,
				    (unsigned long)out_buf),
			     tc->exp_errno,
			     "%s", tc->desc);
	} else {
		TST_EXP_FAIL2(keyctl(tc->op, (unsigned long)params,
				     (unsigned long)tc->info,
				     (unsigned long)in_buf,
				     (unsigned long)out_buf),
			      tc->exp_errno,
			      "%s", tc->desc);
	}
}

static struct tst_test test = {
	.setup = setup,
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.min_kver = "4.20",
	.needs_root = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_KEYS=y",
		"CONFIG_ASYMMETRIC_KEY_TYPE=y",
		"CONFIG_X509_CERTIFICATE_PARSER",
		"CONFIG_PKCS8_PRIVATE_KEY_PARSER",
		"CONFIG_CRYPTO_RSA",
		"CONFIG_CRYPTO_SHA256",
		NULL
	},
	.bufs = (struct tst_buffers []) {
		{&query_buf, .size = sizeof(*query_buf)},
		{&params, .size = sizeof(*params)},
		{},
	},
};
