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

#include <aws/common/mutex.h>

#include <Windows.h>

int aws_mutex_init(struct aws_mutex *mutex) {
    /* Ensure our mutex and Windows' mutex are the same size */
    AWS_STATIC_ASSERT(sizeof(SRWLOCK) == sizeof(mutex->mutex_handle));

    InitializeSRWLock(AWSMUTEX_TO_WINDOWS(mutex));
    mutex->initialized = true;
    return AWS_OP_SUCCESS;
}

/* turn off unused named parameter warning on msvc.*/
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4100)
#endif

void aws_mutex_clean_up(struct aws_mutex *mutex) {
    AWS_PRECONDITION(mutex);
    AWS_ZERO_STRUCT(*mutex);
}

int aws_mutex_lock(struct aws_mutex *mutex) {
    AWS_PRECONDITION(mutex && mutex->initialized);
    AcquireSRWLockExclusive(AWSMUTEX_TO_WINDOWS(mutex));
    return AWS_OP_SUCCESS;
}

int aws_mutex_try_lock(struct aws_mutex *mutex) {
    AWS_PRECONDITION(mutex && mutex->initialized);
    BOOL res = TryAcquireSRWLockExclusive(AWSMUTEX_TO_WINDOWS(mutex));

    if (!res) {
        return AWS_OP_SUCCESS;
    }

    return aws_raise_error(AWS_ERROR_MUTEX_TIMEOUT);
}

int aws_mutex_unlock(struct aws_mutex *mutex) {
    AWS_PRECONDITION(mutex && mutex->initialized);
    ReleaseSRWLockExclusive(AWSMUTEX_TO_WINDOWS(mutex));
    return AWS_OP_SUCCESS;
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif
