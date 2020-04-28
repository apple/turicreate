#ifndef AWS_COMMON_ENCODING_INL
#define AWS_COMMON_ENCODING_INL

/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/common/byte_buf.h>
#include <aws/common/byte_order.h>
#include <aws/common/common.h>
#include <aws/common/encoding.h>

AWS_EXTERN_C_BEGIN

/* Add a 64 bit unsigned integer to the buffer, ensuring network - byte order
 * Assumes the buffer size is at least 8 bytes.
 */
AWS_STATIC_IMPL void aws_write_u64(uint64_t value, uint8_t *buffer) {
    value = aws_hton64(value);

    memcpy((void *)buffer, &value, sizeof(value));
}

/*
 * Extracts a 64 bit unsigned integer from buffer. Ensures conversion from
 * network byte order to host byte order. Assumes buffer size is at least 8
 * bytes.
 */
AWS_STATIC_IMPL uint64_t aws_read_u64(const uint8_t *buffer) {
    uint64_t value = 0;
    memcpy((void *)&value, (void *)buffer, sizeof(value));

    return aws_ntoh64(value);
}

/* Add a 32 bit unsigned integer to the buffer, ensuring network - byte order
 * Assumes the buffer size is at least 4 bytes.
 */
AWS_STATIC_IMPL void aws_write_u32(uint32_t value, uint8_t *buffer) {
    value = aws_hton32(value);

    memcpy((void *)buffer, (void *)&value, sizeof(value));
}

/*
 * Extracts a 32 bit unsigned integer from buffer. Ensures conversion from
 * network byte order to host byte order. Assumes the buffer size is at least 4
 * bytes.
 */
AWS_STATIC_IMPL uint32_t aws_read_u32(const uint8_t *buffer) {
    uint32_t value = 0;
    memcpy((void *)&value, (void *)buffer, sizeof(value));

    return aws_ntoh32(value);
}

/* Add a 24 bit unsigned integer to the buffer, ensuring network - byte order
 * return the new position in the buffer for the next operation.
 * Note, since this uses uint32_t for storage, the 3 least significant bytes
 * will be used. Assumes buffer is at least 3 bytes long.
 */
AWS_STATIC_IMPL void aws_write_u24(uint32_t value, uint8_t *buffer) {
    value = aws_hton32(value);
    memcpy((void *)buffer, (void *)((uint8_t *)&value + 1), sizeof(value) - 1);
}

/*
 * Extracts a 24 bit unsigned integer from buffer. Ensures conversion from
 * network byte order to host byte order. Assumes buffer is at least 3 bytes
 * long.
 */
AWS_STATIC_IMPL uint32_t aws_read_u24(const uint8_t *buffer) {
    uint32_t value = 0;
    memcpy((void *)((uint8_t *)&value + 1), (void *)buffer, sizeof(value) - 1);

    return aws_ntoh32(value);
}

/* Add a 16 bit unsigned integer to the buffer, ensuring network-byte order
 * return the new position in the buffer for the next operation.
 * Assumes buffer is at least 2 bytes long.
 */
AWS_STATIC_IMPL void aws_write_u16(uint16_t value, uint8_t *buffer) {
    value = aws_hton16(value);

    memcpy((void *)buffer, (void *)&value, sizeof(value));
}

/*
 * Extracts a 16 bit unsigned integer from buffer. Ensures conversion from
 * network byte order to host byte order. Assumes buffer is at least 2 bytes
 * long.
 */
AWS_STATIC_IMPL uint16_t aws_read_u16(const uint8_t *buffer) {
    uint16_t value = 0;
    memcpy((void *)&value, (void *)buffer, sizeof(value));

    return aws_ntoh16(value);
}

AWS_EXTERN_C_END

#endif /*  AWS_COMMON_ENCODING_INL */
