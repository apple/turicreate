#ifndef AWS_COMMON_RW_LOCK_H
#define AWS_COMMON_RW_LOCK_H

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

#include <aws/common/common.h>
#ifdef _WIN32
/* NOTE: Do not use this macro before including Windows.h */
#    define AWSSRW_TO_WINDOWS(pCV) (PSRWLOCK) pCV
#else
#    include <pthread.h>
#endif

struct aws_rw_lock {
#ifdef _WIN32
    void *lock_handle;
#else
    pthread_rwlock_t lock_handle;
#endif
};

#ifdef _WIN32
#    define AWS_RW_LOCK_INIT                                                                                           \
        { .lock_handle = NULL }
#else
#    define AWS_RW_LOCK_INIT                                                                                           \
        { .lock_handle = PTHREAD_RWLOCK_INITIALIZER }
#endif

AWS_EXTERN_C_BEGIN

/**
 * Initializes a new platform instance of mutex.
 */
AWS_COMMON_API int aws_rw_lock_init(struct aws_rw_lock *lock);

/**
 * Cleans up internal resources.
 */
AWS_COMMON_API void aws_rw_lock_clean_up(struct aws_rw_lock *lock);

/**
 * Blocks until it acquires the lock. While on some platforms such as Windows,
 * this may behave as a reentrant mutex, you should not treat it like one. On
 * platforms it is possible for it to be non-reentrant, it will be.
 */
AWS_COMMON_API int aws_rw_lock_rlock(struct aws_rw_lock *lock);
AWS_COMMON_API int aws_rw_lock_wlock(struct aws_rw_lock *lock);

/**
 * Attempts to acquire the lock but returns immediately if it can not.
 * While on some platforms such as Windows, this may behave as a reentrant mutex,
 * you should not treat it like one. On platforms it is possible for it to be non-reentrant, it will be.
 */
AWS_COMMON_API int aws_rw_lock_try_rlock(struct aws_rw_lock *lock);
AWS_COMMON_API int aws_rw_lock_try_wlock(struct aws_rw_lock *lock);

/**
 * Releases the lock.
 */
AWS_COMMON_API int aws_rw_lock_runlock(struct aws_rw_lock *lock);
AWS_COMMON_API int aws_rw_lock_wunlock(struct aws_rw_lock *lock);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_RW_LOCK_H */
