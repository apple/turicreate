/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>

#ifdef HAS_MACOS_10_15

struct style_transfer_test {
 public:
  /**
    Test Encoding

    This correctness test checks whether the Style Transfer Encoding layer
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_encoding() {
    TS_ASSERT(true);  // TODO
  }
  /**
    Test Residual

    This correctness test checks whether the Style Transfer Residual layer
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_residual() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Test Decoding

    This correctness test checks whether the Style Transfer Decoding layer
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_decode() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Test Resnet

    This correctness test checks whether the Style Transfer Transformer network
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_resnet() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Test Block 1

    This correctness test checks whether the VGG Block 1 Layer present in TCMPS
    is performing as intended by comparing it to the golden set.
   */
  void test_block_1() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Test Block 2

    This correctness test checks whether the VGG Block 2 Layer present in TCMPS
    is performing as intended by comparing it to the golden set.
   */
  void test_block_2() {
    TS_ASSERT(true);  // TODO
  }

  /**
    VGG 16

    This correctness test checks whether the VGG Model present in TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_vgg16() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Loss

    This test checks whether the Loss out of the total graph defined in TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_loss() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Backwards

    This test checks whether the Weight Update in the total graph defined in
    TCMPS is performing as intended by comparing it to the golden set.
   */
  void test_backwards() {
    TS_ASSERT(true);  // TODO
  }
};

#endif

BOOST_FIXTURE_TEST_SUITE(_style_transfer_test, style_transfer_test)

#ifdef HAS_MACOS_10_15

BOOST_AUTO_TEST_CASE(test_encoding) { style_transfer_test::test_encoding(); }

BOOST_AUTO_TEST_CASE(test_residual) { style_transfer_test::test_residual(); }

BOOST_AUTO_TEST_CASE(test_decode) { style_transfer_test::test_decode(); }

BOOST_AUTO_TEST_CASE(test_resnet) { style_transfer_test::test_resnet(); }

BOOST_AUTO_TEST_CASE(test_block_1) { style_transfer_test::test_block_1(); }

BOOST_AUTO_TEST_CASE(test_block_2) { style_transfer_test::test_block_2(); }

BOOST_AUTO_TEST_CASE(test_vgg16) { style_transfer_test::test_vgg16(); }

BOOST_AUTO_TEST_CASE(test_loss) { style_transfer_test::test_loss(); }

BOOST_AUTO_TEST_CASE(test_backwards) { style_transfer_test::test_backwards(); }

#endif

BOOST_AUTO_TEST_SUITE_END()