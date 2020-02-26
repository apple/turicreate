/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_od_serialization

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <cstdio>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <toolkits/object_detection/od_serialization.hpp>

namespace turi {
namespace object_detection {
namespace {

constexpr size_t NUM_ANCHOR_BOXES = 15;

BOOST_AUTO_TEST_CASE(test_init_darknet_yolo) {
  neural_net::model_spec nn_spec;
  const size_t num_classes = 10;
  init_darknet_yolo(nn_spec, num_classes, NUM_ANCHOR_BOXES, "image");

  const CoreML::Specification::NeuralNetwork& nn = nn_spec.get_coreml_spec();
  TS_ASSERT_EQUALS(nn.layers_size(), 32);
  /* we totally should have 9 conv layers
   * (conv_0 -> conv_8), 8 batch normalization layer (batch_0 -> batch_7),
   * 6 max pooling layers (pool_0 -> pool_5), and 8 relu layers (leakyrelu_0 ->
   * leakyrelu_7) so totally 31 layers.
   */
  int layer_num = 1;  // Skip the scale layer at the beginning.
  int num_features = 3;
  const std::map<int, int> layer_num_to_channels = {
      {0, 16},  {1, 32},  {2, 64},   {3, 128},
      {4, 256}, {5, 512}, {6, 1024}, {7, 1024}};
  for (auto const& x : layer_num_to_channels) {
    const auto& convlayer_ = nn.layers(layer_num);
    TS_ASSERT(convlayer_.has_convolution());
    TS_ASSERT_EQUALS(convlayer_.name(),
                     "conv" + std::to_string(x.first) + "_fwd");
    TS_ASSERT_EQUALS(convlayer_.convolution().outputchannels(), x.second);
    TS_ASSERT_EQUALS(convlayer_.convolution().kernelchannels(), num_features);
    TS_ASSERT_EQUALS(convlayer_.convolution().stride(0), 1);
    TS_ASSERT_EQUALS(convlayer_.convolution().stride(1), 1);
    TS_ASSERT_EQUALS(convlayer_.convolution().kernelsize(0), 3);
    TS_ASSERT_EQUALS(convlayer_.convolution().kernelsize(1), 3);
    TS_ASSERT(convlayer_.convolution().has_same());

    const auto& batchnormlayer_ = nn.layers(layer_num + 1);
    TS_ASSERT(batchnormlayer_.has_batchnorm());
    TS_ASSERT_EQUALS(batchnormlayer_.name(),
                     "batchnorm" + std::to_string(x.first) + "_fwd");
    TS_ASSERT_EQUALS(batchnormlayer_.batchnorm().channels(), x.second);
    TS_ASSERT_EQUALS(batchnormlayer_.batchnorm().epsilon(), 0.00001f);

    const auto& relulayer_ = nn.layers(layer_num + 2);
    TS_ASSERT(relulayer_.has_activation());
    TS_ASSERT_EQUALS(relulayer_.name(),
                     "leakyrelu" + std::to_string(x.first) + "_fwd");
    TS_ASSERT_EQUALS(relulayer_.activation().leakyrelu().alpha(), 0.1f);

    // since the yolo model only has max pooling layers for layer 0 - 5
    int num_max_pooling_layers_in_yolo = 5;
    if (x.first <= num_max_pooling_layers_in_yolo) {
      const auto& poolinglayer_ = nn.layers(layer_num + 3);
      TS_ASSERT(poolinglayer_.has_pooling());
      TS_ASSERT_EQUALS(poolinglayer_.name(),
                       "pool" + std::to_string(x.first) + "_fwd");
      TS_ASSERT_EQUALS(poolinglayer_.pooling().kernelsize(0), 2);
      TS_ASSERT_EQUALS(poolinglayer_.pooling().kernelsize(1), 2);
      if (x.first == num_max_pooling_layers_in_yolo) {
        TS_ASSERT_EQUALS(poolinglayer_.pooling().stride(0), 1);
        TS_ASSERT_EQUALS(poolinglayer_.pooling().stride(1), 1);
        TS_ASSERT(poolinglayer_.pooling().has_same());
        TS_ASSERT(poolinglayer_.pooling().avgpoolexcludepadding());
      } else {
        TS_ASSERT_EQUALS(poolinglayer_.pooling().stride(0), 2);
        TS_ASSERT_EQUALS(poolinglayer_.pooling().stride(1), 2);
        TS_ASSERT(poolinglayer_.pooling().has_valid());
        TS_ASSERT(poolinglayer_.pooling().valid().has_paddingamounts());
        TS_ASSERT_EQUALS(poolinglayer_.pooling()
                             .valid()
                             .paddingamounts()
                             .borderamounts_size(),
                         2);
      }
      layer_num = layer_num + 4;
    } else {
      layer_num = layer_num + 3;
    }
    num_features = x.second;
  }
  // add check for the last layer
  const auto& convlayer_ = nn.layers(layer_num);
  TS_ASSERT(convlayer_.has_convolution());
  TS_ASSERT_EQUALS(convlayer_.name(),
                     "conv8_fwd");
  const size_t num_predictions =
      5 + num_classes;  // Per anchor box, including predict classes and
                        // bounding box regression
  const size_t conv8_c_out = NUM_ANCHOR_BOXES * num_predictions;
  TS_ASSERT_EQUALS(convlayer_.convolution().outputchannels(), conv8_c_out);
  TS_ASSERT_EQUALS(convlayer_.convolution().kernelchannels(), num_features);
  TS_ASSERT_EQUALS(convlayer_.convolution().stride(0), 1);
  TS_ASSERT_EQUALS(convlayer_.convolution().stride(1), 1);
  TS_ASSERT_EQUALS(convlayer_.convolution().kernelsize(0), 1);
  TS_ASSERT_EQUALS(convlayer_.convolution().kernelsize(1), 1);
  TS_ASSERT(convlayer_.convolution().has_same());
}

BOOST_AUTO_TEST_CASE(test_save_load) {
  // Create Test Model
  neural_net::model_spec nn_spec_1;
  std::map<std::string, variant_type> state1 = {
      {"num_classes", 10}, {"model", "darknet_yolo"}, {"max_iterations", 5}};
  init_darknet_yolo(nn_spec_1,
                    variant_get_value<size_t>(state1.at("num_classes")),
                    NUM_ANCHOR_BOXES, "image");

  // Save it
  dir_archive archive_write;
  archive_write.open_directory_for_write("serialized_save_load_tests");
  turi::oarchive oarc(archive_write);
  _save_impl(oarc, state1, nn_spec_1.export_params_view());
  archive_write.close();

  // Load it
  dir_archive archive_read;
  archive_read.open_directory_for_read("serialized_save_load_tests");
  turi::iarchive iarc(archive_read);
  size_t version = 1;
  neural_net::float_array_map weights;
  neural_net::model_spec nn_spec_2;
  std::map<std::string, variant_type> state2 = {
      {"num_classes", 10}, {"model", "darknet_yolo"}, {"max_iterations", 5}};
  _load_version(iarc, version, &state2, &weights);
  archive_read.close();
  init_darknet_yolo(nn_spec_2,
                    variant_get_value<size_t>(state2.at("num_classes")),
                    NUM_ANCHOR_BOXES, "image");
  nn_spec_2.update_params(weights);

  // Compare saved and loaded models
  const CoreML::Specification::NeuralNetwork& nn_saved =
      nn_spec_1.get_coreml_spec();
  const CoreML::Specification::NeuralNetwork& nn_loaded =
      nn_spec_2.get_coreml_spec();
  TS_ASSERT(nn_saved.SerializeAsString() == nn_loaded.SerializeAsString());
}

}  // namespace
}  // namespace object_detection
}  // namespace turi
