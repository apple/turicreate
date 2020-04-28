#ifndef AWS_COMMON_MATH_INL
#define AWS_COMMON_MATH_INL

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

#include <aws/common/common.h>
#include <aws/common/config.h>
#include <aws/common/math.h>

#include <limits.h>
#include <stdlib.h>

AWS_EXTERN_C_BEGIN

#if defined(AWS_HAVE_GCC_OVERFLOW_MATH_EXTENSIONS) && (defined(__clang__) || !defined(__cplusplus))
/*
 * GCC and clang have these super convenient overflow checking builtins...
 * but (in the case of GCC) they're only available when building C source.
 * We'll fall back to one of the other inlinable variants (or a non-inlined version)
 * if we are building this header on G++.
 */
#    include <aws/common/math.gcc_overflow.inl>
#elif defined(__x86_64__) && defined(AWS_HAVE_GCC_INLINE_ASM)
#    include <aws/common/math.gcc_x64_asm.inl>
#elif defined(AWS_HAVE_MSVC_MULX)
#    include <aws/common/math.msvc.inl>
#elif defined(CBMC)
#    include <aws/common/math.cbmc.inl>
#else
#    ifndef AWS_HAVE_GCC_OVERFLOW_MATH_EXTENSIONS
/* Fall back to the pure-C implementations */
#        include <aws/common/math.fallback.inl>
#    else
/*
 * We got here because we are building in C++ mode but we only support overflow extensions
 * in C mode. Because the fallback is _slow_ (involving a division), we'd prefer to make a
 * non-inline call to the fast C intrinsics.
 */
#    endif /*  AWS_HAVE_GCC_OVERFLOW_MATH_EXTENSIONS */
#endif     /*  defined(AWS_HAVE_GCC_OVERFLOW_MATH_EXTENSIONS) && (defined(__clang__) || !defined(__cplusplus)) */

#if _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4127) /*Disable "conditional expression is constant" */
#endif                              /* _MSC_VER */

/**
 * Multiplies a * b. If the result overflows, returns SIZE_MAX.
 */
AWS_STATIC_IMPL size_t aws_mul_size_saturating(size_t a, size_t b) {
#if SIZE_BITS == 32
    return (size_t)aws_mul_u32_saturating(a, b);
#elif SIZE_BITS == 64
    return (size_t)aws_mul_u64_saturating(a, b);
#else
#    error "Target not supported"
#endif
}

/**
 * Multiplies a * b and returns the result in *r. If the result
 * overflows, returns AWS_OP_ERR; otherwise returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_mul_size_checked(size_t a, size_t b, size_t *r) {
#if SIZE_BITS == 32
    return aws_mul_u32_checked(a, b, (uint32_t *)r);
#elif SIZE_BITS == 64
    return aws_mul_u64_checked(a, b, (uint64_t *)r);
#else
#    error "Target not supported"
#endif
}

/**
 * Adds a + b.  If the result overflows returns SIZE_MAX.
 */
AWS_STATIC_IMPL size_t aws_add_size_saturating(size_t a, size_t b) {
#if SIZE_BITS == 32
    return (size_t)aws_add_u32_saturating(a, b);
#elif SIZE_BITS == 64
    return (size_t)aws_add_u64_saturating(a, b);
#else
#    error "Target not supported"
#endif
}

/**
 * Adds a + b and returns the result in *r. If the result
 * overflows, returns AWS_OP_ERR; otherwise returns AWS_OP_SUCCESS.
 */
AWS_STATIC_IMPL int aws_add_size_checked(size_t a, size_t b, size_t *r) {
#if SIZE_BITS == 32
    return aws_add_u32_checked(a, b, (uint32_t *)r);
#elif SIZE_BITS == 64
    return aws_add_u64_checked(a, b, (uint64_t *)r);
#else
#    error "Target not supported"
#endif
}

/**
 * Function to check if x is power of 2
 */
AWS_STATIC_IMPL bool aws_is_power_of_two(const size_t x) {
    /* First x in the below expression is for the case when x is 0 */
    return x && (!(x & (x - 1)));
}

/**
 * Function to find the smallest result that is power of 2 >= n. Returns AWS_OP_ERR if this cannot
 * be done without overflow
 */
AWS_STATIC_IMPL int aws_round_up_to_power_of_two(size_t n, size_t *result) {
    if (n == 0) {
        *result = 1;
        return AWS_OP_SUCCESS;
    }
    if (n > SIZE_MAX_POWER_OF_TWO) {
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_BITS == 64
    n |= n >> 32;
#endif
    n++;
    *result = n;
    return AWS_OP_SUCCESS;
}

#if _MSC_VER
#    pragma warning(pop)
#endif /* _MSC_VER */

AWS_EXTERN_C_END

#endif /* AWS_COMMON_MATH_INL */
