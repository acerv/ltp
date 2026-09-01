// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Test ``KEYCTL_PKEY_ENCRYPT`` and ``KEYCTL_PKEY_DECRYPT`` of :manpage:`keyctl(2)`.
 *
 * ``KEYCTL_PKEY_ENCRYPT`` encrypts a data blob using an asymmetric public key
 * and ``KEYCTL_PKEY_DECRYPT`` decrypts the encrypted blob using the matching
 * private key.
 *
 * Requires root (CAP_SYS_MODULE) to load the ``x509_key_parser`` and
 * ``pkcs8_key_parser`` modules.
 *
 * [Algorithm]
 *
 * - encrypt a 32-byte plaintext using an RSA-2048 X.509 public key with ``enc=pkcs1``
 * - decrypt the 256-byte ciphertext using the matching PKCS#8 private key
 * - verify the decrypted output matches the original 32-byte plaintext
 */

#include "keyctl_common.h"
#include "keyctl_pkey_data.h"
#include "tst_module.h"

#define CIPHERTEXT_SIZE	256

static const char plaintext[] = "LTP_PKEY_ENCRYPT_DECRYPT_TEST_32";
#define PLAINTEXT_SIZE	(sizeof(plaintext) - 1)
static unsigned char ciphertext[CIPHERTEXT_SIZE];
static unsigned char decrypted[CIPHERTEXT_SIZE];

static key_serial_t cert_key, priv_key;
static struct keyctl_pkey_params *enc_params;
static struct keyctl_pkey_params *dec_params;

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
	memset(ciphertext, 0, sizeof(ciphertext));
	memset(decrypted, 0, sizeof(decrypted));

	memset(enc_params, 0, sizeof(*enc_params));
	enc_params->key_id = cert_key;
	enc_params->in_len = PLAINTEXT_SIZE;
	enc_params->out_len = CIPHERTEXT_SIZE;

	TST_EXP_EQ_LI_SILENT(keyctl(KEYCTL_PKEY_ENCRYPT, (unsigned long)enc_params,
				    (unsigned long)"enc=pkcs1",
				    (unsigned long)plaintext,
				    (unsigned long)ciphertext),
			     CIPHERTEXT_SIZE);
	if (!TST_PASS)
		return;

	memset(dec_params, 0, sizeof(*dec_params));
	dec_params->key_id = priv_key;
	dec_params->in_len = CIPHERTEXT_SIZE;
	dec_params->out_len = CIPHERTEXT_SIZE;

	TST_EXP_EQ_LI_SILENT(keyctl(KEYCTL_PKEY_DECRYPT, (unsigned long)dec_params,
				    (unsigned long)"enc=pkcs1",
				    (unsigned long)ciphertext,
				    (unsigned long)decrypted),
			     PLAINTEXT_SIZE);
	if (!TST_PASS)
		return;

	if (memcmp(plaintext, decrypted, PLAINTEXT_SIZE)) {
		tst_res(TFAIL, "decrypted text does not match original plaintext");
		return;
	}

	tst_res(TPASS, "KEYCTL_PKEY_ENCRYPT and KEYCTL_PKEY_DECRYPT roundtrip succeeded");
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
		NULL
	},
	.bufs = (struct tst_buffers []) {
		{&enc_params, .size = sizeof(*enc_params)},
		{&dec_params, .size = sizeof(*dec_params)},
		{},
	},
};
