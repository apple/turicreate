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

#include <aws/common/math.h>
#include <stdarg.h>

AWS_COMMON_API int aws_add_size_checked_varargs(size_t num, size_t *r, ...) {
    va_list argp;
    va_start(argp, r);

    size_t accum = 0;
    for (size_t i = 0; i < num; ++i) {
        size_t next = va_arg(argp, size_t);
        if (aws_add_size_checked(accum, next, &accum) == AWS_OP_ERR) {
            va_end(argp);
            return AWS_OP_ERR;
        }
    }
    *r = accum;
    va_end(argp);
    return AWS_OP_SUCCESS;
}
