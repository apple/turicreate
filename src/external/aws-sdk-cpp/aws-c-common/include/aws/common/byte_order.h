#ifndef AWS_COMMON_BYTE_ORDER_H
#define AWS_COMMON_BYTE_ORDER_H

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

AWS_EXTERN_C_BEGIN

/**
 * Returns 1 if machine is big endian, 0 if little endian.
 * If you compile with even -O1 optimization, this check is completely optimized
 * out at compile time and code which calls "if (aws_is_big_endian())" will do
 * the right thing without branching.
 */
AWS_STATIC_IMPL int aws_is_big_endian(void);
/**
 * Convert 64 bit integer from host to network byte order.
 */
AWS_STATIC_IMPL uint64_t aws_hton64(uint64_t x);
/**
 * Convert 64 bit integer from network to host byte order.
 */
AWS_STATIC_IMPL uint64_t aws_ntoh64(uint64_t x);

/**
 * Convert 32 bit integer from host to network byte order.
 */
AWS_STATIC_IMPL uint32_t aws_hton32(uint32_t x);

/**
 * Convert 32 bit float from host to network byte order.
 */
AWS_STATIC_IMPL float aws_htonf32(float x);

/**
 * Convert 64 bit double from host to network byte order.
 */
AWS_STATIC_IMPL double aws_htonf64(double x);

/**
 * Convert 32 bit integer from network to host byte order.
 */
AWS_STATIC_IMPL uint32_t aws_ntoh32(uint32_t x);

/**
 * Convert 32 bit float from network to host byte order.
 */
AWS_STATIC_IMPL float aws_ntohf32(float x);
/**
 * Convert 32 bit float from network to host byte order.
 */
AWS_STATIC_IMPL double aws_ntohf64(double x);

/**
 * Convert 16 bit integer from host to network byte order.
 */
AWS_STATIC_IMPL uint16_t aws_hton16(uint16_t x);

/**
 * Convert 16 bit integer from network to host byte order.
 */
AWS_STATIC_IMPL uint16_t aws_ntoh16(uint16_t x);

#ifndef AWS_NO_STATIC_IMPL
#    include <aws/common/byte_order.inl>
#endif /* AWS_NO_STATIC_IMPL */

AWS_EXTERN_C_END

#endif /* AWS_COMMON_BYTE_ORDER_H */
