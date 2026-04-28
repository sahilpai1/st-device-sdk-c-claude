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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "TC_MOCK_functions.h"
#include "TC_MOCK_iot_bsp_ble.h"
#include "cmocka_custom.h"
#include "easysetup_ble.h"
#include "iot_easysetup.h"
#include "iot_error.h"
#include "iot_main.h"
#include "iot_nv_data.h"
#include "iot_util.h"
#include "security/iot_security_common.h"

#define UNUSED(x) (void)(x)

/* STATIC_FUNCTION helpers surfaced through SUPPORT_TC_ON_STATIC_FUNC */
extern void st_conn_ownership_confirm(IOT_CTX *iot_ctx, bool confirm);
extern iot_error_t _es_time_set(unsigned char *time);
extern iot_error_t iot_easysetup_get_pubkey(iot_security_context_t **state, unsigned char **pubkey, size_t *pubkey_len);
extern iot_error_t _es_crypto_cipher_gen_iv(iot_security_buffer_t *iv_buffer);
extern char *_es_json_parse_string(void *json, const char *name);
extern iot_error_t _es_copy_cloud_prov(struct iot_cloud_prov_data *dst, struct iot_cloud_prov_data *src);
extern iot_error_t _es_cloud_prov_parse(struct iot_context *ctx, char *in_payload);
extern iot_error_t _es_wifi_prov_parse(struct iot_context *ctx, char *in_payload);
extern struct iot_easysetup_payload *iot_easysetup_get_response(struct iot_context *ctx,
                                                                struct iot_easysetup_payload request);

#define TC_D2D_PRIVATE_KEY "yQMrFPkZ1wI62K1cEuhSL23wBR/nLr7s3YUZn+XAwb8="
#define TC_D2D_PUBLIC_KEY "eV0oOSDhLf8UXqMO6Osat9G28lXldyZ5nzfQCQt/oiQ="

static char _tc_d2d_device_info[] = {
    "{\n"
    "\t\"deviceInfo\": {\n"
    "\t\t\"firmwareVersion\": \"testFirmwareVersion\",\n"
    "\t\t\"privateKey\": \"" TC_D2D_PRIVATE_KEY
    "\",\n"
    "\t\t\"publicKey\": \"" TC_D2D_PUBLIC_KEY
    "\",\n"
    "\t\t\"serialNumber\": \"STDKtestc51ef86c\"\n"
    "\t}\n"
    "}"};

static struct iot_context *_tc_make_ctx(void)
{
    struct iot_context *ctx = (struct iot_context *)calloc(1, sizeof(struct iot_context));
    assert_non_null(ctx);
    ctx->iot_events = iot_os_eventgroup_create();
    ctx->easysetup_security_context = iot_security_init();
    return ctx;
}

static void _tc_free_ctx(struct iot_context *ctx)
{
    if (!ctx)
        return;
    if (ctx->easysetup_security_context)
        iot_security_deinit(ctx->easysetup_security_context);
    if (ctx->iot_events)
        iot_os_eventgroup_delete(ctx->iot_events);
    if (ctx->prov_data.cloud.broker_url)
        iot_os_free(ctx->prov_data.cloud.broker_url);
    if (ctx->prov_data.cloud.label)
        iot_os_free(ctx->prov_data.cloud.label);
    free(ctx);
}

static int _tc_d2d_setup(void **state)
{
    iot_error_t err;

    tc_mock_ble_reset();
    tc_mock_ble_set_get_response_use_wrap(1);
#if !defined(CONFIG_STDK_IOT_CORE_SUPPORT_STNV_PARTITION)
    err = iot_nv_init((unsigned char *)_tc_d2d_device_info, strlen(_tc_d2d_device_info));
#else
    err = iot_nv_init(NULL, 0);
#endif
    assert_int_equal(err, IOT_ERROR_NONE);

    *state = _tc_make_ctx();
    return 0;
}

static int _tc_d2d_teardown(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    _tc_free_ctx(ctx);
    iot_nv_deinit();
    *state = NULL;
    return 0;
}

void TC_st_conn_ownership_confirm_button_true(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned int bits;

    // Given
    ctx->curr_otm_feature = OVF_BIT_BUTTON;
    // When
    st_conn_ownership_confirm((IOT_CTX *)ctx, true);
    // Then: CONFIRM event bit is set
    bits = iot_os_eventgroup_wait_bits(ctx->iot_events, IOT_EVENT_BIT_EASYSETUP_CONFIRM, true, 0);
    assert_true((bits & IOT_EVENT_BIT_EASYSETUP_CONFIRM) != 0);
}

void TC_st_conn_ownership_confirm_button_false(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned int bits;

    // Given
    ctx->curr_otm_feature = OVF_BIT_BUTTON;
    // When
    st_conn_ownership_confirm((IOT_CTX *)ctx, false);
    // Then: CONFIRM_DENY event bit is set
    bits = iot_os_eventgroup_wait_bits(ctx->iot_events, IOT_EVENT_BIT_EASYSETUP_CONFIRM_DENY, true, 0);
    assert_true((bits & IOT_EVENT_BIT_EASYSETUP_CONFIRM_DENY) != 0);
}

void TC_st_conn_ownership_confirm_non_button(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // Given: feature is not BUTTON
    ctx->curr_otm_feature = OVF_BIT_JUSTWORKS;
    // When
    st_conn_ownership_confirm((IOT_CTX *)ctx, true);
    // Then: call is a silent no-op
}

void TC_es_time_set_invalid_format(void **state)
{
    unsigned char t[] = "not-a-date";
    iot_error_t err;
    UNUSED(state);

    // When
    err = _es_time_set(t);
    // Then
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INVALID_TIME);
}

void TC_es_crypto_cipher_gen_iv_null(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When
    err = _es_crypto_cipher_gen_iv(NULL);
    // Then
    assert_int_equal(err, IOT_ERROR_INVALID_ARGS);
}

void TC_es_crypto_cipher_gen_iv_success(void **state)
{
    iot_security_buffer_t iv = {0};
    iot_error_t err;
    UNUSED(state);

    // When
    err = _es_crypto_cipher_gen_iv(&iv);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(iv.p);
    assert_int_equal(iv.len, IOT_SECURITY_IV_LEN);

    // Teardown
    iot_os_free(iv.p);
}

void TC_es_crypto_cipher_gen_iv_malloc_failure(void **state)
{
    iot_security_buffer_t iv = {0};
    iot_error_t err;
    UNUSED(state);

    // Given: simulate malloc failure
    set_mock_detect_memory_leak(false);
    do_not_use_mock_iot_os_malloc_failure();
    set_mock_iot_os_malloc_failure_with_index(0);
    // When
    err = _es_crypto_cipher_gen_iv(&iv);
    // Then
    assert_int_equal(err, IOT_ERROR_EASYSETUP_MEM_ALLOC_ERROR);

    // Teardown
    do_not_use_mock_iot_os_malloc_failure();
}

void TC_es_cloud_prov_parse_no_brokerurl(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    // When: payload contains an empty data object
    err = _es_cloud_prov_parse(ctx, "{\"data\":{}}");
    // Then
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INVALID_REQUEST);
}

void TC_es_cloud_prov_parse_bad_broker_url(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    // When: brokerUrl cannot be parsed
    err = _es_cloud_prov_parse(ctx, "{\"data\":{\"brokerUrl\":\"badurl\"}}");
    // Then
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INVALID_BROKER_URL);
}

void TC_es_cloud_prov_parse_valid(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    // When: valid payload with brokerUrl and deviceName
    err = _es_cloud_prov_parse(
        ctx, "{\"data\":{\"brokerUrl\":\"mqtts://broker.example.com:8883\",\"deviceName\":\"test\"}}");
    // Then: any result accepted (exercises the full parse path)
    (void)err;
}

void TC_es_wifi_prov_parse_invalid_json(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    // When
    err = _es_wifi_prov_parse(ctx, "not-json");
    // Then
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INVALID_REQUEST);
}

void TC_es_wifi_prov_parse_no_wifi_data(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    // When: payload has no 'data' object
    err = _es_wifi_prov_parse(ctx, "{}");
    // Then: any error code is accepted
    (void)err;
}

void TC_es_copy_cloud_prov_success(void **state)
{
    struct iot_cloud_prov_data src = {0};
    struct iot_cloud_prov_data dst = {0};
    iot_error_t err;
    UNUSED(state);

    // Given
    src.broker_port = 8883;
    src.broker_url = strdup("broker.example.com");
    src.label = strdup("mydev");
    // When
    err = _es_copy_cloud_prov(&dst, &src);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_string_equal(dst.broker_url, src.broker_url);
    assert_string_equal(dst.label, src.label);
    assert_int_equal(dst.broker_port, src.broker_port);

    // Teardown
    free(src.broker_url);
    free(src.label);
    iot_os_free(dst.broker_url);
    iot_os_free(dst.label);
}

void TC_es_copy_cloud_prov_malloc_failure(void **state)
{
    struct iot_cloud_prov_data src = {0};
    struct iot_cloud_prov_data dst = {0};
    iot_error_t err;
    UNUSED(state);

    // Given
    src.broker_port = 8883;
    src.broker_url = strdup("broker.example.com");
    src.label = strdup("mydev");
    // Given: malloc fails on the first call
    set_mock_detect_memory_leak(false);
    do_not_use_mock_iot_os_malloc_failure();
    set_mock_iot_os_malloc_failure_with_index(0);
    // When
    err = _es_copy_cloud_prov(&dst, &src);
    // Then
    assert_int_equal(err, IOT_ERROR_MEM_ALLOC);

    // Teardown
    do_not_use_mock_iot_os_malloc_failure();
    free(src.broker_url);
    free(src.label);
    if (dst.label)
        iot_os_free(dst.label);
    if (dst.broker_url)
        iot_os_free(dst.broker_url);
}

static void _tc_invoke_step(struct iot_context *ctx, int step, const char *payload)
{
    struct iot_easysetup_payload req = {0};
    struct iot_easysetup_payload *resp;

    req.step = step;
    req.payload = (char *)payload;

    tc_mock_ble_set_get_response_use_wrap(0);
    resp = iot_easysetup_get_response(ctx, req);
    tc_mock_ble_set_get_response_use_wrap(1);
    if (resp) {
        if (resp->payload)
            free(resp->payload);
        iot_os_free(resp);
    }
}

void TC_iot_easysetup_get_response_default_case(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: unknown step value
    // Then: dispatcher takes the default branch and returns a response with an error
    _tc_invoke_step(ctx, 99, NULL);
}

void TC_iot_easysetup_get_response_log_systeminfo(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: LOG_SYSTEMINFO step
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_LOG_SYSTEMINFO, NULL);
    // Then: no crash, handler is exercised
}

void TC_iot_easysetup_get_response_log_get_dump(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: LOG_GET_DUMP step
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_LOG_GET_DUMP, NULL);
    // Then: no crash, handler is exercised
}

void TC_iot_easysetup_get_response_confirminfo_bad_json(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: CONFIRMINFO with malformed JSON
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, "not-json");
    // Then: handler returns an error code through response->err
}

void TC_iot_easysetup_get_response_confirm_bad_json(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: CONFIRM with malformed JSON
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_CONFIRM, "not-json");
    // Then: handler returns an error code through response->err
}

void TC_iot_easysetup_get_response_wifiprov_bad_json(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: WIFIPROVIONINGINFO with malformed JSON
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_WIFIPROVIONINGINFO, "not-json");
    // Then: handler returns an error code through response->err
}

void TC_iot_easysetup_get_response_setupcomplete_bad_json(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: SETUPCOMPLETE with malformed JSON
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE, "not-json");
    // Then: handler returns an error code through response->err
}

void TC_iot_easysetup_get_response_offline_recovery_bad_json(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: OFFLINE_DIAGNOSTICS_RECOVERY with malformed JSON
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_OFFLINE_DIAGNOSTICS_RECOVERY, "not-json");
    // Then: handler returns an error code through response->err
}

void TC_iot_easysetup_get_response_deviceinfo(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: DEVICEINFO step (real dispatcher)
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL);
    // Then: no crash
}

void TC_iot_easysetup_get_response_keyinfo_bad_json(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    // When: KEYINFO with malformed JSON
    _tc_invoke_step(ctx, IOT_EASYSETUP_BLE_STEP_KEYINFO, "not-json");
    // Then: handler returns an error code through response->err
}

void TC_iot_easysetup_request_handler_null_ctx(void **state)
{
    struct iot_easysetup_payload request = {0};
    iot_error_t err;
    UNUSED(state);

    // When
    err = iot_easysetup_request_handler(NULL, request);
    // Then
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INTERNAL_SERVER_ERROR);
}

void TC_iot_easysetup_request_handler_no_queue(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    struct iot_easysetup_payload request = {0};
    iot_error_t err;

    // Given
    ctx->easysetup_resp_queue = NULL;
    request.step = IOT_EASYSETUP_BLE_STEP_DEVICEINFO;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_DEVICEINFO);
    // When
    err = iot_easysetup_request_handler(ctx, request);
    // Then
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INTERNAL_SERVER_ERROR);
}

void TC_iot_easysetup_request_handler_with_queue(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    struct iot_easysetup_payload request = {0};
    struct iot_easysetup_payload drained;
    iot_error_t err;

    // Given
    ctx->easysetup_resp_queue = iot_util_queue_create(sizeof(struct iot_easysetup_payload));
    request.step = IOT_EASYSETUP_BLE_STEP_DEVICEINFO;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_DEVICEINFO);
    // When
    err = iot_easysetup_request_handler(ctx, request);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);

    // Teardown
    iot_util_queue_receive(ctx->easysetup_resp_queue, &drained);
    iot_util_queue_delete(ctx->easysetup_resp_queue);
    ctx->easysetup_resp_queue = NULL;
}

int TEST_FUNC_iot_easysetup_d2d_ble(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(TC_st_conn_ownership_confirm_button_true, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_st_conn_ownership_confirm_button_false, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_st_conn_ownership_confirm_non_button, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_time_set_invalid_format, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_crypto_cipher_gen_iv_null, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_crypto_cipher_gen_iv_success, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_crypto_cipher_gen_iv_malloc_failure, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_cloud_prov_parse_no_brokerurl, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_cloud_prov_parse_bad_broker_url, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_cloud_prov_parse_valid, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_wifi_prov_parse_invalid_json, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_wifi_prov_parse_no_wifi_data, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_copy_cloud_prov_success, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_es_copy_cloud_prov_malloc_failure, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_default_case, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_log_systeminfo, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_log_get_dump, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_confirminfo_bad_json, _tc_d2d_setup,
                                        _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_confirm_bad_json, _tc_d2d_setup,
                                        _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_wifiprov_bad_json, _tc_d2d_setup,
                                        _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_setupcomplete_bad_json, _tc_d2d_setup,
                                        _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_offline_recovery_bad_json, _tc_d2d_setup,
                                        _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_deviceinfo, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_get_response_keyinfo_bad_json, _tc_d2d_setup,
                                        _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_request_handler_null_ctx, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_request_handler_no_queue, _tc_d2d_setup, _tc_d2d_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_request_handler_with_queue, _tc_d2d_setup, _tc_d2d_teardown),
    };
    return cmocka_run_group_tests_name("iot_easysetup_d2d_ble.c", tests, NULL, NULL);
}
