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

#include <string.h>

#include "cmocka_custom.h"
#include "iot_bsp_nv_data.h"
#include "iot_nv_data.h"

#define UNUSED(x) (void **)(x)

void TC_iot_bsp_nv_get_data_path_wifi_prov_status(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_WIFI_PROV_STATUS);
    assert_non_null(path);
    assert_string_equal(path, "WifiProvStatus");
}

void TC_iot_bsp_nv_get_data_path_ap_ssid(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_AP_SSID);
    assert_non_null(path);
    assert_string_equal(path, "IotAPSSID");
}

void TC_iot_bsp_nv_get_data_path_ap_pass(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_AP_PASS);
    assert_non_null(path);
    assert_string_equal(path, "IotAPPASS");
}

void TC_iot_bsp_nv_get_data_path_ap_bssid(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_AP_BSSID);
    assert_non_null(path);
    assert_string_equal(path, "IotAPBSSID");
}

void TC_iot_bsp_nv_get_data_path_ap_auth_type(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_AP_AUTH_TYPE);
    assert_non_null(path);
    assert_string_equal(path, "IotAPAuthType");
}

void TC_iot_bsp_nv_get_data_path_cloud_prov_status(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_CLOUD_PROV_STATUS);
    assert_non_null(path);
    assert_string_equal(path, "CloudProvStatus");
}

void TC_iot_bsp_nv_get_data_path_server_url(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_SERVER_URL);
    assert_non_null(path);
    assert_string_equal(path, "ServerURL");
}

void TC_iot_bsp_nv_get_data_path_server_port(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_SERVER_PORT);
    assert_non_null(path);
    assert_string_equal(path, "ServerPort");
}

void TC_iot_bsp_nv_get_data_path_label(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_LABEL);
    assert_non_null(path);
    assert_string_equal(path, "Label");
}

void TC_iot_bsp_nv_get_data_path_device_id(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_DEVICE_ID);
    assert_non_null(path);
    assert_string_equal(path, "DeviceID");
}

void TC_iot_bsp_nv_get_data_path_misc_info(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_MISC_INFO);
    assert_non_null(path);
    assert_string_equal(path, "MiscInfo");
}

void TC_iot_bsp_nv_get_data_path_private_key(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_PRIVATE_KEY);
    assert_non_null(path);
    assert_string_equal(path, "PrivateKey");
}

void TC_iot_bsp_nv_get_data_path_public_key(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_PUBLIC_KEY);
    assert_non_null(path);
    assert_string_equal(path, "PublicKey");
}

void TC_iot_bsp_nv_get_data_path_root_ca_cert(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_ROOT_CA_CERT);
    assert_non_null(path);
    assert_string_equal(path, "RootCert");
}

void TC_iot_bsp_nv_get_data_path_sub_ca_cert(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_SUB_CA_CERT);
    assert_non_null(path);
    assert_string_equal(path, "SubCert");
}

void TC_iot_bsp_nv_get_data_path_device_cert(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_DEVICE_CERT);
    assert_non_null(path);
    assert_string_equal(path, "DeviceCert");
}

void TC_iot_bsp_nv_get_data_path_serial_num(void **state)
{
    const char *path;
    UNUSED(state);

    path = iot_bsp_nv_get_data_path(IOT_NVD_SERIAL_NUM);
    assert_non_null(path);
    assert_string_equal(path, "SerialNum");
}

void TC_iot_bsp_nv_get_data_path_unknown_returns_null(void **state)
{
    const char *path;
    UNUSED(state);

    // When: an unknown but in-range value is passed
    path = iot_bsp_nv_get_data_path(IOT_NVD_UNKNOWN);
    // Then: posix returns NULL via the default case
    assert_null(path);
}

void TC_iot_bsp_nv_get_data_path_negative_index(void **state)
{
    const char *path;
    UNUSED(state);

    // When: negative index (out of range)
    path = iot_bsp_nv_get_data_path((iot_nvd_t)-1);
    // Then: posix returns NULL via the IOT_WARN_CHECK
    assert_null(path);
}

void TC_iot_bsp_nv_get_data_path_out_of_range_high(void **state)
{
    const char *path;
    UNUSED(state);

    // When: value greater than IOT_NVD_MAX
    path = iot_bsp_nv_get_data_path((iot_nvd_t)(IOT_NVD_MAX + 5));
    // Then
    assert_null(path);
}

void TC_iot_bsp_nv_get_data_path_at_max_returns_null(void **state)
{
    const char *path;
    UNUSED(state);

    // When: IOT_NVD_MAX has no corresponding path
    path = iot_bsp_nv_get_data_path(IOT_NVD_MAX);
    // Then
    assert_null(path);
}

void TC_iot_bsp_nv_get_data_path_far_out_of_range(void **state)
{
    const char *path;
    UNUSED(state);

    // When: very large invalid value
    path = iot_bsp_nv_get_data_path((iot_nvd_t)0x7FFFFFFF);
    // Then
    assert_null(path);
}

void TC_iot_bsp_nv_get_data_path_negative_min(void **state)
{
    const char *path;
    UNUSED(state);

    // When: minimal negative
    path = iot_bsp_nv_get_data_path((iot_nvd_t)-100);
    // Then
    assert_null(path);
}

void TC_iot_bsp_nv_get_data_path_paths_distinct(void **state)
{
    UNUSED(state);

    // Ensure that two valid paths are not identical (basic structural check)
    assert_string_not_equal(iot_bsp_nv_get_data_path(IOT_NVD_AP_SSID),
                            iot_bsp_nv_get_data_path(IOT_NVD_AP_PASS));
    assert_string_not_equal(iot_bsp_nv_get_data_path(IOT_NVD_PRIVATE_KEY),
                            iot_bsp_nv_get_data_path(IOT_NVD_PUBLIC_KEY));
}
