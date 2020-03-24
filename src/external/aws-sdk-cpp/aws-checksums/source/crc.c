/*
 * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <aws/checksums/crc.h>

#include <aws/checksums/private/cpuid.h>
#include <aws/checksums/private/crc_priv.h>

static uint32_t (*s_crc32c_fn_ptr)(const uint8_t *input, int length, uint32_t previousCrc32) = 0;

uint32_t aws_checksums_crc32(const uint8_t *input, int length, uint32_t previousCrc32) {
    return aws_checksums_crc32_sw(input, length, previousCrc32);
}

uint32_t aws_checksums_crc32c(const uint8_t *input, int length, uint32_t previousCrc32) {
    if (!s_crc32c_fn_ptr) {
        if (aws_checksums_is_sse42_present()) {
            s_crc32c_fn_ptr = aws_checksums_crc32c_hw;
        } else {
            s_crc32c_fn_ptr = aws_checksums_crc32c_sw;
        }
    }
    return s_crc32c_fn_ptr(input, length, previousCrc32);
}
