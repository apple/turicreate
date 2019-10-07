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
    Pre Processing

    This correctness test checks whether the Pre Processing Node present in
    TCMPS is performing as intended by comparing it to the golden set.
   */
  void test_pre_processing() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Gram Matrix

    This test checks whether the Gram Matrix Node defined in TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_gram_matrix() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Content Loss

    This test checks whether the Content Loss defined in TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_content_loss() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Style Loss

    This test checks whether the Style Loss defined in TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_style_loss() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Total Loss

    This test checks whether the Total Loss defined in TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_total_loss() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Weight Update

    This test checks whether the Weight Update defined in Objective-C TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_weight_update() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Weight Update

    This test checks whether the Weight Update defined in TCMPS is
    performing as intended by comparing it to the golden set.
   */
  void test_tcmps_weight_update() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Loss Computation

    This test checks whether the Loss defined in TCMPS is
    performing as intended by comparing it to the golden set for a couple of
    iterations.
   */
  void test_tcmps_style_tansfer() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Loss Computation

    This test checks whether the Loss defined in the C++ wrapper of TCMPS is
    performing as intended by comparing it to the golden set for a couple of
    iterations.
   */
  void test_model_base_tcmps() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Weight Update

    This test checks whether the Weight Update defined in the C++ wrapper of
    TCMPS is performing as intended by comparing it to the golden set for a
    couple of iterations.
   */
  void test_model_base_tcmps_weight_update() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Forward Inference

    This test checks whether the forward inference defined in the C++ wrapper of
    TCMPS is performing as intended by comparing it to the golden set for a
    couple of iterations.
   */
  void test_model_base_tcmps_forward_inference() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Multiple Content Images

    This test checks whether the training defined in the C++ wrapper of TCMPS is
    performing as intended by comparing it to the golden set for a couple of
    iterations. Only one style image tested.
   */
  void test_model_base_tcmps_multiple_content() {
    TS_ASSERT(true);  // TODO
  }

  /**
    Multiple Style Images

    This test checks whether the training defined in the C++ wrapper of TCMPS is
    performing as intended by comparing it to the golden set for a couple of
    iterations. Multiple style images tested for training.
   */
  void test_model_base_tcmps_multiple_style() {
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

BOOST_AUTO_TEST_CASE(test_pre_processing) {
  style_transfer_test::test_pre_processing();
}

BOOST_AUTO_TEST_CASE(test_gram_matrix) {
  style_transfer_test::test_gram_matrix();
}

BOOST_AUTO_TEST_CASE(test_content_loss) {
  style_transfer_test::test_content_loss();
}

BOOST_AUTO_TEST_CASE(test_style_loss) {
  style_transfer_test::test_style_loss();
}

BOOST_AUTO_TEST_CASE(test_total_loss) {
  style_transfer_test::test_total_loss();
}

BOOST_AUTO_TEST_CASE(test_weight_update) {
  style_transfer_test::test_weight_update();
}

BOOST_AUTO_TEST_CASE(test_tcmps_weight_update) {
  style_transfer_test::test_tcmps_weight_update();
}

BOOST_AUTO_TEST_CASE(test_tcmps_style_tansfer) {
  style_transfer_test::test_tcmps_style_tansfer();
}

BOOST_AUTO_TEST_CASE(test_model_base_tcmps) {
  style_transfer_test::test_model_base_tcmps();
}

BOOST_AUTO_TEST_CASE(test_model_base_tcmps_weight_update) {
  style_transfer_test::test_model_base_tcmps_weight_update();
}

BOOST_AUTO_TEST_CASE(test_model_base_tcmps_forward_inference) {
  style_transfer_test::test_model_base_tcmps_forward_inference();
}

BOOST_AUTO_TEST_CASE(test_model_base_tcmps_multiple_content) {
  style_transfer_test::test_model_base_tcmps_multiple_content();
}

BOOST_AUTO_TEST_CASE(test_model_base_tcmps_multiple_style) {
  style_transfer_test::test_model_base_tcmps_multiple_style();
}

#endif

BOOST_AUTO_TEST_SUITE_END()