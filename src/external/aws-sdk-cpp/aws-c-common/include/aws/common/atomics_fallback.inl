#ifndef AWS_COMMON_ATOMICS_FALLBACK_INL
#define AWS_COMMON_ATOMICS_FALLBACK_INL

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

AWS_EXTERN_C_BEGIN

#ifndef AWS_ATOMICS_HAVE_THREAD_FENCE

void aws_atomic_thread_fence(enum aws_memory_order order) {
    struct aws_atomic_var var;
    aws_atomic_int_t expected = 0;

    aws_atomic_store_int(&var, expected, aws_memory_order_relaxed);
    aws_atomic_compare_exchange_int(&var, &expected, 1, order, aws_memory_order_relaxed);
}

#endif /* AWS_ATOMICS_HAVE_THREAD_FENCE */

AWS_EXTERN_C_END
#endif /* AWS_COMMON_ATOMICS_FALLBACK_INL */
