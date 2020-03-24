#ifndef AWS_COMMON_PREDICATES_H
#define AWS_COMMON_PREDICATES_H
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

/**
 * Returns whether all bytes of the two byte arrays match.
 */
#if (AWS_DEEP_CHECKS == 1)
#    ifdef CBMC
/* clang-format off */
#        define AWS_BYTES_EQ(arr1, arr2, len)                                                                              \
            __CPROVER_forall {                                                                                             \
                int i;                                                                                                     \
                (i >= 0 && i < len) ==> ((const uint8_t *)&arr1)[i] == ((const uint8_t *)&arr2)[i]                         \
            }
/* clang-format on */
#    else
#        define AWS_BYTES_EQ(arr1, arr2, len) (memcmp(arr1, arr2, len) == 0)
#    endif /* CBMC */
#else
#    define AWS_BYTES_EQ(arr1, arr2, len) (1)
#endif /* (AWS_DEEP_CHECKS == 1) */

#endif /* AWS_COMMON_PREDICATES_H */
