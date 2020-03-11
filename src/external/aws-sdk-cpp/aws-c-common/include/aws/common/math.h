#ifndef AWS_COMMON_MATH_H
#define AWS_COMMON_MATH_H

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

#include <aws/common/common.h>
#include <aws/common/config.h>

#include <limits.h>
#include <stdlib.h>

/* The number of bits in a size_t variable */
#if SIZE_MAX == UINT32_MAX
#    define SIZE_BITS 32
#elif SIZE_MAX == UINT64_MAX
#    define SIZE_BITS 64
#else
#    error "Target not supported"
#endif

/* The largest power of two that can be stored in a size_t */
#define SIZE_MAX_POWER_OF_TWO (((size_t)1) << (SIZE_BITS - 1))

AWS_EXTERN_C_BEGIN

#if defined(AWS_HAVE_GCC_OVERFLOW_MATH_EXTENSIONS) && (defined(__clang__) || !defined(__cplusplus)) ||                 \
    defined(__x86_64__) && defined(AWS_HAVE_GCC_INLINE_ASM) || defined(AWS_HAVE_MSVC_MULX) || defined(CBMC) ||         \
    !defined(AWS_HAVE_GCC_OVERFLOW_MATH_EXTENSIONS)
/* In all these cases, we can use fast static inline versions of this code */
#    define AWS_COMMON_MATH_API AWS_STATIC_IMPL
#else
/*
 * We got here because we are building in C++ mode but we only support overflow extensions
 * in C mode. Because the fallback is _slow_ (involving a division), we'd prefer to make a
 * non-inline call to the fast C intrinsics.
 */
#    define AWS_COMMON_MATH_API AWS_COMMON_API
#endif

/**
 * Multiplies a * b. If the result overflows, returns 2^64 - 1.
 */
AWS_COMMON_MATH_API uint64_t aws_mul_u64_saturating(uint64_t a, uint64_t b);

/**
 * If a * b overflows, returns AWS_OP_ERR; otherwise multiplies
 * a * b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_COMMON_MATH_API int aws_mul_u64_checked(uint64_t a, uint64_t b, uint64_t *r);

/**
 * Multiplies a * b. If the result overflows, returns 2^32 - 1.
 */
AWS_COMMON_MATH_API uint32_t aws_mul_u32_saturating(uint32_t a, uint32_t b);

/**
 * If a * b overflows, returns AWS_OP_ERR; otherwise multiplies
 * a * b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_COMMON_MATH_API int aws_mul_u32_checked(uint32_t a, uint32_t b, uint32_t *r);

/**
 * Adds a + b.  If the result overflows returns 2^64 - 1.
 */
AWS_COMMON_MATH_API uint64_t aws_add_u64_saturating(uint64_t a, uint64_t b);

/**
 * If a + b overflows, returns AWS_OP_ERR; otherwise adds
 * a + b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_COMMON_MATH_API int aws_add_u64_checked(uint64_t a, uint64_t b, uint64_t *r);

/**
 * Adds a + b. If the result overflows returns 2^32 - 1.
 */
AWS_COMMON_MATH_API uint32_t aws_add_u32_saturating(uint32_t a, uint32_t b);

/**
 * If a + b overflows, returns AWS_OP_ERR; otherwise adds
 * a + b, returns the result in *r, and returns AWS_OP_SUCCESS.
 */
AWS_COMMON_MATH_API int aws_add_u32_checked(uint32_t a, uint32_t b, uint32_t *r);

/**
 * Multiplies a * b. If the result overflows, returns SIZE_MAX.
 */
AWS_STATIC_IMPL size_t aws_mul_size_saturating(size_t a, size_t b);

/**
 * Multiplies a * b and returns the result in *r. If the result
 * overflows, returns AWS_OP_ERR; otherwise returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_mul_size_checked(size_t a, size_t b, size_t *r);

/**
 * Adds a + b.  If the result overflows returns SIZE_MAX.
 */
AWS_STATIC_IMPL size_t aws_add_size_saturating(size_t a, size_t b);

/**
 * Adds a + b and returns the result in *r. If the result
 * overflows, returns AWS_OP_ERR; otherwise returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_add_size_checked(size_t a, size_t b, size_t *r);

/**
 * Adds [num] arguments (expected to be of size_t), and returns the result in *r.
 * If the result overflows, returns AWS_OP_ERR; otherwise returns AWS_OP_SUCCESS.
 */
AWS_COMMON_API int aws_add_size_checked_varargs(size_t num, size_t *r, ...);

/**
 * Function to check if x is power of 2
 */
AWS_STATIC_IMPL bool aws_is_power_of_two(const size_t x);
/**
 * Function to find the smallest result that is power of 2 >= n. Returns AWS_OP_ERR if this cannot
 * be done without overflow
 */
AWS_STATIC_IMPL int aws_round_up_to_power_of_two(size_t n, size_t *result);

#ifndef AWS_NO_STATIC_IMPL
#    include <aws/common/math.inl>
#endif /* AWS_NO_STATIC_IMPL */

AWS_EXTERN_C_END

#endif /* AWS_COMMON_MATH_H */
