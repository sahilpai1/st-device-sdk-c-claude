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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmocka_custom.h"
#include "iot_bsp_fs.h"
#include "iot_error.h"

#define UNUSED(x) (void **)(x)

#define BSP_FS_TEST_FILE "/tmp/stdk_bsp_fs_test_file"
#define BSP_FS_TEST_FILE_2 "/tmp/stdk_bsp_fs_test_file_2"
#define BSP_FS_NO_SUCH_FILE "/tmp/stdk_bsp_fs_no_such_file_xyz"

static void cleanup_test_files(void)
{
    unlink(BSP_FS_TEST_FILE);
    unlink(BSP_FS_TEST_FILE_2);
    unlink(BSP_FS_NO_SUCH_FILE);
}

void TC_iot_bsp_fs_init_returns_success(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When
    err = iot_bsp_fs_init();
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_fs_deinit_returns_success(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When
    err = iot_bsp_fs_deinit();
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
}

void TC_iot_bsp_fs_open_readwrite_creates_file(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: clean state
    cleanup_test_files();
    // When: opening for read/write creates the file
    err = iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_string_equal(handle.filename, BSP_FS_TEST_FILE);
    iot_bsp_fs_close(handle);
    cleanup_test_files();
}

void TC_iot_bsp_fs_open_readonly_no_file(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: file does not exist
    cleanup_test_files();
    // When: open with READONLY
    err = iot_bsp_fs_open(BSP_FS_NO_SUCH_FILE, FS_READONLY, &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_NO_FILE);
}

void TC_iot_bsp_fs_open_readonly_existing(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    iot_bsp_fs_handle_t handle_w;
    UNUSED(state);

    // Given: existing file
    cleanup_test_files();
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &handle_w);
    iot_bsp_fs_close(handle_w);
    // When: open in READONLY
    err = iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READONLY, &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    iot_bsp_fs_close(handle);
    cleanup_test_files();
}

void TC_iot_bsp_fs_open_invalid_path(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // When: opening a path inside a non-existent directory
    err = iot_bsp_fs_open("/nonexistent_dir_xyz/file", FS_READWRITE, &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_OPEN_FAIL);
}

void TC_iot_bsp_fs_open_empty_filename_readonly(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // When: empty filename in READONLY mode
    err = iot_bsp_fs_open("", FS_READONLY, &handle);
    // Then: posix returns FS_NO_FILE since access fails
    assert_int_equal(err, IOT_ERROR_FS_NO_FILE);
}

void TC_iot_bsp_fs_open_empty_filename_readwrite(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // When: empty filename for READWRITE -> open() will fail
    err = iot_bsp_fs_open("", FS_READWRITE, &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_OPEN_FAIL);
}

void TC_iot_bsp_fs_open_from_stnv_no_file(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: file does not exist
    cleanup_test_files();
    // When
    err = iot_bsp_fs_open_from_stnv(BSP_FS_NO_SUCH_FILE, &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_NO_FILE);
}

void TC_iot_bsp_fs_open_from_stnv_success(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    iot_bsp_fs_handle_t handle_w;
    UNUSED(state);

    // Given: existing file
    cleanup_test_files();
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &handle_w);
    iot_bsp_fs_close(handle_w);
    // When
    err = iot_bsp_fs_open_from_stnv(BSP_FS_TEST_FILE, &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    iot_bsp_fs_close(handle);
    cleanup_test_files();
}

void TC_iot_bsp_fs_open_from_stnv_empty_filename(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // When: empty filename
    err = iot_bsp_fs_open_from_stnv("", &handle);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_NO_FILE);
}

void TC_iot_bsp_fs_write_and_read_success(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t w_handle;
    iot_bsp_fs_handle_t r_handle;
    const char *write_data = "hello-bsp-fs";
    char read_buf[64] = {0};
    size_t length;
    UNUSED(state);

    // Given: clean and open file in RW
    cleanup_test_files();
    err = iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &w_handle);
    assert_int_equal(err, IOT_ERROR_NONE);
    // When: write then read
    err = iot_bsp_fs_write(w_handle, write_data, strlen(write_data));
    assert_int_equal(err, IOT_ERROR_NONE);
    iot_bsp_fs_close(w_handle);

    err = iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READONLY, &r_handle);
    assert_int_equal(err, IOT_ERROR_NONE);
    length = sizeof(read_buf) - 1;
    err = iot_bsp_fs_read(r_handle, read_buf, &length);
    // Then: data matches
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_int_equal(length, strlen(write_data));
    assert_string_equal(read_buf, write_data);
    iot_bsp_fs_close(r_handle);
    cleanup_test_files();
}

void TC_iot_bsp_fs_write_failure_invalid_fd(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: a closed/invalid file descriptor
    handle.fd = -1;
    snprintf(handle.filename, sizeof(handle.filename), "/tmp/whatever");
    // When
    err = iot_bsp_fs_write(handle, "data", 4);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_WRITE_FAIL);
}

void TC_iot_bsp_fs_write_zero_length(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: open file
    cleanup_test_files();
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &handle);
    // When: zero-length write (length matches return 0)
    err = iot_bsp_fs_write(handle, "", 0);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    iot_bsp_fs_close(handle);
    cleanup_test_files();
}

void TC_iot_bsp_fs_read_no_file(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    char buf[16];
    size_t len = sizeof(buf);
    UNUSED(state);

    // Given: handle pointing at non-existent file
    handle.fd = -1;
    snprintf(handle.filename, sizeof(handle.filename), "%s", BSP_FS_NO_SUCH_FILE);
    cleanup_test_files();
    // When
    err = iot_bsp_fs_read(handle, buf, &len);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_NO_FILE);
}

void TC_iot_bsp_fs_read_invalid_fd(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t w_handle;
    iot_bsp_fs_handle_t bad_handle;
    char buf[16] = {0};
    size_t len;
    UNUSED(state);

    // Given: file exists, but fd is invalid in the handle
    cleanup_test_files();
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &w_handle);
    iot_bsp_fs_close(w_handle);
    bad_handle.fd = -1;
    snprintf(bad_handle.filename, sizeof(bad_handle.filename), "%s", BSP_FS_TEST_FILE);
    len = sizeof(buf);
    // When
    err = iot_bsp_fs_read(bad_handle, buf, &len);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_READ_FAIL);
    cleanup_test_files();
}

void TC_iot_bsp_fs_read_partial_data(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t w_handle;
    iot_bsp_fs_handle_t r_handle;
    const char *write_data = "abc";
    char buf[16] = {0};
    size_t len;
    UNUSED(state);

    // Given: file with short content; request larger buffer
    cleanup_test_files();
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &w_handle);
    iot_bsp_fs_write(w_handle, write_data, strlen(write_data));
    iot_bsp_fs_close(w_handle);
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READONLY, &r_handle);
    len = sizeof(buf);
    // When
    err = iot_bsp_fs_read(r_handle, buf, &len);
    // Then: actual length is short, NUL terminated by implementation
    assert_int_equal(err, IOT_ERROR_NONE);
    assert_int_equal(len, strlen(write_data));
    assert_string_equal(buf, write_data);
    iot_bsp_fs_close(r_handle);
    cleanup_test_files();
}

void TC_iot_bsp_fs_close_invalid_fd(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: invalid fd
    handle.fd = -1;
    handle.filename[0] = '\0';
    // When
    err = iot_bsp_fs_close(handle);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_CLOSE_FAIL);
}

void TC_iot_bsp_fs_close_double_close(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: a successfully opened file then closed
    cleanup_test_files();
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &handle);
    err = iot_bsp_fs_close(handle);
    assert_int_equal(err, IOT_ERROR_NONE);
    // When: closing the same fd a second time
    err = iot_bsp_fs_close(handle);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_CLOSE_FAIL);
    cleanup_test_files();
}

void TC_iot_bsp_fs_remove_no_file(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // Given: file does not exist
    cleanup_test_files();
    // When
    err = iot_bsp_fs_remove(BSP_FS_NO_SUCH_FILE);
    // Then
    assert_int_equal(err, IOT_ERROR_FS_NO_FILE);
}

void TC_iot_bsp_fs_remove_invalid_path(void **state)
{
    iot_error_t err;
    UNUSED(state);

    // When: empty filename
    err = iot_bsp_fs_remove("");
    // Then
    assert_int_equal(err, IOT_ERROR_FS_NO_FILE);
}

void TC_iot_bsp_fs_remove_success(void **state)
{
    iot_error_t err;
    iot_bsp_fs_handle_t handle;
    UNUSED(state);

    // Given: an existing file
    cleanup_test_files();
    iot_bsp_fs_open(BSP_FS_TEST_FILE, FS_READWRITE, &handle);
    iot_bsp_fs_close(handle);
    // When
    err = iot_bsp_fs_remove(BSP_FS_TEST_FILE);
    // Then
    assert_int_equal(err, IOT_ERROR_NONE);
    cleanup_test_files();
}
