#ifndef AWS_CHECKSUMS_PRIVATE_CPUID_H
#define AWS_CHECKSUMS_PRIVATE_CPUID_H
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
#include <stdint.h>

/***
 * runs cpu id and fills in capabilities for the current cpu architecture.
 * returns non zero on success, zero on failure. If the operation was successful
 * cpuid will be set with the bits from the cpuid call, otherwise they will be untouched.
 **/
int aws_checksums_do_cpu_id(int32_t *cpuid);

/** Returns non-zero if the CPU supports the PCLMULQDQ instruction. */
int aws_checksums_is_clmul_present(void);

/** Returns non-zero if the CPU supports SSE4.1 instructions. */
int aws_checksums_is_sse41_present(void);

/** Returns non-zero if the CPU supports SSE4.2 instructions (i.e. CRC32). */
int aws_checksums_is_sse42_present(void);

#endif /* AWS_CHECKSUMS_PRIVATE_CPUID_H */
