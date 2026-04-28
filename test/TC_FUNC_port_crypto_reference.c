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
#include <port/port_crypto.h>
#include <string.h>

#include "cmocka_custom.h"

#define UNUSED(x) (void)(x)

void TC_port_crypto_generate_key_unsupported_type(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_error_t err;
    UNUSED(state);

    // When: an unsupported key type takes the switch's default branch
    err = port_crypto_generate_key(IOT_SECURITY_KEY_TYPE_UNKNOWN, &seckey, &pubkey);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_MANAGER_KEY_GENERATE);
}

void TC_port_crypto_pk_sign_invalid_input(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    iot_error_t err;
    unsigned char data[] = "x";
    UNUSED(state);

    // When: input pointer is NULL
    err = port_crypto_pk_sign(&pk, NULL, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: input pointer set but buffer pointer NULL
    err = port_crypto_pk_sign(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: input buffer set but length is 0
    input.p = data;
    input.len = 0;
    err = port_crypto_pk_sign(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_port_crypto_pk_sign_null_sig(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // Given: valid input but sig is null
    input.p = data;
    input.len = 1;
    // When
    err = port_crypto_pk_sign(&pk, &input, NULL);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_port_crypto_pk_sign_unsupported_type(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // Given: an unsupported pk type
    input.p = data;
    input.len = 1;
    pk.type = IOT_SECURITY_KEY_TYPE_UNKNOWN; /* takes the default switch branch */
    // When
    err = port_crypto_pk_sign(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_NOT_IMPLEMENTED);
}

void TC_port_crypto_pk_sign_dispatches_rsa(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // Given: RSA2048 type with empty params dispatches to mbedtls_helper which errors
    input.p = data;
    input.len = 1;
    pk.type = IOT_SECURITY_KEY_TYPE_RSA2048;
    // When
    err = port_crypto_pk_sign(&pk, &input, &sig);
    // Then: any non-NONE error is acceptable; the dispatcher branch is what's exercised
    assert_int_not_equal(err, IOT_ERROR_NONE);
}

void TC_port_crypto_pk_sign_dispatches_eccp256(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // Given: ECCP256 type with empty params
    input.p = data;
    input.len = 1;
    pk.type = IOT_SECURITY_KEY_TYPE_ECCP256;
    // When
    err = port_crypto_pk_sign(&pk, &input, &sig);
    // Then
    assert_int_not_equal(err, IOT_ERROR_NONE);
}

void TC_port_crypto_pk_verify_invalid_input(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // When: null input
    err = port_crypto_pk_verify(&pk, NULL, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: input.p NULL
    err = port_crypto_pk_verify(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: input.len 0
    input.p = data;
    input.len = 0;
    err = port_crypto_pk_verify(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_port_crypto_pk_verify_invalid_sig(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // Given: valid input
    input.p = data;
    input.len = 1;

    // When: sig pointer NULL
    err = port_crypto_pk_verify(&pk, &input, NULL);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: sig.p NULL
    err = port_crypto_pk_verify(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: sig.len == 0
    sig.p = data;
    sig.len = 0;
    err = port_crypto_pk_verify(&pk, &input, &sig);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_port_crypto_pk_verify_unsupported_type(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    unsigned char sig_data[] = "y";
    iot_error_t err;
    UNUSED(state);

    // Given: an unsupported pk type with otherwise valid buffers
    input.p = data;
    input.len = 1;
    sig.p = sig_data;
    sig.len = 1;
    pk.type = IOT_SECURITY_KEY_TYPE_UNKNOWN;
    // When
    err = port_crypto_pk_verify(&pk, &input, &sig);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_NOT_IMPLEMENTED);
}

void TC_port_crypto_pk_verify_dispatches_rsa(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    unsigned char sig_data[] = "y";
    iot_error_t err;
    UNUSED(state);

    input.p = data;
    input.len = 1;
    sig.p = sig_data;
    sig.len = 1;
    pk.type = IOT_SECURITY_KEY_TYPE_RSA2048;
    err = port_crypto_pk_verify(&pk, &input, &sig);
    assert_int_not_equal(err, IOT_ERROR_NONE);
}

void TC_port_crypto_pk_verify_dispatches_eccp256(void **state)
{
    iot_security_pk_params_t pk = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t sig = {0};
    unsigned char data[] = "x";
    unsigned char sig_data[] = "y";
    iot_error_t err;
    UNUSED(state);

    input.p = data;
    input.len = 1;
    sig.p = sig_data;
    sig.len = 1;
    pk.type = IOT_SECURITY_KEY_TYPE_ECCP256;
    err = port_crypto_pk_verify(&pk, &input, &sig);
    assert_int_not_equal(err, IOT_ERROR_NONE);
}

void TC_port_crypto_cipher_encrypt_invalid_args(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // When: NULL input
    err = port_crypto_cipher_encrypt(&cipher, NULL, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: input.p NULL
    err = port_crypto_cipher_encrypt(&cipher, &input, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: input set but output NULL
    input.p = data;
    input.len = 1;
    err = port_crypto_cipher_encrypt(&cipher, &input, NULL);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_port_crypto_cipher_encrypt_unsupported_algo(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // Given: an unsupported cipher type
    input.p = data;
    input.len = 1;
    cipher.type = IOT_SECURITY_KEY_TYPE_ED25519;
    // When
    err = port_crypto_cipher_encrypt(&cipher, &input, &output);
    // Then
    assert_int_equal(err, IOT_ERROR_SECURITY_CIPHER_INVALID_ALGO);
}

void TC_port_crypto_cipher_decrypt_invalid_args(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    // When: NULL input
    err = port_crypto_cipher_decrypt(&cipher, NULL, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: input.p NULL
    err = port_crypto_cipher_decrypt(&cipher, &input, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);

    // When: output NULL
    input.p = data;
    input.len = 1;
    err = port_crypto_cipher_decrypt(&cipher, &input, NULL);
    assert_int_equal(err, IOT_ERROR_SECURITY_INVALID_ARGS);
}

void TC_port_crypto_cipher_decrypt_unsupported_algo(void **state)
{
    iot_security_cipher_params_t cipher = {0};
    iot_security_buffer_t input = {0};
    iot_security_buffer_t output = {0};
    unsigned char data[] = "x";
    iot_error_t err;
    UNUSED(state);

    input.p = data;
    input.len = 1;
    cipher.type = IOT_SECURITY_KEY_TYPE_RSA2048;
    err = port_crypto_cipher_decrypt(&cipher, &input, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_CIPHER_INVALID_ALGO);
}

void TC_port_crypto_compute_ecdh_unsupported_type(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    iot_error_t err;
    UNUSED(state);

    err = port_crypto_compute_ecdh_shared(IOT_SECURITY_KEY_TYPE_RSA2048, &seckey, &pubkey, &output);
    assert_int_equal(err, IOT_ERROR_SECURITY_ECDH_LIBRARY);
}

void TC_port_crypto_compute_ecdh_eccp256_dispatch(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    iot_error_t err;
    UNUSED(state);

    /* Empty keys cause the underlying mbedtls helper to fail, but the
     * ECCP256 dispatch branch is exercised. */
    err = port_crypto_compute_ecdh_shared(IOT_SECURITY_KEY_TYPE_ECCP256, &seckey, &pubkey, &output);
    assert_int_not_equal(err, IOT_ERROR_NONE);
}

void TC_port_crypto_compute_ecdh_ed25519_dispatch(void **state)
{
    iot_security_buffer_t seckey = {0};
    iot_security_buffer_t pubkey = {0};
    iot_security_buffer_t output = {0};
    iot_error_t err;
    UNUSED(state);

    err = port_crypto_compute_ecdh_shared(IOT_SECURITY_KEY_TYPE_ED25519, &seckey, &pubkey, &output);
    assert_int_not_equal(err, IOT_ERROR_NONE);
}

int TEST_FUNC_port_crypto_reference(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(TC_port_crypto_generate_key_unsupported_type),
        cmocka_unit_test(TC_port_crypto_pk_sign_invalid_input),
        cmocka_unit_test(TC_port_crypto_pk_sign_null_sig),
        cmocka_unit_test(TC_port_crypto_pk_sign_unsupported_type),
        cmocka_unit_test(TC_port_crypto_pk_sign_dispatches_rsa),
        cmocka_unit_test(TC_port_crypto_pk_sign_dispatches_eccp256),
        cmocka_unit_test(TC_port_crypto_pk_verify_invalid_input),
        cmocka_unit_test(TC_port_crypto_pk_verify_invalid_sig),
        cmocka_unit_test(TC_port_crypto_pk_verify_unsupported_type),
        cmocka_unit_test(TC_port_crypto_pk_verify_dispatches_rsa),
        cmocka_unit_test(TC_port_crypto_pk_verify_dispatches_eccp256),
        cmocka_unit_test(TC_port_crypto_cipher_encrypt_invalid_args),
        cmocka_unit_test(TC_port_crypto_cipher_encrypt_unsupported_algo),
        cmocka_unit_test(TC_port_crypto_cipher_decrypt_invalid_args),
        cmocka_unit_test(TC_port_crypto_cipher_decrypt_unsupported_algo),
        cmocka_unit_test(TC_port_crypto_compute_ecdh_unsupported_type),
        cmocka_unit_test(TC_port_crypto_compute_ecdh_eccp256_dispatch),
        cmocka_unit_test(TC_port_crypto_compute_ecdh_ed25519_dispatch),
    };
    return cmocka_run_group_tests_name("port_crypto_reference.c", tests, NULL, NULL);
}
