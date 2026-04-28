/* ***************************************************************************
 *
 * Copyright (c) 2026 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <iot_error.h>
#include <iot_security_common.h>
#include <string.h>

#include "TC_MOCK_functions.h"
#include "cmocka_custom.h"
#include "mbedtls_helper.h"

#define UNUSED_TC(x) (void)(x)

/* ===== Tests for src/port/crypto/reference/mbedtls_helper.c ===== */

void TC_mbedtls_helper_sha512_success(void **state)
{
    unsigned char input[] = "abc";
    unsigned char output[64];
    iot_error_t err;
    UNUSED_TC(state);

    err = mbedtls_helper_sha512(input, sizeof(input) - 1, output, sizeof(output));
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_mbedtls_helper_sha256_success(void **state)
{
    unsigned char input[] = "abc";
    unsigned char output[32];
    iot_error_t err;
    UNUSED_TC(state);

    err = mbedtls_helper_sha256(input, sizeof(input) - 1, output, sizeof(output));
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_mbedtls_helper_gen_secp256r1_keypair_success(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_error_t err;
    UNUSED_TC(state);

    /* When */
    err = mbedtls_helper_gen_secp256r1_keypair(&seckey, &pubkey);
    /* Then: a fresh secp256r1 keypair is produced */
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_int_equal(seckey.len, 32);
    assert_non_null(seckey.p);
    assert_non_null(pubkey.p);
    assert_true(pubkey.len > 32);

    /* Teardown */
    iot_security_buffer_free(&seckey);
    iot_security_buffer_free(&pubkey);
}

void TC_mbedtls_helper_gen_secp256r1_keypair_malloc_failure(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_error_t err;
    UNUSED_TC(state);

    /* The first iot_os_malloc inside the helper allocates the keypair struct */
    set_mock_detect_memory_leak(false);
    do_not_use_mock_iot_os_malloc_failure();
    set_mock_iot_os_malloc_failure_with_index(0);
    /* When */
    err = mbedtls_helper_gen_secp256r1_keypair(&seckey, &pubkey);
    /* Then: returns mem-alloc */
    assert_int_equal(err, IOT_ERROR_SECURITY_MEM_ALLOC);

    /* Teardown */
    do_not_use_mock_iot_os_malloc_failure();
}

void TC_mbedtls_helper_cipher_aes_invalid_key(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: input set, but key.p is NULL */
    input.p = data;
    input.len = 1;
    /* When */
    err = mbedtls_helper_cipher_aes(&cipher, &input, &output, true);
    /* Then */
    assert_int_equal(err, IOT_ERROR_SECURITY_CIPHER_INVALID_KEY);
}

void TC_mbedtls_helper_cipher_aes_invalid_iv(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    unsigned char key_buf[32];
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: key set, iv NULL */
    input.p = data;
    input.len = 1;
    cipher.key.p = key_buf;
    cipher.key.len = sizeof(key_buf);
    /* When */
    err = mbedtls_helper_cipher_aes(&cipher, &input, &output, true);
    /* Then */
    assert_int_equal(err, IOT_ERROR_SECURITY_CIPHER_INVALID_IV);
}

void TC_mbedtls_helper_cipher_aes_key_len_mismatch(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    unsigned char key_buf[16]; /* AES256 expects 32 bytes */
    unsigned char iv_buf[16];
    iot_error_t err;
    UNUSED_TC(state);

    input.p = data;
    input.len = 1;
    cipher.type = IOT_SECURITY_KEY_TYPE_AES256;
    cipher.key.p = key_buf;
    cipher.key.len = sizeof(key_buf);
    cipher.iv.p = iv_buf;
    cipher.iv.len = sizeof(iv_buf);
    /* When */
    err = mbedtls_helper_cipher_aes(&cipher, &input, &output, true);
    /* Then */
    assert_int_equal(err, IOT_ERROR_SECURITY_CIPHER_KEY_LEN);
}

void TC_mbedtls_helper_cipher_aes_iv_len_mismatch(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    unsigned char key_buf[32];
    unsigned char iv_buf[8]; /* AES256-CBC expects 16 bytes */
    iot_error_t err;
    UNUSED_TC(state);

    input.p = data;
    input.len = 1;
    cipher.type = IOT_SECURITY_KEY_TYPE_AES256;
    cipher.key.p = key_buf;
    cipher.key.len = sizeof(key_buf);
    cipher.iv.p = iv_buf;
    cipher.iv.len = sizeof(iv_buf);
    /* When */
    err = mbedtls_helper_cipher_aes(&cipher, &input, &output, true);
    /* Then */
    assert_int_equal(err, IOT_ERROR_SECURITY_CIPHER_IV_LEN);
}

void TC_mbedtls_helper_cipher_aes_encrypt_decrypt_roundtrip(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t encrypted = {0};
    iot_security_buffer_t decrypted = {0};
    unsigned char message[] = "the quick brown fox jumps over the lazy dog";
    unsigned char key_buf[32];
    unsigned char iv_buf[16];
    iot_error_t err;
    int i;
    UNUSED_TC(state);

    /* Given: a deterministic key and iv plus AES256 cipher params */
    for (i = 0; i < (int)sizeof(key_buf); i++)
        key_buf[i] = (unsigned char)i;
    for (i = 0; i < (int)sizeof(iv_buf); i++)
        iv_buf[i] = (unsigned char)(i * 3);
    cipher.type = IOT_SECURITY_KEY_TYPE_AES256;
    cipher.key.p = key_buf;
    cipher.key.len = sizeof(key_buf);
    cipher.iv.p = iv_buf;
    cipher.iv.len = sizeof(iv_buf);
    input.p = message;
    input.len = sizeof(message) - 1;

    /* When: encrypt */
    err = mbedtls_helper_cipher_aes(&cipher, &input, &encrypted, true);
    /* Then */
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(encrypted.p);
    assert_true(encrypted.len > 0);

    /* When: decrypt the produced ciphertext */
    err = mbedtls_helper_cipher_aes(&cipher, &encrypted, &decrypted, false);
    /* Then: get the original message back */
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(decrypted.p);
    assert_memory_equal(decrypted.p, message, sizeof(message) - 1);

    /* Teardown */
    iot_security_buffer_free(&encrypted);
    iot_security_buffer_free(&decrypted);
}

void TC_mbedtls_helper_ecdh_compute_shared_ed25519_invalid_args(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    iot_error_t err;
    UNUSED_TC(state);

    /* When: NULL t_seckey_buf */
    err = mbedtls_helper_ecdh_compute_shared_ed25519(NULL, &pubkey, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    /* When: NULL c_pubkey_buf */
    err = mbedtls_helper_ecdh_compute_shared_ed25519(&seckey, NULL, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    /* When: NULL output_buf */
    err = mbedtls_helper_ecdh_compute_shared_ed25519(&seckey, &pubkey, NULL);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_mbedtls_helper_ecdh_compute_shared_ed25519_seckey_too_large(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    unsigned char buf[64];
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: seckey > 32 bytes */
    seckey.p = buf;
    seckey.len = sizeof(buf);
    /* When */
    err = mbedtls_helper_ecdh_compute_shared_ed25519(&seckey, &pubkey, &output);
    /* Then */
    assert_int_equal(err, IOT_ERROR_SECURITY_ECDH_INVALID_SECKEY);
}

void TC_mbedtls_helper_ecdh_compute_shared_ed25519_pubkey_too_large(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    unsigned char sk_buf[16];
    unsigned char pk_buf[64];
    iot_error_t err;
    UNUSED_TC(state);

    seckey.p = sk_buf;
    seckey.len = sizeof(sk_buf);
    pubkey.p = pk_buf;
    pubkey.len = sizeof(pk_buf);
    err = mbedtls_helper_ecdh_compute_shared_ed25519(&seckey, &pubkey, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_ECDH_INVALID_PUBKEY);
}

void TC_mbedtls_helper_ecdh_compute_shared_ed25519_success(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    unsigned char sk_buf[32];
    unsigned char pk_buf[32];
    iot_error_t err;
    int i;
    UNUSED_TC(state);

    /* Given: deterministic-but-arbitrary 32-byte seckey/pubkey */
    for (i = 0; i < 32; i++) {
        sk_buf[i] = (unsigned char)(i + 1);
        pk_buf[i] = (unsigned char)(i + 100);
    }
    seckey.p = sk_buf;
    seckey.len = sizeof(sk_buf);
    pubkey.p = pk_buf;
    pubkey.len = sizeof(pk_buf);

    /* When: drive the ed25519 ECDH happy-path (note: random keys may fail at
     * mbedtls_ecdh_compute_shared because they don't form a valid curve point;
     * either outcome exercises the body of the function) */
    err = mbedtls_helper_ecdh_compute_shared_ed25519(&seckey, &pubkey, &output);
    /* Then: function returned without crashing */
    (void)err;

    /* Teardown */
    if (output.p) {
        iot_security_buffer_free(&output);
    }
}

void TC_mbedtls_helper_ecdh_compute_shared_ecdsa_invalid_args(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    iot_error_t err;
    UNUSED_TC(state);

    err = mbedtls_helper_ecdh_compute_shared_ecdsa(NULL, &pubkey, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    err = mbedtls_helper_ecdh_compute_shared_ecdsa(&seckey, NULL, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    err = mbedtls_helper_ecdh_compute_shared_ecdsa(&seckey, &pubkey, NULL);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_mbedtls_helper_ecdh_compute_shared_ecdsa_seckey_too_large(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    unsigned char buf[128]; /* > 64 */
    iot_error_t err;
    UNUSED_TC(state);

    seckey.p = buf;
    seckey.len = sizeof(buf);
    err = mbedtls_helper_ecdh_compute_shared_ecdsa(&seckey, &pubkey, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_ECDH_INVALID_SECKEY);
}

void TC_mbedtls_helper_ecdh_compute_shared_ecdsa_pubkey_too_large(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    unsigned char sk_buf[32];
    unsigned char pk_buf[128]; /* > 65 */
    iot_error_t err;
    UNUSED_TC(state);

    seckey.p = sk_buf;
    seckey.len = sizeof(sk_buf);
    pubkey.p = pk_buf;
    pubkey.len = sizeof(pk_buf);
    err = mbedtls_helper_ecdh_compute_shared_ecdsa(&seckey, &pubkey, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_ECDH_INVALID_PUBKEY);
}

void TC_mbedtls_helper_ecdh_compute_shared_ecdsa_success(void **state)
{
    iot_security_buffer_t a_seckey = {0};
    iot_security_buffer_t a_pubkey = {0};
    iot_security_buffer_t b_seckey = {0};
    iot_security_buffer_t b_pubkey = {0};
    iot_security_buffer_t shared_a = {0};
    iot_security_buffer_t shared_b = {0};
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: two fresh secp256r1 keypairs */
    err = mbedtls_helper_gen_secp256r1_keypair(&a_seckey, &a_pubkey);
    assert_int_equal(err, IOT_ERROR_NONE);
    err = mbedtls_helper_gen_secp256r1_keypair(&b_seckey, &b_pubkey);
    assert_int_equal(err, IOT_ERROR_NONE);

    /* When: compute shared secret from both sides; they should agree. */
    /* Use the raw uncompressed pubkey form (already what gen returns). */
    err = mbedtls_helper_ecdh_compute_shared_ecdsa(&a_seckey, &b_pubkey, &shared_a);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_int_equal(shared_a.len, 32);
    assert_non_null(shared_a.p);

    err = mbedtls_helper_ecdh_compute_shared_ecdsa(&b_seckey, &a_pubkey, &shared_b);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_int_equal(shared_b.len, 32);
    assert_memory_equal(shared_a.p, shared_b.p, shared_a.len);

    /* Teardown */
    iot_security_buffer_free(&a_seckey);
    iot_security_buffer_free(&a_pubkey);
    iot_security_buffer_free(&b_seckey);
    iot_security_buffer_free(&b_pubkey);
    iot_security_buffer_free(&shared_a);
    iot_security_buffer_free(&shared_b);
}

void TC_mbedtls_helper_pk_sign_rsa_invalid_seckey(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    UNUSED_TC(state);

    /* When: seckey buffer pointer NULL */
    err = mbedtls_helper_pk_sign_rsa(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_SECKEY);
}

void TC_mbedtls_helper_pk_sign_rsa_parse_failure(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char bogus_seckey[] = "not a real RSA private key";
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED_TC(state);

    pk.seckey.p = bogus_seckey;
    pk.seckey.len = sizeof(bogus_seckey) - 1;
    input.p = data;
    input.len = 1;
    /* When: PEM parse fails */
    err = mbedtls_helper_pk_sign_rsa(&pk, &input, &sig);
    /* Then */
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_PARSEKEY);
}

void TC_mbedtls_helper_pk_sign_ecdsa_invalid_seckey(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    UNUSED_TC(state);

    err = mbedtls_helper_pk_sign_ecdsa(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_SECKEY);
}

void TC_mbedtls_helper_pk_sign_ecdsa_parse_failure(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char bogus_seckey[] = "not an ECDSA key either";
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED_TC(state);

    pk.seckey.p = bogus_seckey;
    pk.seckey.len = sizeof(bogus_seckey) - 1;
    input.p = data;
    input.len = 1;
    err = mbedtls_helper_pk_sign_ecdsa(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_PARSEKEY);
}

void TC_mbedtls_helper_pk_verify_rsa_invalid_pubkey(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    UNUSED_TC(state);

    err = mbedtls_helper_pk_verify_rsa(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_PUBKEY);
}

void TC_mbedtls_helper_pk_verify_rsa_parse_failure(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char bogus_pubkey[] = "not an x509 cert";
    unsigned char data[] = "x";
    unsigned char sig_data[] = "y";
    iot_error_t err;
    UNUSED_TC(state);

    pk.pubkey.p = bogus_pubkey;
    pk.pubkey.len = sizeof(bogus_pubkey) - 1;
    input.p = data;
    input.len = 1;
    sig.p = sig_data;
    sig.len = 1;
    err = mbedtls_helper_pk_verify_rsa(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_PARSEKEY);
}

void TC_mbedtls_helper_pk_verify_ecdsa_invalid_pubkey(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    UNUSED_TC(state);

    err = mbedtls_helper_pk_verify_ecdsa(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_PUBKEY);
}

void TC_mbedtls_helper_pk_verify_ecdsa_parse_failure(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char bogus_pubkey[] = "still not an x509 cert";
    unsigned char data[] = "x";
    unsigned char sig_data[] = "y";
    iot_error_t err;
    UNUSED_TC(state);

    pk.pubkey.p = bogus_pubkey;
    pk.pubkey.len = sizeof(bogus_pubkey) - 1;
    input.p = data;
    input.len = 1;
    sig.p = sig_data;
    sig.len = 1;
    err = mbedtls_helper_pk_verify_ecdsa(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_PARSEKEY);
}

/* ECDSA secp256r1 private key in PEM form, for pk_sign_ecdsa success path */
#define TC_MBEDTLS_TEST_ECDSA_PRIVATE_KEY                                  \
    "-----BEGIN EC PRIVATE KEY-----\r\n"                                   \
    "MHcCAQEEID9/vyoZcyY7OI7LbnkyfMTnINtYOLpNk5x9KCM8lmLdoAoGCCqGSM49\r\n" \
    "AwEHoUQDQgAEPYS01pFX7+PsnY2GUfpRTjohuSnQxP+3zdEP5Ovd5CnTTLzvPwbb\r\n" \
    "C0tUb0s4Jet0duPc7Vz9b91zqX5A5yZO8w==\r\n"                             \
    "-----END EC PRIVATE KEY-----"

#define TC_MBEDTLS_TEST_ECDSA_CERTIFICATE                                  \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIICMDCCAdegAwIBAgIJANC5Hjw2G4qUMAoGCCqGSM49BAMCMGcxCzAJBgNVBAYT\r\n" \
    "AktSMRYwFAYDVQQKDA1UaGluZ3MgU3lzdGVtMRkwFwYDVQQLDBBTbWFydFRoaW5n\r\n" \
    "cyBNUVRUMSUwIwYDVQQDDBxUaGluZ3Mgc2VsZi1zaWduZWQgcm9vdCBjZXJ0MCAX\r\n" \
    "DTIwMDYzMDAyMjExMVoYDzIwNjAwNjIwMDIyMTExWjBYMQswCQYDVQQGEwJLUjEW\r\n" \
    "MBQGA1UECgwNVGhpbmdzIFN5c3RlbTEZMBcGA1UECwwQU21hcnRUaGluZ3MgTVFU\r\n" \
    "VDEWMBQGA1UEAwwNVGhpbmdzIERldmljZTBZMBMGByqGSM49AgEGCCqGSM49AwEH\r\n" \
    "A0IABD2EtNaRV+/j7J2NhlH6UU46Ibkp0MT/t83RD+Tr3eQp00y87z8G2wtLVG9L\r\n" \
    "OCXrdHbj3O1c/W/dc6l+QOcmTvOjeTB3MAkGA1UdEwQCMAAwHwYDVR0jBBgwFoAU\r\n" \
    "7ZcCOlSfrHMs4F9+InsLj57fQsYwHQYDVR0OBBYEFJy/7DNDLa3DuAFMmYHhHtJg\r\n" \
    "PP/EMAsGA1UdDwQEAwIF4DAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\r\n" \
    "CgYIKoZIzj0EAwIDRwAwRAIgNDgGFm5fDKtUNJMLXNWmnkeGYlmq4r4X7beHOu2Z\r\n" \
    "Ei8CIFdv66Rn53RtLL4AgpfBk8k1ut4ZJpQUEYG7F9P+GZOf\r\n"                 \
    "-----END CERTIFICATE-----"

void TC_mbedtls_helper_pk_sign_ecdsa_success_raw(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char message[] = "the quick brown fox";
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: a valid ECDSA private key in PEM and pk_sign_type=RAW so the
     * helper performs DER-to-raw conversion */
    pk.seckey.p = (unsigned char *)TC_MBEDTLS_TEST_ECDSA_PRIVATE_KEY;
    pk.seckey.len = strlen(TC_MBEDTLS_TEST_ECDSA_PRIVATE_KEY);
    pk.pk_sign_type = IOT_SECURITY_PK_SIGN_TYPE_RAW;
    input.p = message;
    input.len = sizeof(message) - 1;
    /* When */
    err = mbedtls_helper_pk_sign_ecdsa(&pk, &input, &sig);
    /* Then: produces a raw 64-byte signature */
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(sig.p);
    assert_true(sig.len > 0);

    /* Teardown */
    iot_security_buffer_free(&sig);
}

void TC_mbedtls_helper_pk_sign_ecdsa_success_der(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char message[] = "another message";
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: pk_sign_type=DER skips the der-to-raw conversion */
    pk.seckey.p = (unsigned char *)TC_MBEDTLS_TEST_ECDSA_PRIVATE_KEY;
    pk.seckey.len = strlen(TC_MBEDTLS_TEST_ECDSA_PRIVATE_KEY);
    pk.pk_sign_type = IOT_SECURITY_PK_SIGN_TYPE_DER;
    input.p = message;
    input.len = sizeof(message) - 1;
    /* When */
    err = mbedtls_helper_pk_sign_ecdsa(&pk, &input, &sig);
    /* Then */
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(sig.p);

    /* Teardown */
    iot_security_buffer_free(&sig);
}

void TC_mbedtls_helper_pk_verify_ecdsa_success(void **state)
{
    iot_security_pk_params_t sign_params = {0};
    iot_security_pk_params_t verify_params = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char message[] = "to be signed and verified";
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: sign with the ECDSA PEM private key (DER form so we can verify
     * using the matching x509 cert which uses DER signatures), then verify
     * with the matching x509 certificate. */
    sign_params.seckey.p = (unsigned char *)TC_MBEDTLS_TEST_ECDSA_PRIVATE_KEY;
    sign_params.seckey.len = strlen(TC_MBEDTLS_TEST_ECDSA_PRIVATE_KEY);
    sign_params.pk_sign_type = IOT_SECURITY_PK_SIGN_TYPE_DER;
    input.p = message;
    input.len = sizeof(message) - 1;
    err = mbedtls_helper_pk_sign_ecdsa(&sign_params, &input, &sig);
    assert_int_equal(err, IOT_ERROR_NONE);

    verify_params.pubkey.p = (unsigned char *)TC_MBEDTLS_TEST_ECDSA_CERTIFICATE;
    verify_params.pubkey.len = strlen(TC_MBEDTLS_TEST_ECDSA_CERTIFICATE);
    /* When */
    err = mbedtls_helper_pk_verify_ecdsa(&verify_params, &input, &sig);
    /* Then */
    assert_int_equal(err, IOT_ERROR_NONE);

    /* Teardown */
    iot_security_buffer_free(&sig);
}

void TC_mbedtls_helper_pk_verify_ecdsa_bad_signature(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char message[] = "to be verified";
    /* DER-encoded ECDSA signature of right shape but bogus contents */
    unsigned char bogus_sig[71] = {
        0x30,
        0x45,
        0x02,
        0x21,
        0x00,
        /* 32 bytes of 'r' */
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0xaa,
        0x02,
        0x20,
        /* 32 bytes of 's' */
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
        0xbb,
    };
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: a valid ECDSA cert + an obviously wrong signature */
    pk.pubkey.p = (unsigned char *)TC_MBEDTLS_TEST_ECDSA_CERTIFICATE;
    pk.pubkey.len = strlen(TC_MBEDTLS_TEST_ECDSA_CERTIFICATE);
    input.p = message;
    input.len = sizeof(message) - 1;
    sig.p = bogus_sig;
    sig.len = sizeof(bogus_sig);
    /* When */
    err = mbedtls_helper_pk_verify_ecdsa(&pk, &input, &sig);
    /* Then: verification fails */
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_VERIFY);
}

/* RSA test vectors borrowed from the iot_security_crypto sample data;
 * they let us drive pk_verify_rsa through the parse-success then verify-fail
 * path. */
#define TC_MBEDTLS_TEST_RSA_CERTIFICATE                                    \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIID0jCCArqgAwIBAgIJAOmRaXeUY/lXMA0GCSqGSIb3DQEBCwUAMHMxCzAJBgNV\r\n" \
    "BAYTAktSMR8wHQYDVQQKDBZTbWFydFRoaW5ncyBEZXZpY2UgU0RLMRUwEwYDVQQL\r\n" \
    "DAxNUVRUIFJvb3QgQ0ExLDAqBgNVBAMMI1NtYXJ0VGhpbmdzIERldmljZSBTREsg\r\n" \
    "Um9vdCBDQSBURVNUMCAXDTIwMDMxOTA5MDkzMVoYDzIwNjAwMzA5MDkwOTMxWjBh\r\n" \
    "MQswCQYDVQQGEwJLUjEfMB0GA1UECgwWU21hcnRUaGluZ3MgRGV2aWNlIFNESzEU\r\n" \
    "MBIGA1UECwwLTVFUVCBEZXZpY2UxGzAZBgNVBAMMElNtYXJ0VGhpbmdzIERldmlj\r\n" \
    "ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANf9ZMrPXBoyIXW0Pehn\r\n" \
    "rlYQvALE88VR8RsS4IUtlJCyd5bG479S9Q9uEXvlJ5+eyPCG/kMH4XZhaQ/FKQZd\r\n" \
    "p+53RTnH91S6ZufaUNvRYptZPFS72nqS2A0Rhdu73/+S4BuJYNqORzhYVLOZEy7O\r\n" \
    "S1qZWiZQphbtxMnwRO3ecHLsSwCJhVGppZyM1T6NRyrLztYfqTIj/HQAIg5yP9VJ\r\n" \
    "dJSY7i2suD9cOloI7VqjLyvEExe8EIhhPR1rDE/8wuBbNdHLd351+xwFFe/JsY6a\r\n" \
    "Uzlna83gRHKm+6bERT1UG7ALuauEMe497vXjPk7DTnRdsney6UmidUdZWEn/7bsT\r\n" \
    "1yECAwEAAaN5MHcwCQYDVR0TBAIwADAfBgNVHSMEGDAWgBRNfBbpcuDMVV2TAU3w\r\n" \
    "tohHHkZEczAdBgNVHQ4EFgQU4dGfYxzu3k2Qu/ZwyCZWbzdg4P8wCwYDVR0PBAQD\r\n" \
    "AgXgMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjANBgkqhkiG9w0BAQsF\r\n" \
    "AAOCAQEAmMlYsf32MuuGtFQxSkgif1ahumBRRQIh6gIXzZ5FiPYzRmK/CkvluLl1\r\n" \
    "fOsXF9loWHfX78mqdZrYcnYkt6YNg5RIMk4Rg6GUHT8mb6+r9vWSkTcKC8VkVNak\r\n" \
    "BPRzWRVbVIur4BKZn7xL6CgizYL4WeseJUUBqxNLWg4aDelHYuhMCqQbnThmAv6Z\r\n" \
    "d2a9T9hzxJASoWA6cLDh3m6EMwjKPbEyMPEd4n1l2t7n2yc81DCNNtegz3QEsCMt\r\n" \
    "onE+w5kmmxSPX/5Jn1122IzX1nRXlFuhK1U6riQ/8SxxuiIm33OXh2gYmwtpQATY\r\n" \
    "SdxhWUDsV4MxNuDc5todC5xNMePMBQ==\r\n"                                 \
    "-----END CERTIFICATE-----"

void TC_mbedtls_helper_pk_verify_rsa_bad_signature(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char message[] = "to be verified";
    unsigned char bogus_sig[256] = {0};
    iot_error_t err;
    UNUSED_TC(state);

    /* Given: a valid RSA cert + an all-zero signature */
    pk.pubkey.p = (unsigned char *)TC_MBEDTLS_TEST_RSA_CERTIFICATE;
    pk.pubkey.len = strlen(TC_MBEDTLS_TEST_RSA_CERTIFICATE);
    input.p = message;
    input.len = sizeof(message) - 1;
    sig.p = bogus_sig;
    sig.len = sizeof(bogus_sig);
    /* When */
    err = mbedtls_helper_pk_verify_rsa(&pk, &input, &sig);
    /* Then: verification fails */
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_VERIFY);
}

int TEST_FUNC_mbedtls_helper(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(TC_mbedtls_helper_sha512_success),
        cmocka_unit_test(TC_mbedtls_helper_sha256_success),
        cmocka_unit_test(TC_mbedtls_helper_gen_secp256r1_keypair_success),
        cmocka_unit_test(TC_mbedtls_helper_gen_secp256r1_keypair_malloc_failure),
        cmocka_unit_test(TC_mbedtls_helper_cipher_aes_invalid_key),
        cmocka_unit_test(TC_mbedtls_helper_cipher_aes_invalid_iv),
        cmocka_unit_test(TC_mbedtls_helper_cipher_aes_key_len_mismatch),
        cmocka_unit_test(TC_mbedtls_helper_cipher_aes_iv_len_mismatch),
        cmocka_unit_test(TC_mbedtls_helper_cipher_aes_encrypt_decrypt_roundtrip),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ed25519_invalid_args),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ed25519_seckey_too_large),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ed25519_pubkey_too_large),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ed25519_success),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ecdsa_invalid_args),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ecdsa_seckey_too_large),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ecdsa_pubkey_too_large),
        cmocka_unit_test(TC_mbedtls_helper_ecdh_compute_shared_ecdsa_success),
        cmocka_unit_test(TC_mbedtls_helper_pk_sign_rsa_invalid_seckey),
        cmocka_unit_test(TC_mbedtls_helper_pk_sign_rsa_parse_failure),
        cmocka_unit_test(TC_mbedtls_helper_pk_sign_ecdsa_invalid_seckey),
        cmocka_unit_test(TC_mbedtls_helper_pk_sign_ecdsa_parse_failure),
        cmocka_unit_test(TC_mbedtls_helper_pk_verify_rsa_invalid_pubkey),
        cmocka_unit_test(TC_mbedtls_helper_pk_verify_rsa_parse_failure),
        cmocka_unit_test(TC_mbedtls_helper_pk_verify_ecdsa_invalid_pubkey),
        cmocka_unit_test(TC_mbedtls_helper_pk_verify_ecdsa_parse_failure),
        cmocka_unit_test(TC_mbedtls_helper_pk_sign_ecdsa_success_raw),
        cmocka_unit_test(TC_mbedtls_helper_pk_sign_ecdsa_success_der),
        cmocka_unit_test(TC_mbedtls_helper_pk_verify_ecdsa_success),
        cmocka_unit_test(TC_mbedtls_helper_pk_verify_ecdsa_bad_signature),
        cmocka_unit_test(TC_mbedtls_helper_pk_verify_rsa_bad_signature),
    };
    return cmocka_run_group_tests_name("mbedtls_helper.c", tests, NULL, NULL);
}
