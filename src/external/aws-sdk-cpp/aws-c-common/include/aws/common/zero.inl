#ifndef AWS_COMMON_ZERO_INL
#define AWS_COMMON_ZERO_INL

/*
 * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/common/stdbool.h>
#include <aws/common/stdint.h>
#include <aws/common/zero.h>
#include <string.h>

AWS_EXTERN_C_BEGIN
/**
 * Returns whether each byte is zero.
 */
AWS_STATIC_IMPL
bool aws_is_mem_zeroed(const void *buf, size_t bufsize) {
    /* Optimization idea: vectorized instructions to check more than 64 bits at a time. */

    /* Check 64 bits at a time */
    const uint64_t *buf_u64 = (const uint64_t *)buf;
    const size_t num_u64_checks = bufsize / 8;
    size_t i;
    for (i = 0; i < num_u64_checks; ++i) {
        if (buf_u64[i]) {
            return false;
        }
    }

    /* Update buf to where u64 checks left off */
    buf = buf_u64 + num_u64_checks;
    bufsize = bufsize % 8;

    /* Check 8 bits at a time */
    const uint8_t *buf_u8 = (const uint8_t *)buf;
    for (i = 0; i < bufsize; ++i) {
        if (buf_u8[i]) {
            return false;
        }
    }

    return true;
}

AWS_EXTERN_C_END

#endif /* AWS_COMMON_ZERO_INL */
