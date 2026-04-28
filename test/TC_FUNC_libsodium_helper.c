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
#include <sodium.h>
#include <string.h>

#include "TC_MOCK_functions.h"
#include "cmocka_custom.h"
#include "libsodium_helper.h"

#define UNUSED(x) (void)(x)

struct tc_keypair {
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    unsigned char sk_full[crypto_sign_SECRETKEYBYTES]; /* seed[32] + pk[32] */
};

static void _tc_make_keypair(struct tc_keypair *kp)
{
    int ret = crypto_sign_keypair(kp->pk, kp->sk_full);
    assert_int_equal(ret, 0);
}

void TC_libsodium_helper_pk_sign_ed25519_invalid_pubkey(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    UNUSED(state);

    // When: pubkey buffer pointer is NULL
    err = libsodium_helper_pk_sign_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_PUBKEY);
}

void TC_libsodium_helper_pk_sign_ed25519_invalid_pubkey_len(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    unsigned char pubkey[16];
    UNUSED(state);

    // Given: pubkey set but with the wrong length
    pk.pubkey.p = pubkey;
    pk.pubkey.len = sizeof(pubkey);
    // When
    err = libsodium_helper_pk_sign_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_PUBKEY);
}

void TC_libsodium_helper_pk_sign_ed25519_invalid_seckey(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    struct tc_keypair kp;
    iot_error_t err;
    UNUSED(state);

    // Given: valid pubkey, but seckey buffer pointer is NULL
    _tc_make_keypair(&kp);
    pk.pubkey.p = kp.pk;
    pk.pubkey.len = sizeof(kp.pk);
    // When
    err = libsodium_helper_pk_sign_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_SECKEY);
}

void TC_libsodium_helper_pk_sign_ed25519_invalid_seckey_len(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    struct tc_keypair kp;
    iot_error_t err;
    unsigned char seckey_short[16];
    UNUSED(state);

    // Given: pubkey valid, seckey set but too short
    _tc_make_keypair(&kp);
    pk.pubkey.p = kp.pk;
    pk.pubkey.len = sizeof(kp.pk);
    pk.seckey.p = seckey_short;
    pk.seckey.len = sizeof(seckey_short);
    // When
    err = libsodium_helper_pk_sign_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_SECKEY);
}

void TC_libsodium_helper_pk_sign_ed25519_malloc_failure(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    struct tc_keypair kp;
    unsigned char message[] = "hello";
    iot_error_t err;
    UNUSED(state);

    // Given
    _tc_make_keypair(&kp);
    pk.pubkey.p = kp.pk;
    pk.pubkey.len = sizeof(kp.pk);
    pk.seckey.p = kp.sk_full; /* first 32 bytes are the seed */
    pk.seckey.len = crypto_sign_PUBLICKEYBYTES;
    input.p = message;
    input.len = sizeof(message) - 1;
    set_mock_detect_memory_leak(false);
    do_not_use_mock_iot_os_malloc_failure();
    set_mock_iot_os_malloc_failure_with_index(0);
    // When
    err = libsodium_helper_pk_sign_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_MEM_ALLOC);

    // Teardown
    do_not_use_mock_iot_os_malloc_failure();
}

void TC_libsodium_helper_pk_sign_ed25519_success_and_verify(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    struct tc_keypair kp;
    unsigned char message[] = "the quick brown fox";
    iot_error_t err;
    UNUSED(state);

    // Given: a fresh keypair, the message and the helper-style key layout
    _tc_make_keypair(&kp);
    pk.pubkey.p = kp.pk;
    pk.pubkey.len = sizeof(kp.pk);
    pk.seckey.p = kp.sk_full;
    pk.seckey.len = crypto_sign_PUBLICKEYBYTES;
    input.p = message;
    input.len = sizeof(message) - 1;

    // When: sign
    err = libsodium_helper_pk_sign_ed25519(&pk, &input, &sig);
    // Then: produces a 64-byte signature
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_int_equal(sig.len, 64);
    assert_non_null(sig.p);

    // When: verify the same signature -> should succeed
    err = libsodium_helper_pk_verify_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);

    // Teardown
    iot_security_buffer_free(&sig);
}

void TC_libsodium_helper_pk_verify_ed25519_invalid_pubkey(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    UNUSED(state);

    // When: pubkey buffer pointer is NULL
    err = libsodium_helper_pk_verify_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_PUBKEY);

    // When: pubkey set but length zero
    {
        unsigned char buf[1];
        pk.pubkey.p = buf;
        pk.pubkey.len = 0;
        err = libsodium_helper_pk_verify_ed25519(&pk, &input, &sig);
        // Then
        assert_int_equal(err, IOT_ERROR_SECURITY_PK_INVALID_PUBKEY);
    }
}

void TC_libsodium_helper_pk_verify_ed25519_wrong_pubkey_len(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char short_pubkey[16];
    iot_error_t err;
    UNUSED(state);

    // Given: pubkey is the wrong size for ed25519
    pk.pubkey.p = short_pubkey;
    pk.pubkey.len = sizeof(short_pubkey);
    // When
    err = libsodium_helper_pk_verify_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_KEY_LEN);
}

void TC_libsodium_helper_pk_verify_ed25519_signature_mismatch(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    struct tc_keypair kp;
    unsigned char message[] = "valid input";
    unsigned char tampered_sig[64] = {0};
    iot_error_t err;
    UNUSED(state);

    // Given: valid pubkey but a 64-byte all-zero signature won't verify
    _tc_make_keypair(&kp);
    pk.pubkey.p = kp.pk;
    pk.pubkey.len = sizeof(kp.pk);
    input.p = message;
    input.len = sizeof(message) - 1;
    sig.p = tampered_sig;
    sig.len = sizeof(tampered_sig);
    // When
    err = libsodium_helper_pk_verify_ed25519(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_PK_VERIFY);
}

int TEST_FUNC_libsodium_helper(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(TC_libsodium_helper_pk_sign_ed25519_invalid_pubkey),
        cmocka_unit_test(TC_libsodium_helper_pk_sign_ed25519_invalid_pubkey_len),
        cmocka_unit_test(TC_libsodium_helper_pk_sign_ed25519_invalid_seckey),
        cmocka_unit_test(TC_libsodium_helper_pk_sign_ed25519_invalid_seckey_len),
        cmocka_unit_test(TC_libsodium_helper_pk_sign_ed25519_malloc_failure),
        cmocka_unit_test(TC_libsodium_helper_pk_sign_ed25519_success_and_verify),
        cmocka_unit_test(TC_libsodium_helper_pk_verify_ed25519_invalid_pubkey),
        cmocka_unit_test(TC_libsodium_helper_pk_verify_ed25519_wrong_pubkey_len),
        cmocka_unit_test(TC_libsodium_helper_pk_verify_ed25519_signature_mismatch),
    };
    return cmocka_run_group_tests_name("libsodium_helper.c", tests, NULL, NULL);
}
