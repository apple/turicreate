/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>

#ifdef HAS_MACOS_10_15

#include <string>
#include <sstream>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "style_transfer_utils.hpp"

/**
  GOLDEN SET
 
  These values were generated using MxNet as the reference framework. A golden
  set was generated from this reference framework and exported to JSON. Theses
  generated values are used to make sure that the TCMPS implemetation hasn't
  diverged.
*/

#include <data/encode/test_1/config.h>
#include <data/encode/test_1/inputs.h>
#include <data/encode/test_1/outputs.h>
#include <data/encode/test_1/weights.h>

#include <data/residual/test_1/config.h>
#include <data/residual/test_1/inputs.h>
#include <data/residual/test_1/outputs.h>
#include <data/residual/test_1/weights.h>

#include <data/decode/test_1/config.h>
#include <data/decode/test_1/inputs.h>
#include <data/decode/test_1/outputs.h>
#include <data/decode/test_1/weights.h>

#include <data/resnet/test_1/config.h>
#include <data/resnet/test_1/inputs.h>
#include <data/resnet/test_1/outputs.h>
#include <data/resnet/test_1/weights.h>

#include <data/block1/test_1/config.h>
#include <data/block1/test_1/inputs.h>
#include <data/block1/test_1/outputs.h>
#include <data/block1/test_1/weights.h>

#include <data/block2/test_1/config.h>
#include <data/block2/test_1/inputs.h>
#include <data/block2/test_1/outputs.h>
#include <data/block2/test_1/weights.h>

using boost::lexical_cast;
using boost::property_tree::ptree;
using boost::property_tree::read_json;

using neural_net_test::style_transfer::EncodingTest;
using neural_net_test::style_transfer::ResidualTest;
using neural_net_test::style_transfer::DecodingTest;
using neural_net_test::style_transfer::ResnetTest;
using neural_net_test::style_transfer::Block1Test;
using neural_net_test::style_transfer::Block2Test;

/**
  Parses the char array for json and puts it into a `boost::property::ptree`

  @param data   - The character array containing the sequence of bytes
                  representing the json
  
  @param length - The length of the character array

  @return - `boost::property::ptree` of the json contained in the `data` param.
*/
ptree extract_json(unsigned char data[], unsigned int length) {
  std::string data_string(data, data + length);
  std::stringstream data_stream(data_string);

  ptree data_json;
  read_json(data_stream, data_json);

  return data_json;
}

// Contains the tests for the Style Transfer TCMPS Implementation
struct style_transfer_test {
public:
  /**
    Test Encoding
    
    This correctness test checks whether the Style Transfer Encoding layer 
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_encoding() {
    ptree config_json = extract_json(data_encode_test_1_config_json, data_encode_test_1_config_json_len);
    ptree inputs_json = extract_json(data_encode_test_1_inputs_json, data_encode_test_1_inputs_json_len);
    ptree outputs_json = extract_json(data_encode_test_1_outputs_json, data_encode_test_1_outputs_json_len);
    ptree weights_json = extract_json(data_encode_test_1_weights_json, data_encode_test_1_weights_json_len);

    EncodingTest encoding_test(config_json, weights_json);
    
    TS_ASSERT(encoding_test.check_predict(inputs_json, outputs_json));
  }

  /**
    Test Residual
    
    This correctness test checks whether the Style Transfer Residual layer
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_residual() {
    ptree config_json = extract_json(data_residual_test_1_config_json, data_residual_test_1_config_json_len);
    ptree inputs_json = extract_json(data_residual_test_1_inputs_json, data_residual_test_1_inputs_json_len);
    ptree outputs_json = extract_json(data_residual_test_1_outputs_json, data_residual_test_1_outputs_json_len);
    ptree weights_json = extract_json(data_residual_test_1_weights_json, data_residual_test_1_weights_json_len);

    ResidualTest residual_test(config_json, weights_json);
    TS_ASSERT(residual_test.check_predict(inputs_json, outputs_json));
  }

  /**
    Test Decoding
    
    This correctness test checks whether the Style Transfer Decoding layer
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_decode() {
    ptree config_json = extract_json(data_decode_test_1_config_json, data_decode_test_1_config_json_len);
    ptree inputs_json = extract_json(data_decode_test_1_inputs_json, data_decode_test_1_inputs_json_len);
    ptree outputs_json = extract_json(data_decode_test_1_outputs_json, data_decode_test_1_outputs_json_len);
    ptree weights_json = extract_json(data_decode_test_1_weights_json, data_decode_test_1_weights_json_len);

    DecodingTest decoding_test(config_json, weights_json);
    TS_ASSERT(decoding_test.check_predict(inputs_json, outputs_json));
  }

  /**
    Test Resnet
    
    This correctness test checks whether the Style Transfer Transformer network
    present in TCMPS is performing as intended by comparing it to the golden
    set.
   */
  void test_resnet() {
    ptree config_json = extract_json(data_resnet_test_1_config_json, data_resnet_test_1_config_json_len);
    ptree inputs_json = extract_json(data_resnet_test_1_inputs_json, data_resnet_test_1_inputs_json_len);
    ptree outputs_json = extract_json(data_resnet_test_1_outputs_json, data_resnet_test_1_outputs_json_len);
    ptree weights_json = extract_json(data_resnet_test_1_weights_json, data_resnet_test_1_weights_json_len);

    ResnetTest resnet_test(config_json, weights_json);
    TS_ASSERT(resnet_test.check_predict(inputs_json, outputs_json));
  }

  /**
    Test Block 1
    
    This correctness test checks whether the VGG Block 1 Layer present in TCMPS
    is performing as intended by comparing it to the golden set.
   */
  void test_block_1() {
    ptree config_json = extract_json(data_block1_test_1_config_json, data_block1_test_1_config_json_len);
    ptree inputs_json = extract_json(data_block1_test_1_inputs_json, data_block1_test_1_inputs_json_len);
    ptree outputs_json = extract_json(data_block1_test_1_outputs_json, data_block1_test_1_outputs_json_len);
    ptree weights_json = extract_json(data_block1_test_1_weights_json, data_block1_test_1_weights_json_len);

    Block1Test block_1_test(config_json, weights_json);
    TS_ASSERT(block_1_test.check_predict(inputs_json, outputs_json));
  }

  /**
    Test Block 2
    
    This correctness test checks whether the VGG Block 2 Layer present in TCMPS
    is performing as intended by comparing it to the golden set.
   */
  void test_block_2() {
    ptree config_json = extract_json(data_block2_test_1_config_json, data_block2_test_1_config_json_len);
    ptree inputs_json = extract_json(data_block2_test_1_inputs_json, data_block2_test_1_inputs_json_len);
    ptree outputs_json = extract_json(data_block2_test_1_outputs_json, data_block2_test_1_outputs_json_len);
    ptree weights_json = extract_json(data_block2_test_1_weights_json, data_block2_test_1_weights_json_len);

    Block2Test block_2_test(config_json, weights_json);
    TS_ASSERT(block_2_test.check_predict(inputs_json, outputs_json));
  }

  // TODO: write the test
  void test_vgg16() {
    TS_ASSERT(true); 
  }

  // TODO: write the test
  void test_loss() {
    TS_ASSERT(true); 
  }

  // TODO: write the test
  void test_weight_update() {
    TS_ASSERT(true); 
  }
};

#endif

BOOST_FIXTURE_TEST_SUITE(_style_transfer_test, style_transfer_test)

#ifdef HAS_MACOS_10_15

BOOST_AUTO_TEST_CASE(test_encoding) {
  style_transfer_test::test_encoding();
}

BOOST_AUTO_TEST_CASE(test_residual) {
  style_transfer_test::test_residual();
}

BOOST_AUTO_TEST_CASE(test_decode) {
  style_transfer_test::test_decode();
}

BOOST_AUTO_TEST_CASE(test_resnet) {
  style_transfer_test::test_resnet();
}

BOOST_AUTO_TEST_CASE(test_block_1) {
  style_transfer_test::test_block_1();
}

BOOST_AUTO_TEST_CASE(test_block_2) {
  style_transfer_test::test_block_2();
}

BOOST_AUTO_TEST_CASE(test_vgg16) {
  style_transfer_test::test_vgg16();
}

BOOST_AUTO_TEST_CASE(test_loss) {
  style_transfer_test::test_loss();
}

BOOST_AUTO_TEST_CASE(test_weight_update) {
  style_transfer_test::test_weight_update();
}

#endif

BOOST_AUTO_TEST_SUITE_END()