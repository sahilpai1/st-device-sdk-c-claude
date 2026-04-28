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
#include "iot_bsp_wifi.h"
#include "iot_error.h"

#define UNUSED(x) (void **)(x)

/*
 * iot_bsp_wifi_get_mac, iot_bsp_wifi_set_mode and iot_bsp_wifi_get_scan_result
 * are linker-wrapped for the rest of the suite. Coverage of the real posix
 * implementations is reached via the __real_ aliases.
 */
extern iot_error_t __real_iot_bsp_wifi_get_mac(struct iot_mac *wifi_mac);
extern iot_error_t __real_iot_bsp_wifi_set_mode(iot_wifi_conf *conf);
extern uint16_t __real_iot_bsp_wifi_get_scan_result(iot_wifi_scan_result_t *scan_result);

static void wifi_event_cb(iot_wifi_event_t event, iot_error_t error)
{
    (void)event;
    (void)error;
}

void TC_iot_bsp_wifi_init_returns_success(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When
    err = iot_bsp_wifi_init();
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_wifi_set_mode_real_returns_success(void **state)
{
    iot_error_t err;
    iot_wifi_conf conf;
    UNUSED(state);

    // Given: a configuration buffer
    memset(&conf, 0, sizeof(conf));
    conf.mode = IOT_WIFI_MODE_STATION;
    // When: real (posix) implementation, which is a no-op stub returning success
    err = __real_iot_bsp_wifi_set_mode(&conf);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_wifi_set_mode_real_null_conf(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When: NULL config to the real posix stub
    err = __real_iot_bsp_wifi_set_mode(NULL);
    // Then: posix stub does not dereference -> still success
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_wifi_get_scan_result_real_returns_zero(void **state)
{
    uint16_t count;
    iot_wifi_scan_result_t buf[1];
    UNUSED(state);

    // When: real posix stub
    count = __real_iot_bsp_wifi_get_scan_result(buf);
    // Then: posix stub always returns 0
    assert_int_equal(count, 0);
}

void TC_iot_bsp_wifi_get_scan_result_real_null_buffer(void **state)
{
    uint16_t count;
    UNUSED(state);

    // When: NULL pointer (posix stub does not touch it)
    count = __real_iot_bsp_wifi_get_scan_result(NULL);
    // Then
    assert_int_equal(count, 0);
}

void TC_iot_bsp_wifi_get_mac_real_no_iface(void **state)
{
    iot_error_t err;
    struct iot_mac mac;
    UNUSED(state);

    // Given: most CI environments do not have a wlan0 interface
    memset(&mac, 0, sizeof(mac));
    // When: real posix implementation
    err = __real_iot_bsp_wifi_get_mac(&mac);
    // Then: ioctl fails -> READ_FAIL; if wlan0 is somehow present accept NONE
    assert_true(err == IOT_ERROR_READ_FAIL || err == IOT_ERROR_NONE);
}

void TC_iot_bsp_wifi_get_freq_returns_2_4g_only(void **state)
{
    iot_wifi_freq_t freq;
    UNUSED(state);

    // When
    freq = iot_bsp_wifi_get_freq();
    // Then: posix returns 2.4GHz only
    assert_int_equal(freq, IOT_WIFI_FREQ_2_4G_ONLY);
}

void TC_iot_bsp_wifi_register_event_cb_returns_bad_req(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When: posix does not implement event callbacks
    err = iot_bsp_wifi_register_event_cb(wifi_event_cb);
    // Then
    assert_int_equal(err, IOT_ERROR_BAD_REQ);
}

void TC_iot_bsp_wifi_register_event_cb_null_cb_returns_bad_req(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When: NULL callback (posix returns BAD_REQ regardless)
    err = iot_bsp_wifi_register_event_cb(NULL);
    // Then
    assert_int_equal(err, IOT_ERROR_BAD_REQ);
}

void TC_iot_bsp_wifi_clear_event_cb_no_crash(void **state)
{
    UNUSED(state);

    // When: invoking clear callback is a no-op in posix
    iot_bsp_wifi_clear_event_cb();
    // Then: no crash; second invocation still safe
    iot_bsp_wifi_clear_event_cb();
    assert_true(1);
}

void TC_iot_bsp_wifi_get_auth_mode_excludes_unsupported(void **state)
{
    iot_wifi_auth_mode_bits_t bits;
    UNUSED(state);

    // When: query supported auth modes
    bits = iot_bsp_wifi_get_auth_mode();
    // Then: WPA2_ENTERPRISE and WPA3_PERSONAL must be excluded; OPEN included
    assert_true((bits & IOT_WIFI_AUTH_MODE_BIT(IOT_WIFI_AUTH_OPEN)) != 0);
    assert_true((bits & IOT_WIFI_AUTH_MODE_BIT(IOT_WIFI_AUTH_WPA2_PSK)) != 0);
    assert_int_equal(bits & IOT_WIFI_AUTH_MODE_BIT(IOT_WIFI_AUTH_WPA2_ENTERPRISE), 0);
    assert_int_equal(bits & IOT_WIFI_AUTH_MODE_BIT(IOT_WIFI_AUTH_WPA3_PERSONAL), 0);
}

void TC_iot_bsp_wifi_get_auth_mode_consistent(void **state)
{
    iot_wifi_auth_mode_bits_t bits1;
    iot_wifi_auth_mode_bits_t bits2;
    UNUSED(state);

    // When: invoking twice
    bits1 = iot_bsp_wifi_get_auth_mode();
    bits2 = iot_bsp_wifi_get_auth_mode();
    // Then
    assert_int_equal(bits1, bits2);
}

void TC_iot_bsp_wifi_get_status_returns_success(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When
    err = iot_bsp_wifi_get_status();
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
}
