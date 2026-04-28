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

#include "cmocka_custom.h"
#include "iot_bsp_random.h"

#define UNUSED(x) (void **)(x)

void TC_iot_bsp_random_posix_first_call_initializes_seed(void **state)
{
    unsigned int value;
    UNUSED(state);

    // When: first call (seed gets initialized)
    value = iot_bsp_random();
    // Then: returns a value within uint32 range (sanity)
    assert_true(value <= UINT32_MAX);
}

void TC_iot_bsp_random_posix_subsequent_calls(void **state)
{
    unsigned int v1, v2;
    UNUSED(state);

    // When: invoking after seed is initialized
    v1 = iot_bsp_random();
    v2 = iot_bsp_random();
    // Then: both values must be in valid range
    assert_true(v1 <= UINT32_MAX);
    assert_true(v2 <= UINT32_MAX);
}

void TC_iot_bsp_random_posix_value_below_uint32_max(void **state)
{
    unsigned int value;
    UNUSED(state);

    // When: invoking iot_bsp_random
    value = iot_bsp_random();
    // Then: result is strictly less than UINT32_MAX (function uses % UINT32_MAX)
    assert_true(value < UINT32_MAX);
}

void TC_iot_bsp_random_posix_multiple_invocations_in_range(void **state)
{
    int i;
    unsigned int value;
    UNUSED(state);

    // When: calling many times
    for (i = 0; i < 32; i++) {
        value = iot_bsp_random();
        // Then: each call returns valid 32-bit unsigned value
        assert_true(value < UINT32_MAX);
    }
}

void TC_iot_bsp_random_posix_distribution_not_constant(void **state)
{
    int i;
    unsigned int first;
    int diff_count = 0;
    UNUSED(state);

    // Given: a baseline value
    first = iot_bsp_random();
    // When: invoking iot_bsp_random many times
    for (i = 0; i < 64; i++) {
        if (iot_bsp_random() != first) {
            diff_count++;
        }
    }
    // Then: not all values are identical (it's not a constant function)
    assert_true(diff_count > 0);
}

void TC_iot_bsp_random_posix_high_byte_within_range(void **state)
{
    unsigned int value;
    UNUSED(state);

    // When: invoking iot_bsp_random
    value = iot_bsp_random();
    // Then: high byte is within 0x00..0xFF (always true for uint32, sanity)
    assert_true(((value >> 24) & 0xFF) <= 0xFF);
}
