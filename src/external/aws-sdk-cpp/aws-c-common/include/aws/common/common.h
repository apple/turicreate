#ifndef AWS_COMMON_COMMON_H
#define AWS_COMMON_COMMON_H

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

#include <aws/common/config.h>
#include <aws/common/exports.h>

#include <aws/common/allocator.h>
#include <aws/common/assert.h>
#include <aws/common/error.h>
#include <aws/common/macros.h>
#include <aws/common/predicates.h>
#include <aws/common/stdbool.h>
#include <aws/common/stdint.h>
#include <aws/common/zero.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h> /* for abort() */
#include <string.h>

AWS_EXTERN_C_BEGIN

/**
 * Initializes internal datastructures used by aws-c-common.
 * Must be called before using any functionality in aws-c-common.
 */
AWS_COMMON_API
void aws_common_library_init(struct aws_allocator *allocator);

/**
 * Shuts down the internal datastructures used by aws-c-common.
 */
AWS_COMMON_API
void aws_common_library_clean_up(void);

AWS_COMMON_API
void aws_common_fatal_assert_library_initialized(void);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_COMMON_H */
