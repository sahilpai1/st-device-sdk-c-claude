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

#include <stdio.h>
#include <unistd.h>

#include "cmocka_custom.h"
#include "iot_bsp_debug.h"
#include "iot_debug.h"

#define UNUSED(x) (void **)(x)

extern void iot_bsp_dump(char *buf);

static void redirect_stdout_to_devnull(int *saved_fd)
{
    fflush(stdout);
    *saved_fd = dup(STDOUT_FILENO);
    int devnull = open("/dev/null", 1);
    if (devnull >= 0) {
        dup2(devnull, STDOUT_FILENO);
        close(devnull);
    }
}

static void restore_stdout(int saved_fd)
{
    fflush(stdout);
    if (saved_fd >= 0) {
        dup2(saved_fd, STDOUT_FILENO);
        close(saved_fd);
    }
}

void TC_iot_bsp_debug_posix_level_info(void **state)
{
    int saved;
    UNUSED(state);

    // When: invoke debug with INFO level
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_INFO, "TAG_INFO", "info message %d", 1);
    restore_stdout(saved);
    // Then: no crash; tested via successful execution
    assert_true(1);
}

void TC_iot_bsp_debug_posix_level_error(void **state)
{
    int saved;
    UNUSED(state);

    // When: ERROR level path
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_ERROR, "TAG_ERR", "error %s", "x");
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_level_warn(void **state)
{
    int saved;
    UNUSED(state);

    // When: WARN level path
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_WARN, "TAG_WARN", "warn %d", 2);
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_level_debug(void **state)
{
    int saved;
    UNUSED(state);

    // When: DEBUG level path
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_DEBUG, "TAG_DBG", "dbg %s", "msg");
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_level_sensitive_info(void **state)
{
    int saved;
    UNUSED(state);

    // When: SENSITIVE INFO level path
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_SENSITIVE_INFO, "TAG_SI", "si %d %s", 3, "msg");
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_level_none(void **state)
{
    int saved;
    UNUSED(state);

    // When: NONE level path falls into default branch
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_NONE, "TAG_NONE", "msg");
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_level_unknown(void **state)
{
    int saved;
    UNUSED(state);

    // When: invalid/unknown level → default branch
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug((iot_debug_level_t)999, "TAG_UNK", "u%d", 4);
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_empty_format(void **state)
{
    int saved;
    UNUSED(state);

    // When: empty format string
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_INFO, "EMPTY", "%s", "");
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_long_message(void **state)
{
    char long_msg[1024];
    int saved;
    int i;
    UNUSED(state);

    // Given: a message larger than the internal BUF_SIZE (512)
    for (i = 0; i < (int)sizeof(long_msg) - 1; i++) {
        long_msg[i] = 'A' + (i % 26);
    }
    long_msg[sizeof(long_msg) - 1] = '\0';

    // When: log a message that should get truncated
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_DEBUG, "TAG_LONG", "%s", long_msg);
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_null_tag(void **state)
{
    int saved;
    UNUSED(state);

    // When: NULL tag passed (printf("%s") prints "(null)" on glibc)
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug(IOT_DEBUG_LEVEL_INFO, NULL, "msg %d", 5);
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_dump_posix_no_op(void **state)
{
    char buf[16] = "test";
    UNUSED(state);

    // When: invoking iot_bsp_dump (no-op stub)
    iot_bsp_dump(buf);
    iot_bsp_dump(NULL);
    // Then: no crash
    assert_true(1);
}

void TC_iot_bsp_debug_posix_check_heap_first(void **state)
{
    int saved;
    UNUSED(state);

    // When: first invocation triggers count==0 branch
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug_check_heap("TAG_HEAP", "func", 42, "h%d", 0);
    restore_stdout(saved);
    assert_true(1);
}

void TC_iot_bsp_debug_posix_check_heap_subsequent(void **state)
{
    int saved;
    UNUSED(state);

    // When: subsequent calls go through the non-first branch
    redirect_stdout_to_devnull(&saved);
    iot_bsp_debug_check_heap("TAG_H2", "f2", 1, "tail-%s", "x");
    iot_bsp_debug_check_heap("TAG_H3", "f3", 2, "tail-%s", "y");
    restore_stdout(saved);
    assert_true(1);
}
