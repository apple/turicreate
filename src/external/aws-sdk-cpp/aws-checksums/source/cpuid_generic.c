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

#if (!defined(_M_ARM) && !defined(__arm__) && !defined(__ARM_ARCH_ISA_A64)) &&                                         \
    ((!defined(__x86_64__) && !defined(_M_X64) && !defined(_M_IX86)) || (defined(__GNUC__) && defined(DEBUG_BUILD)))

/* clang-format is being dumb and wrong, this can't be const or the symbols won't match */
/* NOLINTNEXTLINE(readability-non-const-parameter) */
int aws_checksums_do_cpu_id(int32_t *cpuid) {
    (void)cpuid;
    return 0;
}

#endif

static int32_t s_cpuid = 0;

static void do_check(void) {
    if (!s_cpuid) {
        aws_checksums_do_cpu_id(&s_cpuid);
    }
}

/** Returns non-zero if the CPU supports the PCLMULQDQ instruction. */
int aws_checksums_is_clmul_present(void) {
    do_check();
    return s_cpuid & 0x00000002;
}

/** Returns non-zero if the CPU supports SSE4.1 instructions. */
int aws_checksums_is_sse41_present(void) {
    do_check();
    return s_cpuid & 0x00080000;
}

/** Returns non-zero if the CPU supports SSE4.2 instructions (i.e. CRC32). */
int aws_checksums_is_sse42_present(void) {
    do_check();
    return s_cpuid & 0x00100000;
}
