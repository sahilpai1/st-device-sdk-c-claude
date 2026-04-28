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

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cmocka_custom.h"
#include "iot_bsp_system.h"
#include "iot_error.h"

#define UNUSED(x) (void **)(x)

/*
 * iot_bsp_system_set_time_in_sec is wrapped (mocked) to avoid clobbering
 * the test runner's wall-clock. To exercise the real posix implementation
 * for coverage purposes, the linker exposes the original symbol via
 * __real_iot_bsp_system_set_time_in_sec.
 */
extern iot_error_t __real_iot_bsp_system_set_time_in_sec(time_t time_in_sec);
extern void __real_iot_bsp_system_reboot(void);

void TC_iot_bsp_get_bsp_name_returns_posix(void **state)
{
    const char *name;
    UNUSED(state);

    // When
    name = iot_bsp_get_bsp_name();
    // Then
    assert_non_null(name);
    assert_string_equal(name, "posix");
}

void TC_iot_bsp_get_bsp_name_consistent_calls(void **state)
{
    const char *first;
    const char *second;
    UNUSED(state);

    // When: invoking twice
    first = iot_bsp_get_bsp_name();
    second = iot_bsp_get_bsp_name();
    // Then: result is identical (it's a literal string)
    assert_ptr_equal(first, second);
}

void TC_iot_bsp_get_bsp_version_string_returns_empty(void **state)
{
    const char *version;
    UNUSED(state);

    // When
    version = iot_bsp_get_bsp_version_string();
    // Then: posix version is an empty string
    assert_non_null(version);
    assert_string_equal(version, "");
}

void TC_iot_bsp_get_bsp_version_string_consistent_calls(void **state)
{
    const char *first;
    const char *second;
    UNUSED(state);

    // When: called more than once
    first = iot_bsp_get_bsp_version_string();
    second = iot_bsp_get_bsp_version_string();
    // Then
    assert_ptr_equal(first, second);
}

void TC_iot_bsp_system_get_time_in_sec_success(void **state)
{
    iot_error_t err;
    time_t t = 0;
    UNUSED(state);

    // When
    err = iot_bsp_system_get_time_in_sec(&t);
    // Then: success and value populated to a recent epoch
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_true(t > 0);
}

void TC_iot_bsp_system_get_time_in_sec_advances(void **state)
{
    iot_error_t err;
    time_t t1 = 0;
    time_t t2 = 0;
    UNUSED(state);

    // When: two reads with sleep in between
    err = iot_bsp_system_get_time_in_sec(&t1);
    assert_int_equal(err, IOT_ERROR_NONE);
    sleep(1);
    err = iot_bsp_system_get_time_in_sec(&t2);
    // Then: time progressed
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_true(t2 >= t1);
}

void TC_iot_bsp_system_set_time_in_sec_negative_value(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // Given: a negative tv_sec is rejected by clock_settime regardless of user
    // privileges; this exercises the failure branch without affecting the
    // host clock.
    // When: invoking the real implementation
    err = __real_iot_bsp_system_set_time_in_sec((time_t)-1);
    // Then: posix returns INVALID_ARGS
    assert_int_equal(err, IOT_ERROR_INVALID_ARGS);
}

void TC_iot_bsp_system_set_time_in_sec_returns_invalid_when_clock_settime_fails(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When: another negative invocation to cover the same error branch
    err = __real_iot_bsp_system_set_time_in_sec((time_t)-86400);
    // Then
    assert_int_equal(err, IOT_ERROR_INVALID_ARGS);
}

void TC_iot_bsp_system_set_timezone_success(void **state)
{
    iot_error_t err;
    char *tz_before;
    UNUSED(state);

    // Given: capture existing TZ
    tz_before = getenv("TZ");
    // When
    err = iot_bsp_system_set_timezone("Asia/Seoul");
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_string_equal(getenv("TZ"), "Asia/Seoul");
    // restore
    if (tz_before) {
        setenv("TZ", tz_before, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
}

void TC_iot_bsp_system_set_timezone_empty_string(void **state)
{
    iot_error_t err;
    char *tz_before;
    UNUSED(state);

    // Given: capture existing TZ
    tz_before = getenv("TZ");
    // When: empty TZ
    err = iot_bsp_system_set_timezone("");
    // Then: setenv with empty value still succeeds; impl does not error
    assert_int_equal(err, IOT_ERROR_NONE);
    // restore
    if (tz_before) {
        setenv("TZ", tz_before, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
}

void TC_iot_bsp_system_set_timezone_other_value(void **state)
{
    iot_error_t err;
    char *tz_before;
    UNUSED(state);

    tz_before = getenv("TZ");
    // When: set to UTC
    err = iot_bsp_system_set_timezone("UTC");
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_string_equal(getenv("TZ"), "UTC");
    if (tz_before) {
        setenv("TZ", tz_before, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
}

void TC_iot_bsp_system_set_timezone_overrides_existing(void **state)
{
    iot_error_t err;
    char *tz_before;
    UNUSED(state);

    tz_before = getenv("TZ");
    // Given: set initial TZ
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();
    // When: override
    err = iot_bsp_system_set_timezone("Europe/London");
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_string_equal(getenv("TZ"), "Europe/London");
    if (tz_before) {
        setenv("TZ", tz_before, 1);
    } else {
        unsetenv("TZ");
    }
    tzset();
}

void TC_iot_bsp_system_reboot_real_calls_exit(void **state)
{
    pid_t pid;
    int status;
    UNUSED(state);

    // Given: real iot_bsp_system_reboot calls exit(0); fork to isolate the
    // exit from the test runner.
    pid = fork();
    if (pid == 0) {
        __real_iot_bsp_system_reboot();
        _exit(2);
    }
    assert_true(pid > 0);
    waitpid(pid, &status, 0);
    // Then: child exited normally with status 0
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);
}

void TC_iot_bsp_system_poweroff_calls_exit(void **state)
{
    pid_t pid;
    int status;
    UNUSED(state);

    // Given: iot_bsp_system_poweroff calls exit(0); fork the test
    pid = fork();
    if (pid == 0) {
        iot_bsp_system_poweroff();
        _exit(2);
    }
    assert_true(pid > 0);
    waitpid(pid, &status, 0);
    // Then
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);
}
