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

#include <aws/checksums/private/crc_priv.h>
#include <intrin.h>

#if defined(_M_X64) || defined(_M_IX86)

#    if defined(_M_X64)
typedef uint64_t *slice_ptr_type;
typedef uint64_t slice_ptr_int_type;
#    else
typedef uint32_t *slice_ptr_type;
typedef uint32_t slice_ptr_int_type;
#    endif

/**
 * This implements crc32c via the intel sse 4.2 instructions.
 *  This is separate from the straight asm version, because visual c does not allow
 *  inline assembly for x64.
 */
uint32_t aws_checksums_crc32c_hw(const uint8_t *data, int length, uint32_t previousCrc32) {
    uint32_t crc = ~previousCrc32;
    int length_to_process = length;

    slice_ptr_type temp = (slice_ptr_type)data;

    /*to eek good performance out of the intel implementation, we need to only hit the hardware
      once we are aligned on the byte boundaries we are using. So, peel off a byte at a time until we are
      8 byte aligned (64 bit arch) or 4 byte aligned (32 bit arch)

      first calculate how many bytes we need to burn before we are aligned.
      for a 64 bit arch this is:
      (8 - <how far we are past a boundary>) mod 8
      32 bit:
      (4 - <how far we are past a boundary>) mod 4 */
    uint8_t alignment_offset = (sizeof(slice_ptr_int_type) - ((slice_ptr_int_type)temp % sizeof(slice_ptr_int_type))) %
                               sizeof(slice_ptr_int_type);

    /*for every byte we need to burn off, just do them a byte at a time.
      increment the temp pointer by one byte at a time until we get it on an alignment boundary */
    while (alignment_offset != 0 && length_to_process) {
        uint8_t *byte_pos = (uint8_t *)temp;
        crc = (uint32_t)_mm_crc32_u8(crc, *byte_pos++);
        temp = (slice_ptr_type)byte_pos;
        --alignment_offset;
        --length_to_process;
    }

    /*now whatever is left is properly aligned on a boundary*/
    uint32_t slices = length_to_process / sizeof(temp);
    uint32_t remainder = length_to_process % sizeof(temp);

    while (slices--) {
#    if defined(_M_X64)
        crc = (uint32_t)_mm_crc32_u64(crc, *temp++);
#    else
        crc = _mm_crc32_u32(crc, *temp++);
#    endif
    }

    /* process the remaining parts that can't be done on the slice size. */
    uint8_t *remainderPos = (uint8_t *)temp;

    while (remainder--) {
        crc = (uint32_t)_mm_crc32_u8(crc, *remainderPos++);
    }

    return ~crc;
}

#endif /* x64 || x86 */
