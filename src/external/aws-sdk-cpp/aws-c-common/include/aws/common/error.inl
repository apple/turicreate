#ifndef AWS_COMMON_ERROR_INL
#define AWS_COMMON_ERROR_INL

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

#include <aws/common/error.h>

AWS_EXTERN_C_BEGIN

/*
 * Raises `err` to the installed callbacks, and sets the thread's error.
 */
AWS_STATIC_IMPL
int aws_raise_error(int err) {
    /*
     * Certain static analyzers can't see through the out-of-line call to aws_raise_error,
     * and assume that this might return AWS_OP_SUCCESS. We'll put the return inline just
     * to help with their assumptions.
     */

    aws_raise_error_private(err);

    return AWS_OP_ERR;
}

AWS_EXTERN_C_END

#endif /* AWS_COMMON_ERROR_INL */
