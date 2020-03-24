#ifndef AWS_COMMON_DEVICE_RANDOM_H
#define AWS_COMMON_DEVICE_RANDOM_H
/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/common/common.h>

struct aws_byte_buf;

AWS_EXTERN_C_BEGIN

/**
 * Get an unpredictably random 64bit number, suitable for cryptographic use.
 */
AWS_COMMON_API int aws_device_random_u64(uint64_t *output);

/**
 * Get an unpredictably random 32bit number, suitable for cryptographic use.
 */
AWS_COMMON_API int aws_device_random_u32(uint32_t *output);

/**
 * Get an unpredictably random 16bit number, suitable for cryptographic use.
 */
AWS_COMMON_API int aws_device_random_u16(uint16_t *output);

/**
 * Get an unpredictably random 8bit number, suitable for cryptographic use.
 */
AWS_COMMON_API int aws_device_random_u8(uint8_t *output);

/**
 * Fill a buffer with unpredictably random bytes, suitable for cryptographic use.
 */
AWS_COMMON_API int aws_device_random_buffer(struct aws_byte_buf *output);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_DEVICE_RANDOM_H */
