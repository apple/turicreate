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

/*
 * MSVC wants us to use the non-portable _dupenv_s instead; since we need
 * to remain portable, tell MSVC to suppress this warning.
 */
#define _CRT_SECURE_NO_WARNINGS

/* for _may_i_use_cpu_feature */
#include <immintrin.h>

#ifdef HAVE_MSVC_CPUIDEX
/* for __cpuidex */
#    include <intrin.h>
#endif

#include <aws/common/common.h>
#include <stdlib.h>

#define CPUID_UNKNOWN 2
#define CPUID_AVAILABLE 0
#define CPUID_UNAVAILABLE 1
static int cpuid_state = 2;

#ifndef HAVE_BUILTIN_CPU_SUPPORTS
#    ifdef HAVE_MSVC_CPUIDEX
static bool msvc_check_avx2(void) {
    int cpuInfo[4];

    /* Check maximum supported function */
    __cpuidex(cpuInfo, 0, 0);

    if (cpuInfo[0] < 7) {
        /*
         * AVX2 support bit is in function 7, so if function 7 is not
         * supported, we know we have no AVX2 support
         */
        return false;
    }

    /* CPUID: Extended features */
    __cpuidex(cpuInfo, 7, 0);

    /* EBX bit 5: AVX2 support */
    return cpuInfo[1] & (1 << 5);
}
#    endif
#endif

bool aws_common_private_has_avx2(void) {
    if (AWS_LIKELY(cpuid_state == 0)) {
        return true;
    }
    if (AWS_LIKELY(cpuid_state == 1)) {
        return false;
    }

    /* Provide a hook for testing fallbacks and benchmarking */
    const char *env_avx2_enabled = getenv("AWS_COMMON_AVX2");
    if (env_avx2_enabled) {
        int is_enabled = atoi(env_avx2_enabled);
        cpuid_state = !is_enabled;
        return is_enabled;
    }

#ifdef HAVE_BUILTIN_CPU_SUPPORTS
    bool available = __builtin_cpu_supports("avx2");
#elif defined(HAVE_MAY_I_USE)
    bool available = _may_i_use_cpu_feature(_FEATURE_AVX2);
#elif defined(HAVE_MSVC_CPUIDEX)
    bool available = msvc_check_avx2();
#else
#    error No CPUID probe mechanism available
#endif
    cpuid_state = available ? CPUID_AVAILABLE : CPUID_UNAVAILABLE;

    return available;
}
