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
#include "TC_MOCK_iot_bsp_ble.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "iot_bsp_ble.h"
#include "iot_error.h"
#include "iot_security_common.h"

#define UNUSED(x) (void)(x)

/* Mock state for iot_bsp_ble_get_mtu */
static uint32_t mock_ble_mtu = 0;

/* Mock state for iot_send_indication */
static int mock_send_indication_rc = 0;
static int mock_send_indication_call_count = 0;
static uint32_t mock_send_indication_last_len = 0;

/* Mock state for es_msg_dispatch */
static int mock_es_msg_dispatch_call_count = 0;
static uint8_t mock_es_msg_dispatch_last_cmd_num = 0;
static uint8_t mock_es_msg_dispatch_last_buf_count = 0;

void tc_mock_ble_reset(void)
{
    mock_ble_mtu = 0;
    mock_send_indication_rc = 0;
    mock_send_indication_call_count = 0;
    mock_send_indication_last_len = 0;
    mock_es_msg_dispatch_call_count = 0;
    mock_es_msg_dispatch_last_cmd_num = 0;
    mock_es_msg_dispatch_last_buf_count = 0;
}

void tc_mock_ble_set_mtu(uint32_t mtu)
{
    mock_ble_mtu = mtu;
}

void tc_mock_ble_set_send_indication_rc(int rc)
{
    mock_send_indication_rc = rc;
}

int tc_mock_ble_get_send_indication_call_count(void)
{
    return mock_send_indication_call_count;
}

uint32_t tc_mock_ble_get_send_indication_last_len(void)
{
    return mock_send_indication_last_len;
}

int tc_mock_ble_get_es_msg_dispatch_call_count(void)
{
    return mock_es_msg_dispatch_call_count;
}

uint8_t tc_mock_ble_get_es_msg_dispatch_last_cmd_num(void)
{
    return mock_es_msg_dispatch_last_cmd_num;
}

uint8_t tc_mock_ble_get_es_msg_dispatch_last_buf_count(void)
{
    return mock_es_msg_dispatch_last_buf_count;
}

/* Wrap for iot_bsp_ble_get_mtu */
uint32_t __wrap_iot_bsp_ble_get_mtu(void)
{
    return mock_ble_mtu;
}

/* Wrap for iot_send_indication */
int __wrap_iot_send_indication(uint8_t *buf, uint32_t len)
{
    UNUSED(buf);
    mock_send_indication_call_count++;
    mock_send_indication_last_len = len;
    return mock_send_indication_rc;
}

/* Wrap for es_msg_dispatch: record the call but do nothing else, so the
 * iot_easysetup_ble_msg.c tests can verify dispatch was triggered without
 * pulling in the full d2d message handler. */
void __wrap_es_msg_dispatch(iot_security_buffer_t *buf, uint8_t buf_count, uint8_t cmd_num)
{
    UNUSED(buf);
    mock_es_msg_dispatch_call_count++;
    mock_es_msg_dispatch_last_buf_count = buf_count;
    mock_es_msg_dispatch_last_cmd_num = cmd_num;
}
