/* clang-format off */
/* clang-format gets confused by the #define bool line, and gives crazy indenting */
#ifndef AWS_COMMON_STDBOOL_H
#define AWS_COMMON_STDBOOL_H

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

#ifndef NO_STDBOOL
#    include <stdbool.h> /* NOLINT(fuchsia-restrict-system-includes) */
#else
#    ifndef __cplusplus
#        define bool _Bool
#        define true 1
#        define false 0
#    elif defined(__GNUC__) && !defined(__STRICT_ANSI__)
#        define _Bool bool
#        if __cplusplus < 201103L
/* For C++98, define bool, false, true as a GNU extension. */
#            define bool bool
#            define false false
#            define true true
#        endif /* __cplusplus < 201103L */
#    endif     /* __cplusplus */
#endif         /* NO_STDBOOL */

#endif /* AWS_COMMON_STDBOOL_H */
/* clang-format on */
