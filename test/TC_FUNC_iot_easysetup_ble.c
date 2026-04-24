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
#include "iot_bsp_ble.h"
#include "iot_easysetup.h"
#include "iot_error.h"
#include "iot_main.h"
#include "iot_util.h"
#include "security/iot_security_common.h"
#include "security/iot_security_crypto.h"

#define UNUSED(x) (void)(x)

/* Exposed globals / statics-turned-externs via SUPPORT_TC_ON_STATIC_FUNC */
extern struct iot_context *context;
extern int ref_step;

iot_error_t iot_easysetup_init(struct iot_context *ctx);
void iot_easysetup_deinit(struct iot_context *ctx);
iot_error_t iot_easysetup_ble_send_response(int cmd, char *payload, size_t payload_len);
void iot_easysetup_ble_msg_handler(int cmd, char *data_buf, size_t data_buf_len);
void _ble_deinit_request_handler(struct iot_context *ctx, device_work_param param);
void _send_ble_deinit_request(void);

/* STATIC_FUNCTION helpers surfaced for unit testing. */
iot_error_t _iot_easysetup_con_timer_init(struct iot_context *ctx);
void _iot_easysetup_ble_conn_cb(iot_ble_conn_evt_t evt);
iot_error_t _iot_easysetup_gen_payload(struct iot_context *ctx, int cmd, char *in_payload, char **out_payload,
                                       size_t *payload_len);
iot_error_t _iot_easysetup_ble_msg_decrypt(iot_security_context_t *security_context, int cmd,
                                           unsigned char *encrypt_msg, size_t encrypt_msg_len, char **out_msg);
iot_error_t _iot_easysetup_ble_msg_encrypt(struct iot_context *context, int cmd, unsigned char *payload,
                                           size_t payload_len, iot_security_buffer_t **encrypt_buf, int *buf_len);

/*
 * Minimal fixture: heap-allocated iot_context with the fields the BLE module
 * reads.  Tests call _tc_ble_setup/_tc_ble_teardown to run setup on a fresh
 * context every time.
 */
static struct iot_context *_tc_make_context(void)
{
    struct iot_context *ctx = (struct iot_context *)calloc(1, sizeof(struct iot_context));
    assert_non_null(ctx);
    ctx->iot_events = iot_os_eventgroup_create();
    ctx->work_queue = iot_util_queue_create(sizeof(device_work_data_t));
    ctx->work_queue_signal = iot_os_eventgroup_create();
    ctx->easysetup_security_context = iot_security_init();
    ctx->otm_confirmed = false;
    return ctx;
}

static void _tc_free_context(struct iot_context *ctx)
{
    if (!ctx)
        return;
    if (ctx->easysetup_security_context) {
        iot_security_deinit(ctx->easysetup_security_context);
    }
    if (ctx->work_queue) {
        iot_util_queue_delete(ctx->work_queue);
    }
    if (ctx->work_queue_signal) {
        iot_os_eventgroup_delete(ctx->work_queue_signal);
    }
    if (ctx->iot_events) {
        iot_os_eventgroup_delete(ctx->iot_events);
    }
    if (ctx->cloud_con_timer) {
        iot_os_timer_destroy(&ctx->cloud_con_timer);
    }
    if (ctx->offline_diagnostics_wifiupdate_timeout) {
        iot_os_timer_delete(ctx->offline_diagnostics_wifiupdate_timeout);
    }
    if (ctx->lookup_id) {
        free(ctx->lookup_id);
    }
    free(ctx);
}

static int _tc_ble_setup(void **state)
{
    tc_mock_ble_reset();
    tc_mock_ble_set_mtu(256);
    context = NULL;
    ref_step = 0;
    struct iot_context *ctx = _tc_make_context();
    *state = ctx;
    return 0;
}

static int _tc_ble_teardown(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    context = NULL;
    _tc_free_context(ctx);
    *state = NULL;
    return 0;
}

/*
 * iot_easysetup_init / deinit tests
 */
void TC_iot_easysetup_init_null_ctx(void **state)
{
    iot_error_t err;
    UNUSED(state);

    err = iot_easysetup_init(NULL);
    assert_int_equal(err, IOT_ERROR_INVALID_ARGS);
}

void TC_iot_easysetup_init_success(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    ctx->es_ble_ready = false;
    err = iot_easysetup_init(ctx);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_true(ctx->es_ble_ready);
    assert_ptr_equal(context, ctx);
    assert_int_equal(ref_step, 0);
}

void TC_iot_easysetup_init_already_ready(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    ctx->es_ble_ready = true;
    err = iot_easysetup_init(ctx);
    assert_int_equal(err, IOT_ERROR_NONE);
    /* Starting advertisement and BLE init are skipped when already ready. */
    assert_int_equal(tc_mock_ble_get_start_adv_call_count(), 0);
}

void TC_iot_easysetup_deinit_null_ctx(void **state)
{
    UNUSED(state);
    /* Must not crash on NULL context. */
    iot_easysetup_deinit(NULL);
}

void TC_iot_easysetup_deinit_not_ready(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_ble_ready = false;
    ctx->wifi_update_enabled = true;

    iot_easysetup_deinit(ctx);
    assert_false(ctx->wifi_update_enabled);
}

/*
 * iot_easysetup_ble_msg_handler negative cases (invalid cmd)
 */
void TC_iot_easysetup_ble_msg_handler_cmd_below_range(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    /* cmd < DEVICEINFO triggers the err_report path */
    iot_easysetup_ble_msg_handler(-5, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_cmd_above_range(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    /* cmd >= INVALID_STEP triggers the err_report path */
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_INVALID_STEP + 3, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = IOT_ERROR_NONE;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response_mqtt_reject(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = IOT_ERROR_MQTT_REJECT_CONNECT;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response_auth_fail(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = IOT_ERROR_CONN_STA_AUTH_FAIL;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response_dhcp_fail(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = IOT_ERROR_CONN_STA_DHCP_FAIL;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response_dns_fail(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = IOT_ERROR_CONN_DNS_QUERY_FAIL;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response_ap_not_found(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = IOT_ERROR_CONN_STA_AP_NOT_FOUND;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response_operate_fail(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = IOT_ERROR_CONN_OPERATE_FAIL;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

void TC_iot_easysetup_ble_msg_handler_setup_complete_response_default(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_network_status = (iot_error_t)0xDEADC0DE;
    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE_RESPONSE, NULL, 0);
}

/*
 * iot_easysetup_ble_msg_handler with good deviceinfo cmd -> exercises
 * _iot_easysetup_gen_payload success via mocked get_response.
 */
void TC_iot_easysetup_ble_msg_handler_deviceinfo_success(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_DEVICEINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, 0);
    assert_int_equal(tc_mock_ble_get_get_response_call_count(), 1);
}

void TC_iot_easysetup_ble_msg_handler_setupcomplete(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ref_step = IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_SETUPCOMPLETE, NULL, 0);
    /* Should have initialized the cloud_con_timer */
    assert_non_null(ctx->cloud_con_timer);
}

/*
 * _iot_easysetup_gen_payload direct tests
 */
void TC_iot_easysetup_gen_payload_invalid_cmd_while_not_confirmed(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ctx->otm_confirmed = false;
    ref_step = 0;

    /* cmd >= WIFISCANINFO while otm not confirmed returns INVALID_CMD */
    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_WIFISCANINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INVALID_CMD);
}

void TC_iot_easysetup_gen_payload_invalid_step_sequence(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ctx->otm_confirmed = true;
    ref_step = 2;

    /* cmd != ref_step and not one of the special steps (DEVICEINFO and
     * KEYINFO are plain sequence steps, not the allowed out-of-sequence
     * set WIFISCANINFO/SETUPCOMPLETE/CONFIRMINFO). */
    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_KEYINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INVALID_CMD);
}

void TC_iot_easysetup_gen_payload_response_null(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ref_step = 0;
    tc_mock_ble_set_get_response_return_null(1);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INTERNAL_SERVER_ERROR);
}

void TC_iot_easysetup_gen_payload_step_mismatch(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_KEYINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INTERNAL_SERVER_ERROR);
}

void TC_iot_easysetup_gen_payload_wifiscaninfo_sync(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ctx->otm_confirmed = true;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_WIFISCANINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_WIFISCANINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_easysetup_gen_payload_confirminfo_sync(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ctx->otm_confirmed = false;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_CONFIRMINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_easysetup_gen_payload_log_systeminfo_allowed_unconfirmed(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ctx->otm_confirmed = false;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_LOG_SYSTEMINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_LOG_SYSTEMINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_easysetup_gen_payload_response_err(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_DEVICEINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_EASYSETUP_INVALID_SEQUENCE);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INVALID_SEQUENCE);
}

/*
 * _iot_easysetup_ble_msg_decrypt tests
 */
void TC_iot_easysetup_ble_msg_decrypt_deviceinfo_passthrough(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char enc[] = {'a', 'b', 'c'};
    char *out = NULL;
    iot_error_t err;

    context = ctx;

    err = _iot_easysetup_ble_msg_decrypt(ctx->easysetup_security_context, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, enc,
                                         sizeof(enc), &out);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_ptr_equal(out, (char *)enc);
}

void TC_iot_easysetup_ble_msg_decrypt_no_cipher_params(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char enc[] = {'a', 'b', 'c'};
    char *out = NULL;
    iot_error_t err;

    context = ctx;
    /* CONFIRMINFO is not in the plaintext list, so it tries to decrypt. */
    err = _iot_easysetup_ble_msg_decrypt(ctx->easysetup_security_context, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, enc,
                                         sizeof(enc), &out);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_INTERNAL_SERVER_ERROR);
}

/*
 * _iot_easysetup_ble_msg_encrypt invalid-args
 */
void TC_iot_easysetup_ble_msg_encrypt_null_payload(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_security_buffer_t *enc_buf = NULL;
    int buf_len = 0;
    iot_error_t err;

    context = ctx;

    err = _iot_easysetup_ble_msg_encrypt(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, 0, &enc_buf, &buf_len);
    assert_int_equal(err, IOT_ERROR_INVALID_ARGS);
}

/*
 * _iot_easysetup_con_timer_init
 */
void TC_iot_easysetup_con_timer_init_success(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    err = _iot_easysetup_con_timer_init(ctx);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(ctx->cloud_con_timer);

    /* Calling again deletes and recreates */
    err = _iot_easysetup_con_timer_init(ctx);
    assert_int_equal(err, IOT_ERROR_NONE);
}

/*
 * _iot_easysetup_ble_conn_cb
 */
void TC_iot_easysetup_ble_conn_cb_connected(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ref_step = 5;
    _iot_easysetup_ble_conn_cb(IOT_BLE_CONNECTION_EVENT_CONNECTED);
    assert_int_equal(ref_step, 0);
    assert_true(ctx->ble_connected);
}

void TC_iot_easysetup_ble_conn_cb_disconnected(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->ble_connected = true;
    ctx->d2d_event_request = true;
    ctx->wifi_update_enabled = false;
    /* Queue a work item to make sure the queue-drain path runs */
    device_work_data_t work = {0};
    iot_util_queue_send(ctx->work_queue, &work);

    _iot_easysetup_ble_conn_cb(IOT_BLE_CONNECTION_EVENT_DISCONNECTED);
    assert_false(ctx->ble_connected);
    assert_false(ctx->d2d_event_request);
}

void TC_iot_easysetup_ble_conn_cb_unknown_event(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    _iot_easysetup_ble_conn_cb((iot_ble_conn_evt_t)99);
}

/*
 * _send_ble_deinit_request + _ble_deinit_request_handler
 */
void TC_send_ble_deinit_request(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    _send_ble_deinit_request();
    /* Verify the work item was queued */
    device_work_data_t received = {0};
    iot_error_t err = iot_util_queue_receive(ctx->work_queue, &received);
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_ble_deinit_request_handler(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    ctx->es_ble_ready = true;
    _ble_deinit_request_handler(ctx, NULL);
    assert_false(ctx->es_ble_ready);
}

/*
 * iot_easysetup_ble_send_response
 */
void TC_iot_easysetup_ble_send_response_null_payload(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    context = ctx;
    err = iot_easysetup_ble_send_response(IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, 0);
    assert_int_not_equal(err, IOT_ERROR_NONE);
}

/*
 * Helper: initialize cipher on the easysetup_security_context so
 * encrypt/decrypt tests can exercise the real AES path.
 */
static void _tc_ble_init_cipher(struct iot_context *ctx)
{
    static unsigned char secret_buf[IOT_SECURITY_SECRET_LEN];
    static unsigned char iv_buf[IOT_SECURITY_IV_LEN];
    iot_security_cipher_params_t aes_params = {0};
    iot_error_t err;
    size_t i;

    err = iot_security_cipher_init(ctx->easysetup_security_context);
    assert_int_equal(err, IOT_ERROR_NONE);
    for (i = 0; i < sizeof(secret_buf); i++)
        secret_buf[i] = (unsigned char)(i * 7 + 1);
    for (i = 0; i < sizeof(iv_buf); i++)
        iv_buf[i] = (unsigned char)(i * 11 + 3);

    aes_params.type = IOT_SECURITY_KEY_TYPE_AES256;
    aes_params.key.p = secret_buf;
    aes_params.key.len = sizeof(secret_buf);
    aes_params.iv.p = iv_buf;
    aes_params.iv.len = sizeof(iv_buf);
    err = iot_security_cipher_set_params(ctx->easysetup_security_context, &aes_params);
    assert_int_equal(err, IOT_ERROR_NONE);
}

/*
 * Encrypt/decrypt round-trip using a real cipher context.  Covers the AES
 * branches of _iot_easysetup_ble_msg_encrypt and _iot_easysetup_ble_msg_decrypt.
 */
void TC_iot_easysetup_ble_msg_encrypt_decrypt_roundtrip(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char payload[32];
    iot_security_buffer_t *enc_buf = NULL;
    int buf_len = 0;
    char *decrypted = NULL;
    iot_error_t err;
    size_t i;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    for (i = 0; i < sizeof(payload); i++)
        payload[i] = (unsigned char)(i + 1);

    /* KEYINFO is the passthrough step for decrypt, so use CONFIRMINFO to
     * actually run aes_encrypt / aes_decrypt.  Set a reasonable MTU. */
    tc_mock_ble_set_mtu(256);
    err = _iot_easysetup_ble_msg_encrypt(ctx, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, payload, sizeof(payload), &enc_buf,
                                         &buf_len);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(enc_buf);
    assert_true(buf_len >= 1);

    err = _iot_easysetup_ble_msg_decrypt(ctx->easysetup_security_context, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO,
                                         enc_buf[0].p, enc_buf[0].len, &decrypted);
    assert_int_equal(err, IOT_ERROR_NONE);

    if (enc_buf) {
        for (i = 0; i < (size_t)buf_len; i++) {
            if (enc_buf[i].p)
                iot_os_free(enc_buf[i].p);
        }
        iot_os_free(enc_buf);
    }
    if (decrypted)
        iot_os_free((void *)decrypted);
}

/*
 * DEVICEINFO in encrypt path is the "no-cipher" branch that just copies
 * payload bytes (no AES).  Covers lines 395-408.
 */
void TC_iot_easysetup_ble_msg_encrypt_deviceinfo_passthrough(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char payload[16];
    iot_security_buffer_t *enc_buf = NULL;
    int buf_len = 0;
    iot_error_t err;
    size_t i;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    for (i = 0; i < sizeof(payload); i++)
        payload[i] = (unsigned char)(i + 1);

    tc_mock_ble_set_mtu(256);
    err = _iot_easysetup_ble_msg_encrypt(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, payload, sizeof(payload), &enc_buf,
                                         &buf_len);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_non_null(enc_buf);

    for (i = 0; i < (size_t)buf_len; i++) {
        if (enc_buf[i].p)
            iot_os_free(enc_buf[i].p);
    }
    iot_os_free(enc_buf);
}

/*
 * Small MTU clamping on encrypt (covers mtu < MIN_MTU_SIZE and mtu > MAX).
 */
void TC_iot_easysetup_ble_msg_encrypt_small_mtu(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    iot_security_buffer_t *enc_buf = NULL;
    int buf_len = 0;
    iot_error_t err;
    size_t i;

    context = ctx;
    _tc_ble_init_cipher(ctx);

    tc_mock_ble_set_mtu(5); /* below MIN_MTU_SIZE -> clamped to MIN */
    err = _iot_easysetup_ble_msg_encrypt(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, payload, sizeof(payload), &enc_buf,
                                         &buf_len);
    assert_int_equal(err, IOT_ERROR_NONE);
    for (i = 0; i < (size_t)buf_len; i++) {
        if (enc_buf[i].p)
            iot_os_free(enc_buf[i].p);
    }
    iot_os_free(enc_buf);
}

void TC_iot_easysetup_ble_msg_encrypt_large_mtu(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    iot_security_buffer_t *enc_buf = NULL;
    int buf_len = 0;
    iot_error_t err;
    size_t i;

    context = ctx;
    _tc_ble_init_cipher(ctx);

    tc_mock_ble_set_mtu(1024); /* above MAX_ATT_VALUE_LEN -> clamped to MAX */
    err = _iot_easysetup_ble_msg_encrypt(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, payload, sizeof(payload), &enc_buf,
                                         &buf_len);
    assert_int_equal(err, IOT_ERROR_NONE);
    for (i = 0; i < (size_t)buf_len; i++) {
        if (enc_buf[i].p)
            iot_os_free(enc_buf[i].p);
    }
    iot_os_free(enc_buf);
}

/*
 * Encrypt with no cipher params yields IOT_ERROR_INVALID_ARGS.
 */
void TC_iot_easysetup_ble_msg_encrypt_no_cipher_params(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char payload[4] = {1, 2, 3, 4};
    iot_security_buffer_t *enc_buf = NULL;
    int buf_len = 0;
    iot_error_t err;

    context = ctx;
    /* security_context exists but cipher_params == NULL */
    tc_mock_ble_set_mtu(128);
    err = _iot_easysetup_ble_msg_encrypt(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, payload, sizeof(payload), &enc_buf,
                                         &buf_len);
    assert_int_equal(err, IOT_ERROR_INVALID_ARGS);
}

/*
 * iot_easysetup_ble_send_response with encryption failure (no cipher_params).
 */
void TC_iot_easysetup_ble_send_response_no_cipher_params(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    context = ctx;
    tc_mock_ble_set_mtu(128);
    err = iot_easysetup_ble_send_response(IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, "hi", 2);
    assert_int_equal(err, IOT_ERROR_INVALID_ARGS);
}

/*
 * iot_easysetup_ble_send_response success path (encrypts and sends).
 */
void TC_iot_easysetup_ble_send_response_deviceinfo_success(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    iot_error_t err;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    tc_mock_ble_set_mtu(256);
    err = iot_easysetup_ble_send_response(IOT_EASYSETUP_BLE_STEP_DEVICEINFO, "hello", 5);
    assert_int_equal(err, IOT_ERROR_NONE);
}

/*
 * iot_easysetup_deinit with ready ctx + network error -> goes through the
 * encrypt-and-report path then iot_state_update.  The wrap for state_update
 * doesn't exist so state_update real impl will run but with empty context it
 * will produce some errors; the important part is we cover the code.
 */
void TC_iot_easysetup_deinit_ready_network_error(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    ctx->es_ble_ready = true;
    ctx->es_network_status = IOT_ERROR_CONN_STA_AUTH_FAIL;
    tc_mock_ble_set_mtu(256);

    /* Will go through iot_easysetup_ble_msg_handler(SETUPCOMPLETE_RESPONSE) */
    iot_easysetup_deinit(ctx);
}

/*
 * iot_easysetup_deinit with ready ctx + success -> starts advertisement.
 */
void TC_iot_easysetup_deinit_ready_success(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    ctx->es_ble_ready = true;
    ctx->es_network_status = IOT_ERROR_NONE;
    tc_mock_ble_set_mtu(256);

    iot_easysetup_deinit(ctx);
    assert_int_equal(tc_mock_ble_get_start_adv_call_count(), 1);
}

/*
 * iot_easysetup_ble_conn_cb DISCONNECTED with cipher_params set -> covers
 * the cipher_deinit path.
 */
void TC_iot_easysetup_ble_conn_cb_disconnected_with_cipher(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    ctx->ble_connected = true;
    ctx->wifi_update_enabled = true; /* bypass iot_device_cleanup/state_update */
    _iot_easysetup_ble_conn_cb(IOT_BLE_CONNECTION_EVENT_DISCONNECTED);
    assert_false(ctx->ble_connected);
}

/*
 * iot_easysetup_ble_msg_handler deviceinfo with status_cb set.
 */
static int tc_status_cb_count;
static void _tc_status_cb(int status, void *user_data)
{
    (void)status;
    (void)user_data;
    tc_status_cb_count++;
}

void TC_iot_easysetup_ble_msg_handler_deviceinfo_with_status_cb(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    ctx->status_cb = _tc_status_cb;
    tc_status_cb_count = 0;
    ref_step = 0;
    tc_mock_ble_set_mtu(256);
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_DEVICEINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, 0);
    assert_true(tc_status_cb_count >= 1);
}

/*
 * gen_payload for OFFLINE_DIAGNOSTICS steps: exercises the ref_step=0 reset
 * branches in the success tail.
 */
void TC_iot_easysetup_gen_payload_offline_diagnostics_connection_info(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ctx->otm_confirmed = true;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_OFFLINE_DIAGNOSTICS_CONNECTION_INFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_OFFLINE_DIAGNOSTICS_CONNECTION_INFO, NULL,
                                     &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_int_equal(ref_step, 0);
}

/*
 * Encrypt path: malloc failure for the encrypt_buf array.
 */
void TC_iot_easysetup_ble_msg_encrypt_array_malloc_failure(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char payload[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    iot_security_buffer_t *enc_buf = NULL;
    int buf_len = 0;
    iot_error_t err;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    tc_mock_ble_set_mtu(256);

    set_mock_detect_memory_leak(false);
    do_not_use_mock_iot_os_malloc_failure();
    set_mock_iot_os_malloc_failure_with_index(0);
    err = _iot_easysetup_ble_msg_encrypt(ctx, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, payload, sizeof(payload), &enc_buf,
                                         &buf_len);
    do_not_use_mock_iot_os_malloc_failure();
    assert_int_equal(err, IOT_ERROR_MEM_ALLOC);
}

/*
 * Decrypt path: malloc failure for the decrypt buffer.
 */
void TC_iot_easysetup_ble_msg_decrypt_malloc_failure(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char enc[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    char *out = NULL;
    iot_error_t err;

    context = ctx;
    _tc_ble_init_cipher(ctx);

    set_mock_detect_memory_leak(false);
    do_not_use_mock_iot_os_malloc_failure();
    set_mock_iot_os_malloc_failure_with_index(0);
    err = _iot_easysetup_ble_msg_decrypt(ctx->easysetup_security_context, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, enc,
                                         sizeof(enc), &out);
    do_not_use_mock_iot_os_malloc_failure();
    assert_int_equal(err, IOT_ERROR_EASYSETUP_MEM_ALLOC_ERROR);
}

/*
 * Decrypt path: real AES decrypt failure (garbage input).
 */
void TC_iot_easysetup_ble_msg_decrypt_bad_input(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char enc[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    char *out = NULL;
    iot_error_t err;

    context = ctx;
    _tc_ble_init_cipher(ctx);

    /* Input is not a valid encrypted blob: the AES decrypt call should
     * report an error. */
    err = _iot_easysetup_ble_msg_decrypt(ctx->easysetup_security_context, IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, enc,
                                         sizeof(enc), &out);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_AES256_DECRYPTION_ERROR);
}

/*
 * msg_handler with non-empty data_buf: exercises the decrypt call in
 * msg_handler (lines 493-498).
 */
void TC_iot_easysetup_ble_msg_handler_with_payload(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char enc[16];
    size_t i;

    context = ctx;
    _tc_ble_init_cipher(ctx);
    for (i = 0; i < sizeof(enc); i++)
        enc[i] = (unsigned char)(i * 3);
    ref_step = 0;
    tc_mock_ble_set_mtu(256);
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_DEVICEINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_NONE);

    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_DEVICEINFO, (char *)enc, sizeof(enc));
}

/*
 * msg_handler with decrypt error: pass a garbage payload on a non-passthrough
 * step (CONFIRMINFO).  Covers the decrypt-error early return.
 */
void TC_iot_easysetup_ble_msg_handler_decrypt_error(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    unsigned char enc[16] = {0};

    context = ctx;
    _tc_ble_init_cipher(ctx);
    tc_mock_ble_set_mtu(256);

    iot_easysetup_ble_msg_handler(IOT_EASYSETUP_BLE_STEP_CONFIRMINFO, (char *)enc, sizeof(enc));
}

/*
 * gen_payload when response has REQUEST_PENDING error: returns early.
 */
void TC_iot_easysetup_gen_payload_request_pending(void **state)
{
    struct iot_context *ctx = (struct iot_context *)*state;
    char *out_payload = NULL;
    size_t payload_len = 0;
    iot_error_t err;

    context = ctx;
    ref_step = 0;
    tc_mock_ble_set_get_response_step(IOT_EASYSETUP_BLE_STEP_DEVICEINFO);
    tc_mock_ble_set_get_response_err(IOT_ERROR_EASYSETUP_REQUEST_PENDING);

    err = _iot_easysetup_gen_payload(ctx, IOT_EASYSETUP_BLE_STEP_DEVICEINFO, NULL, &out_payload, &payload_len);
    assert_int_equal(err, IOT_ERROR_EASYSETUP_REQUEST_PENDING);
}

/*
 * Group registration
 */
int TEST_FUNC_iot_easysetup_ble(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_init_null_ctx, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_init_success, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_init_already_ready, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_deinit_null_ctx, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_deinit_not_ready, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_cmd_below_range, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_cmd_above_range, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response_mqtt_reject,
                                        _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response_auth_fail,
                                        _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response_dhcp_fail,
                                        _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response_dns_fail,
                                        _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response_ap_not_found,
                                        _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response_operate_fail,
                                        _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setup_complete_response_default, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_deviceinfo_success, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_setupcomplete, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_invalid_cmd_while_not_confirmed, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_invalid_step_sequence, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_response_null, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_step_mismatch, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_wifiscaninfo_sync, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_confirminfo_sync, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_log_systeminfo_allowed_unconfirmed, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_response_err, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_decrypt_deviceinfo_passthrough, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_decrypt_no_cipher_params, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_encrypt_null_payload, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_con_timer_init_success, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_conn_cb_connected, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_conn_cb_disconnected, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_conn_cb_unknown_event, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_send_ble_deinit_request, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_ble_deinit_request_handler, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_send_response_null_payload, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_encrypt_decrypt_roundtrip, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_encrypt_deviceinfo_passthrough, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_encrypt_small_mtu, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_encrypt_large_mtu, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_encrypt_no_cipher_params, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_send_response_no_cipher_params, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_send_response_deviceinfo_success, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_deinit_ready_network_error, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_deinit_ready_success, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_conn_cb_disconnected_with_cipher, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_deviceinfo_with_status_cb, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_offline_diagnostics_connection_info, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_encrypt_array_malloc_failure, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_decrypt_malloc_failure, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_decrypt_bad_input, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_with_payload, _tc_ble_setup, _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_ble_msg_handler_decrypt_error, _tc_ble_setup,
                                        _tc_ble_teardown),
        cmocka_unit_test_setup_teardown(TC_iot_easysetup_gen_payload_request_pending, _tc_ble_setup, _tc_ble_teardown),
    };
    return cmocka_run_group_tests_name("iot_easysetup_ble.c", tests, NULL, NULL);
}
