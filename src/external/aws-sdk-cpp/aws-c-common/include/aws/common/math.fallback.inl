#ifndef AWS_COMMON_MATH_FALLBACK_INL
#define AWS_COMMON_MATH_FALLBACK_INL

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

/*
 * This header is already included, but include it again to make editor
 * highlighting happier.
 */
#include <aws/common/common.h>
#include <aws/common/math.h>

AWS_EXTERN_C_BEGIN
/**
 * Multiplies a * b. If the result overflows, returns 2^64 - 1.
 */
AWS_STATIC_IMPL uint64_t aws_mul_u64_saturating(uint64_t a, uint64_t b) {
    if (a > 0 && b > 0 && a > (UINT64_MAX / b))
        return UINT64_MAX;
    return a * b;
}

/**
 * If a * b overflows, returns AWS_OP_ERR; otherwise multiplies
 * a * b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_mul_u64_checked(uint64_t a, uint64_t b, uint64_t *r) {
    if (a > 0 && b > 0 && a > (UINT64_MAX / b))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a * b;
    return AWS_OP_SUCCESS;
}

/**
 * Multiplies a * b. If the result overflows, returns 2^32 - 1.
 */
AWS_STATIC_IMPL uint32_t aws_mul_u32_saturating(uint32_t a, uint32_t b) {
    if (a > 0 && b > 0 && a > (UINT32_MAX / b))
        return UINT32_MAX;
    return a * b;
}

/**
 * If a * b overflows, returns AWS_OP_ERR; otherwise multiplies
 * a * b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_mul_u32_checked(uint32_t a, uint32_t b, uint32_t *r) {
    if (a > 0 && b > 0 && a > (UINT32_MAX / b))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a * b;
    return AWS_OP_SUCCESS;
}

/**
 * Adds a + b.  If the result overflows returns 2^64 - 1.
 */
AWS_STATIC_IMPL uint64_t aws_add_u64_saturating(uint64_t a, uint64_t b) {
    if ((b > 0) && (a > (UINT64_MAX - b)))
        return UINT64_MAX;
    return a + b;
}

/**
 * If a + b overflows, returns AWS_OP_ERR; otherwise adds
 * a + b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_add_u64_checked(uint64_t a, uint64_t b, uint64_t *r) {
    if ((b > 0) && (a > (UINT64_MAX - b)))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a + b;
    return AWS_OP_SUCCESS;
}

/**
 * Adds a + b. If the result overflows returns 2^32 - 1.
 */
AWS_STATIC_IMPL uint32_t aws_add_u32_saturating(uint32_t a, uint32_t b) {
    if ((b > 0) && (a > (UINT32_MAX - b)))
        return UINT32_MAX;
    return a + b;
}

/**
 * If a + b overflows, returns AWS_OP_ERR; otherwise adds
 * a + b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_add_u32_checked(uint32_t a, uint32_t b, uint32_t *r) {
    if ((b > 0) && (a > (UINT32_MAX - b)))
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    *r = a + b;
    return AWS_OP_SUCCESS;
}

AWS_EXTERN_C_END

#endif /*  AWS_COMMON_MATH_FALLBACK_INL */
