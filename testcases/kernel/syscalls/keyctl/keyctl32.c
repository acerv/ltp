// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test ``KEYCTL_PKEY_SIGN`` and ``KEYCTL_PKEY_VERIFY`` of :manpage:`keyctl(2)`.
 *
 * ``KEYCTL_PKEY_SIGN`` signs a digest using an asymmetric private key and
 * ``KEYCTL_PKEY_VERIFY`` verifies the signature using the matching public key.
 *
 * Requires root (CAP_SYS_MODULE) to load the ``x509_key_parser`` and
 * ``pkcs8_key_parser`` modules.
 *
 * [Algorithm]
 *
 * - sign a 32-byte digest using an RSA-2048 PKCS#8 private key with
 *   ``enc=pkcs1 hash=sha256``
 * - verify the 256-byte signature using the matching X.509 public key
 * - verify a mismatched digest fails verification with ``EKEYREJECTED``
 */

#include "keyctl_common.h"
#include "keyctl_pkey_data.h"
#include "tst_module.h"

#define DIGEST_SIZE	32
#define SIG_SIZE	256

static const unsigned char digest[DIGEST_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

static unsigned char sig[SIG_SIZE];
static unsigned char wrong_digest[DIGEST_SIZE];

static key_serial_t cert_key, priv_key;
static struct keyctl_pkey_params *sign_params;
static struct keyctl_pkey_params *verify_params;

static void setup(void)
{
	SAFE_KEYCTL(KEYCTL_JOIN_SESSION_KEYRING, 0, 0, 0, 0);

	tst_modprobe("x509_key_parser", NULL);
	tst_modprobe("pkcs8_key_parser", NULL);

	cert_key = add_asymmetric_key_or_tconf("cert", rsa2048_cert,
					       sizeof(rsa2048_cert),
					       "CONFIG_X509_CERTIFICATE_PARSER");
	priv_key = add_asymmetric_key_or_tconf("priv", rsa2048_pkcs8,
					       sizeof(rsa2048_pkcs8),
					       "CONFIG_PKCS8_PRIVATE_KEY_PARSER");
}

static void run(void)
{
	memset(sig, 0, sizeof(sig));

	memset(sign_params, 0, sizeof(*sign_params));
	sign_params->key_id = priv_key;
	sign_params->in_len = DIGEST_SIZE;
	sign_params->out_len = SIG_SIZE;

	TST_EXP_EQ_LI_SILENT(keyctl(KEYCTL_PKEY_SIGN, (unsigned long)sign_params,
				    (unsigned long)"enc=pkcs1 hash=sha256",
				    (unsigned long)digest,
				    (unsigned long)sig),
			     SIG_SIZE);
	if (!TST_PASS)
		return;

	memset(verify_params, 0, sizeof(*verify_params));
	verify_params->key_id = cert_key;
	verify_params->in_len = DIGEST_SIZE;
	verify_params->in2_len = SIG_SIZE;

	TST_EXP_EQ_LI_SILENT(keyctl(KEYCTL_PKEY_VERIFY, (unsigned long)verify_params,
				    (unsigned long)"enc=pkcs1 hash=sha256",
				    (unsigned long)digest,
				    (unsigned long)sig),
			     0);
	if (!TST_PASS)
		return;

	tst_res(TPASS, "KEYCTL_PKEY_SIGN and KEYCTL_PKEY_VERIFY roundtrip succeeded");

	memcpy(wrong_digest, digest, sizeof(wrong_digest));
	wrong_digest[0] ^= 0xff;

	TST_EXP_FAIL(keyctl(KEYCTL_PKEY_VERIFY, (unsigned long)verify_params,
			    (unsigned long)"enc=pkcs1 hash=sha256",
			    (unsigned long)wrong_digest,
			    (unsigned long)sig),
		     EKEYREJECTED,
		     "KEYCTL_PKEY_VERIFY with mismatched digest");
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
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
		{&sign_params, .size = sizeof(*sign_params)},
		{&verify_params, .size = sizeof(*verify_params)},
		{},
	},
};
