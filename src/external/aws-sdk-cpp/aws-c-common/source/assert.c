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

#include <aws/common/system_info.h>

#include <stdio.h>
#include <stdlib.h>

void aws_fatal_assert(const char *cond_str, const char *file, int line) {
    aws_debug_break();
    fprintf(stderr, "Fatal error condition occurred in %s:%d: %s\nExiting Application\n", file, line, cond_str);
    aws_backtrace_print(stderr, NULL);
    abort();
}
