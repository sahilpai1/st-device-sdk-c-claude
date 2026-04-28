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
#include <string.h>

#include "cmocka_custom.h"
#include "iot_bsp_ble.h"
#include "iot_error.h"

#define UNUSED(x) (void **)(x)

static bool dummy_write_cb(uint8_t *buf, uint32_t len)
{
    (void)buf;
    (void)len;
    return true;
}

static void dummy_conn_cb(iot_ble_conn_evt_t evt)
{
    (void)evt;
}

void TC_iot_bsp_ble_posix_init_with_valid_callbacks(void **state)
{
    iot_error_t err;
    iot_ble_cbs_t cbs;
    UNUSED(state);

    // Given: callbacks struct populated
    cbs.conn_cb = dummy_conn_cb;
    cbs.write_cb = dummy_write_cb;
    // When
    err = iot_bsp_ble_init(&cbs);
    // Then: posix stub returns success
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_ble_posix_init_null_callbacks(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When: NULL callback container
    err = iot_bsp_ble_init(NULL);
    // Then: posix stub still returns success (no-op stub)
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_ble_posix_init_callbacks_inner_null(void **state)
{
    iot_error_t err;
    iot_ble_cbs_t cbs;
    UNUSED(state);

    // Given: empty callbacks struct
    cbs.conn_cb = NULL;
    cbs.write_cb = NULL;
    // When
    err = iot_bsp_ble_init(&cbs);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_ble_posix_init_partial_callbacks(void **state)
{
    iot_error_t err;
    iot_ble_cbs_t cbs;
    UNUSED(state);

    // Given: only one callback set
    cbs.conn_cb = dummy_conn_cb;
    cbs.write_cb = NULL;
    // When
    err = iot_bsp_ble_init(&cbs);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_ble_posix_deinit_no_crash(void **state)
{
    UNUSED(state);

    // When: deinit invoked
    iot_bsp_ble_deinit();
    // Then: no crash and returns void
}

void TC_iot_bsp_ble_posix_start_adv_success(void **state)
{
    int ret;
    uint8_t mn_data[4] = {0x01, 0x02, 0x03, 0x04};
    char local_name[] = "test_dev";
    UNUSED(state);

    // When: invoking start_adv with valid arguments
    ret = iot_bsp_ble_start_adv(0x1234, mn_data, sizeof(mn_data), local_name);
    // Then: posix stub returns 0
    assert_int_equal(ret, 0);
}

void TC_iot_bsp_ble_posix_start_adv_null_mn_data(void **state)
{
    int ret;
    char local_name[] = "name";
    UNUSED(state);

    // When: mn_data NULL
    ret = iot_bsp_ble_start_adv(0x0000, NULL, 0, local_name);
    // Then: posix stub returns 0
    assert_int_equal(ret, 0);
}

void TC_iot_bsp_ble_posix_start_adv_null_local_name(void **state)
{
    int ret;
    uint8_t mn_data[2] = {0xAA, 0xBB};
    UNUSED(state);

    // When: local_name NULL
    ret = iot_bsp_ble_start_adv(0xFFFF, mn_data, sizeof(mn_data), NULL);
    // Then: posix stub returns 0
    assert_int_equal(ret, 0);
}

void TC_iot_bsp_ble_posix_start_adv_zero_length(void **state)
{
    int ret;
    uint8_t buf[1] = {0};
    UNUSED(state);

    // When: zero data length
    ret = iot_bsp_ble_start_adv(0x0001, buf, 0, "name");
    // Then
    assert_int_equal(ret, 0);
}

void TC_iot_bsp_ble_posix_start_adv_all_null(void **state)
{
    int ret;
    UNUSED(state);

    // When: all pointer args NULL
    ret = iot_bsp_ble_start_adv(0x0000, NULL, 0, NULL);
    // Then: posix stub returns 0
    assert_int_equal(ret, 0);
}

void TC_iot_bsp_ble_posix_stop_adv_returns_zero(void **state)
{
    int ret;
    UNUSED(state);

    // When
    ret = iot_bsp_ble_stop_adv();
    // Then
    assert_int_equal(ret, 0);
}

void TC_iot_send_indication_posix_success(void **state)
{
    int ret;
    uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    UNUSED(state);

    // When: valid indication payload
    ret = iot_send_indication(buf, sizeof(buf));
    // Then
    assert_int_equal(ret, 0);
}

void TC_iot_send_indication_posix_null_buffer(void **state)
{
    int ret;
    UNUSED(state);

    // When: NULL buffer
    ret = iot_send_indication(NULL, 0);
    // Then
    assert_int_equal(ret, 0);
}

void TC_iot_send_indication_posix_zero_length(void **state)
{
    int ret;
    uint8_t buf[1] = {0};
    UNUSED(state);

    // When: zero length
    ret = iot_send_indication(buf, 0);
    // Then
    assert_int_equal(ret, 0);
}

void TC_iot_bsp_ble_posix_get_mtu_returns_zero(void **state)
{
    uint32_t mtu;
    UNUSED(state);

    // When
    mtu = iot_bsp_ble_get_mtu();
    // Then: posix stub returns 0
    assert_int_equal(mtu, 0);
}

void TC_iot_bsp_ble_posix_get_mac_address_success(void **state)
{
    int ret;
    uint8_t mac[6] = {0};
    UNUSED(state);

    // When: valid mac buffer
    ret = iot_bsp_ble_get_mac_address(mac);
    // Then: posix stub returns 0
    assert_int_equal(ret, 0);
}

void TC_iot_bsp_ble_posix_disconnect_returns_zero(void **state)
{
    int ret;
    UNUSED(state);

    // When
    ret = iot_bsp_ble_disconnect();
    // Then
    assert_int_equal(ret, 0);
}
