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

#ifndef AWS_COMMON_POSIX_COMMON_INL
#define AWS_COMMON_POSIX_COMMON_INL

#include <aws/common/common.h>

#include <errno.h>

AWS_EXTERN_C_BEGIN

static inline int aws_private_convert_and_raise_error_code(int error_code) {
    switch (error_code) {
        case 0:
            return AWS_OP_SUCCESS;
        case EINVAL:
            return aws_raise_error(AWS_ERROR_MUTEX_NOT_INIT);
        case EBUSY:
            return aws_raise_error(AWS_ERROR_MUTEX_TIMEOUT);
        case EPERM:
            return aws_raise_error(AWS_ERROR_MUTEX_CALLER_NOT_OWNER);
        case ENOMEM:
            return aws_raise_error(AWS_ERROR_OOM);
        case EDEADLK:
            return aws_raise_error(AWS_ERROR_THREAD_DEADLOCK_DETECTED);
        default:
            return aws_raise_error(AWS_ERROR_MUTEX_FAILED);
    }
}

AWS_EXTERN_C_END

#endif /* AWS_COMMON_POSIX_COMMON_INL */
