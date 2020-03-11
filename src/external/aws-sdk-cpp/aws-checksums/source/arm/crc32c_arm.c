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

/* for the moment, fallback to SW on ARM until MSFT adds intrensics for ARM v8.1+ */
#if (defined(_M_ARM) || defined(__arm__) || defined(__aarch64__) || defined(__ARM_ARCH_ISA_A64))

#    include <aws/checksums/private/crc_priv.h>

uint32_t aws_checksums_crc32c_hw(const uint8_t *data, int length, uint32_t previousCrc32) {
    return aws_checksums_crc32c_sw(data, length, previousCrc32);
}

#endif
