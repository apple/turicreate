/*
 * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/checksums/private/cpuid.h>
#include <stdint.h>

#if defined(__x86_64__) && !(defined(__GNUC__) && defined(DEBUG_BUILD))

static int32_t s_cpuid_output = 0;
static int s_cpuid_ran = 0;

int aws_checksums_do_cpu_id(int32_t *cpuid) {

    if (!s_cpuid_ran) {

        asm volatile("XOR    %%rax, %%rax    # zero the eax register\n"
                     "INC    %%eax           # eax=1 for processor feature bits\n"
                     "CPUID                  #get feature bits\n"
                     : "=c"(s_cpuid_output)
                     : // none
                     : "%rax", "%rbx", "%rdx", "cc");
        s_cpuid_ran = 1;
    }

    *cpuid = s_cpuid_output;
    return 1;
}

#endif /* defined(__x86_64__) && !(defined(__GNUC__) && defined(DEBUG_BUILD)) */
