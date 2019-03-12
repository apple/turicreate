/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/bitops.hpp>
#include <util/test_macros.hpp>

using namespace turi;

BOOST_AUTO_TEST_CASE(test_bit_mask) {
  uint128_t mask[256];

  mask[0] = 0x00000000;
  mask[1] = 0x00000001;
  mask[2] = 0x00000003;
  mask[3] = 0x00000007;
  mask[4] = 0x0000000f;
  mask[5] = 0x0000001f;
  mask[6] = 0x0000003f;
  mask[7] = 0x0000007f;
  mask[8] = 0x000000ff;
  mask[9] = 0x000001ff;
  mask[10] = 0x000003ff;
  mask[11] = 0x000007ff;
  mask[12] = 0x00000fff;
  mask[13] = 0x00001fff;
  mask[14] = 0x00003fff;
  mask[15] = 0x00007fff;
  mask[16] = 0x0000ffff;
  mask[17] = 0x0001ffff;
  mask[18] = 0x0003ffff;
  mask[19] = 0x0007ffff;
  mask[20] = 0x000fffff;
  mask[21] = 0x001fffff;
  mask[22] = 0x003fffff;
  mask[23] = 0x007fffff;
  mask[24] = 0x00ffffff;
  mask[25] = 0x01ffffff;
  mask[26] = 0x03ffffff;
  mask[27] = 0x07ffffff;
  mask[28] = 0x0fffffff;
  mask[29] = 0x1fffffff;
  mask[30] = 0x3fffffff;
  mask[31] = 0x7fffffff;
  mask[32] = 0xffffffff;

  for (size_t i = 0; i < 32; ++i) {
    mask[32 + i] = (mask[i] << 32) | 0xffffffff;
  }

  for (size_t i = 0; i < 64; ++i) {
    mask[64 + i] = ~uint64_t(0) | (mask[i] << 64);
  }

  for (size_t i = 0; i < 128; ++i) {
    mask[128 + i] = ~uint128_t(0);
  }

  // The tests

  for (size_t i = 0; i < 256; ++i) {
    TS_ASSERT(bit_mask<uint128_t>(i) == mask[i]);
  }

  for (size_t i = 0; i < 256; ++i) {
    TS_ASSERT_EQUALS(bit_mask<uint64_t>(i), uint64_t(mask[i] & ~uint64_t(0)));
  }

  for (size_t i = 0; i < 256; ++i) {
    TS_ASSERT_EQUALS(bit_mask<uint32_t>(i), uint32_t(mask[i] & ~uint32_t(0)));
  }

  for (size_t i = 0; i < 256; ++i) {
  TS_ASSERT_EQUALS(bit_mask<uint16_t>(i), uint16_t(mask[i] & 0xfffful));
  }

  for (size_t i = 0; i < 256; ++i) {
  TS_ASSERT_EQUALS(bit_mask<uint8_t>(i), uint8_t(mask[i] & 0xff));
  }
}
