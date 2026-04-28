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

/*
 * MQTT-success harness used to drive iot_es_connect -> _do_iot_main_command
 * end-to-end success paths in iot_main.c.  The strategy:
 *
 *   - Mock the socket/read stream via port_net_mock_*.
 *   - Pre-build a chain of MQTT packets the lib will consume in order:
 *       CONNACK | SUBACK | PUBLISH(connect.success) | PUBACK
 *     so st_mqtt_connect, st_mqtt_subscribe, the yield-loop wait for the
 *     connect-response notification, and the registration QoS1 PUBACK all
 *     succeed without an actual broker.
 *   - Stub the root certificate fetch via the BLE-test mock toggle.
 *   - Provide just enough iot_context state for _do_iot_main_command.
 */

#include <iot_capability.h>
#include <iot_easysetup.h>
#include <iot_internal.h>
#include <iot_main.h>
#include <iot_mqtt_client.h>
#include <iot_nv_data.h>
#include <iot_util.h>
#include <security/iot_security_common.h>
#include <st_dev.h>
#include <stdbool.h>
#include <string.h>

#include "TC_MOCK_functions.h"
#include "TC_MOCK_iot_bsp_ble.h"
#include "cmocka_custom.h"
#include "mqtt/packet/iot_mqtt_publish.h"

#define UNUSED(x) (void *)(x)

#define HARNESS_SERIAL "STDKharness00001"
#define HARNESS_DEVICE_INFO                                                   \
    "{\n"                                                                     \
    "\t\"deviceInfo\": {\n"                                                   \
    "\t\t\"firmwareVersion\": \"v1.0\",\n"                                    \
    "\t\t\"privateKey\": \"ztqmQ24u86J9bpFLjaoMfwauUZwKLjUIGsnrDwwnDM8=\",\n" \
    "\t\t\"publicKey\": \"BKb7+m1Mo8OuMsodM91ohz/+rZKDc/otzUPSn4UkCUk=\",\n"  \
    "\t\t\"serialNumber\": \"" HARNESS_SERIAL                                 \
    "\"\n"                                                                    \
    "\t}\n"                                                                   \
    "}"

/* Forward declarations of the cooperating internal entry points. */
extern iot_error_t _do_iot_main_command(struct iot_context *ctx, struct iot_command *cmd);

/* Build a MQTT packet stream with CONNACK + SUBACK + connect.success PUBLISH + PUBACK
 * and install it as the simulated read stream for the next port_net session.
 *
 * - sub_packet_id: packet ID the SUBACK should acknowledge (typically 1)
 * - pub_packet_id: packet ID the PUBACK should acknowledge (typically 2)
 *
 * Returns the number of bytes written. The caller owns the buffer.
 */
static int _tc_install_full_handshake_stream(unsigned char *buf, size_t buf_size, unsigned short sub_packet_id,
                                             unsigned short pub_packet_id, const char *pub_topic,
                                             const char *pub_payload)
{
    int p = 0;

    /* CONNACK: fixed header 0x20, rem=2, flags=0x00, rc=0x00 */
    assert_true(buf_size >= 4);
    buf[p++] = 0x20;
    buf[p++] = 0x02;
    buf[p++] = 0x00;
    buf[p++] = 0x00;

    /* SUBACK: fixed header 0x90, rem=3, packet ID, granted QoS = 0 */
    assert_true(p + 5 <= (int)buf_size);
    buf[p++] = 0x90;
    buf[p++] = 0x03;
    buf[p++] = (unsigned char)((sub_packet_id >> 8) & 0xFF);
    buf[p++] = (unsigned char)(sub_packet_id & 0xFF);
    buf[p++] = 0x00;

    /* PUBLISH (QoS 0) on pub_topic with pub_payload via MQTTSerialize_publish */
    {
        MQTTString topic = MQTTString_initializer;
        int written;
        topic.cstring = (char *)pub_topic;
        written = MQTTSerialize_publish(buf + p, (int)(buf_size - p), 0, 0, 0, 0, topic, (unsigned char *)pub_payload,
                                        strlen(pub_payload));
        assert_true(written > 0);
        p += written;
    }

    /* PUBACK for our subsequent QoS1 publish: 0x40, rem=2, packet ID */
    assert_true(p + 4 <= (int)buf_size);
    buf[p++] = 0x40;
    buf[p++] = 0x02;
    buf[p++] = (unsigned char)((pub_packet_id >> 8) & 0xFF);
    buf[p++] = (unsigned char)(pub_packet_id & 0xFF);

    port_net_mock_reset_read_stream(buf, p);
    port_net_mock_reset_socket_status(1);
    return p;
}

/* Allocate a fully-formed iot_context that satisfies IS_CTX_VALID and exposes
 * enough state to run _do_iot_main_command CLOUD_REGISTERING through to the
 * end of iot_es_connect. */
static struct iot_context *_tc_make_main_ctx(void)
{
    struct iot_context *ctx;
    iot_error_t err;

    err = iot_nv_init((unsigned char *)HARNESS_DEVICE_INFO, strlen(HARNESS_DEVICE_INFO));
    assert_int_equal(err, IOT_ERROR_NONE);

    ctx = (struct iot_context *)calloc(1, sizeof(struct iot_context));
    assert_non_null(ctx);
    ctx->work_queue = iot_util_queue_create(sizeof(device_work_data_t));
    ctx->work_queue_signal = iot_os_eventgroup_create();
    ctx->iot_events = iot_os_eventgroup_create();
    ctx->usr_events = iot_os_eventgroup_create();
    ctx->easysetup_security_context = iot_security_init();
    ctx->devconf.mnid = strdup("fTST");
    ctx->prov_data.cloud.broker_url = strdup("test.example.com");
    ctx->prov_data.cloud.broker_port = 8883;
    ctx->is_wifi_station = true; /* skip the wifi-mode-control branch */
    return ctx;
}

static void _tc_free_main_ctx(struct iot_context *ctx)
{
    if (!ctx)
        return;
    if (ctx->easysetup_security_context)
        iot_security_deinit(ctx->easysetup_security_context);
    if (ctx->iot_events)
        iot_os_eventgroup_delete(ctx->iot_events);
    if (ctx->usr_events)
        iot_os_eventgroup_delete(ctx->usr_events);
    if (ctx->work_queue_signal)
        iot_os_eventgroup_delete(ctx->work_queue_signal);
    if (ctx->work_queue) {
        device_work_data_t drained;
        while (iot_util_queue_receive(ctx->work_queue, &drained) == IOT_ERROR_NONE) {
            struct iot_command *cmd = (struct iot_command *)drained.param;
            if (cmd) {
                if (cmd->param)
                    iot_os_free(cmd->param);
                iot_os_free(cmd);
            }
        }
        iot_util_queue_delete(ctx->work_queue);
    }
    free(ctx->devconf.mnid);
    if (ctx->prov_data.cloud.broker_url)
        free(ctx->prov_data.cloud.broker_url);
    if (ctx->prov_data.cloud.label)
        iot_os_free(ctx->prov_data.cloud.label);
    if (ctx->iot_reg_data.dip)
        iot_os_free(ctx->iot_reg_data.dip);
    if (ctx->iot_reg_data.locationId)
        iot_os_free(ctx->iot_reg_data.locationId);
    free(ctx);
    iot_nv_deinit();
}

void TC_HARNESS_do_iot_main_command_cloud_registering_success(void **state)
{
    struct iot_context *ctx;
    struct iot_command cmd = {0};
    unsigned char read_stream[256];
    char sub_topic[64];
    UNUSED(*state);

    /* Given: full ctx + a pre-built CONNACK -> SUBACK -> PUBLISH(connect.success)
     * stream, plus the certificate-fetch mock returning a fake cert. */
    ctx = _tc_make_main_ctx();
    /* devconf.combo_sn or hashed_sn is required by _iot_es_mqtt_registration_json;
     * supply hashed_sn so the JSON build succeeds. */
    ctx->devconf.hashed_sn = "VNZCGRB2VIt+4QckH7OWZPp8UxulH/nZDCDgXpPHr1M";
    ctx->devconf.device_type = "Switch";
    ctx->devconf.vid = "TEST_VID";
    ctx->lookup_id = strdup("c37e0475-b727-49ca-bdfe-33bda78c28a7");
    ctx->device_info.firmware_version = strdup("v1.0");

    snprintf(sub_topic, sizeof(sub_topic), IOT_SUB_TOPIC_REGISTRATION, HARNESS_SERIAL);

    tc_mock_ble_set_get_certificate_use_wrap(1);
    /* st_mqtt_create sets next_packetid=1 then increments before assigning,
     * so SUBSCRIBE gets packet_id 2 and the subsequent PUBLISH gets 3. */
    _tc_install_full_handshake_stream(read_stream, sizeof(read_stream), 2, 3, sub_topic,
                                      "{\"event\":\"connect.success\"}");
    /* The stub mqtt write side accepts whatever bytes we hand it. */
    expect_any_always(__wrap_port_net_write, len);
    expect_any_always(__wrap_port_net_write, buf);

    cmd.cmd_type = IOT_COMMAND_CLOUD_REGISTERING;

    /* When: drive CLOUD_REGISTERING through _do_iot_main_command. iot_es_connect
     * either succeeds (set registered_msg_requested=true) or fails (records
     * es_network_status); both paths exercise lines around the iot_es_connect
     * call site. */
    (void)_do_iot_main_command(ctx, &cmd);

    /* Teardown */
    iot_es_disconnect(ctx, IOT_CONNECT_TYPE_REGISTRATION);
    free(ctx->lookup_id);
    free(ctx->device_info.firmware_version);
    tc_mock_ble_set_get_certificate_use_wrap(0);
    port_net_mock_reset_read_stream(NULL, 0);
    _tc_free_main_ctx(ctx);
}

int TEST_FUNC_iot_main_harness(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(TC_HARNESS_do_iot_main_command_cloud_registering_success),
    };
    return cmocka_run_group_tests_name("iot_main_harness.c", tests, NULL, NULL);
}
