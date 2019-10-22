/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_dc_serialization

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <cstdio>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <toolkits/drawing_classifier/drawing_classifier.hpp>

namespace turi {
namespace drawing_classifer {
namespace {

class drawing_classifier_mock
    : public turi::drawing_classifier::drawing_classifier {
 public:
  std::unique_ptr<neural_net::model_spec> my_init_model() {
    return init_model();
  }
};

BOOST_AUTO_TEST_CASE(test_dc_init_model) {
  // states
  constexpr unsigned int num_classes = 10;
  const std::string target = "target";
  const std::vector<std::string> features = {"0", "1"};

  // init_model
  drawing_classifier_mock dc;
  dc.add_or_update_state(
      {{"target", target},
       {"num_classes", num_classes},
       {"features", flex_list(features.begin(), features.end())}});

  auto nn_spec = dc.my_init_model();

  const CoreML::Specification::NeuralNetwork& nn = nn_spec->get_coreml_spec();
  /*
   * 1 input,
   * 3 (conv layers, relu, maxpool), 1 flatten, 2 dense, 1 softmax
   * in total: 14 layers
   */
  TS_ASSERT_EQUALS(nn.layers_size(), 14);

  /* layer 0: concat layer */
  {
    auto concat_layer = nn.layers(0);
    TS_ASSERT(concat_layer.has_concat());
    TS_ASSERT(concat_layer.output_size() == 1);
    TS_ASSERT_EQUALS(concat_layer.output(0), "features");
    TS_ASSERT_EQUALS(concat_layer.name(), "features");
    for (int ii = 0; ii < concat_layer.input_size(); ii++) {
      TS_ASSERT_EQUALS(concat_layer.input(ii), features.at(ii));
    }
  }

  {
    const std::map<int, int> layer_num_to_channels = {
        {0, 16}, {1, 32}, {2, 64}};
    for (auto& x : layer_num_to_channels) {
      int layer_index = x.first * 3 + 1;
      const auto& convlayer = nn.layers(layer_index);
      TS_ASSERT(convlayer.has_convolution());
      TS_ASSERT_EQUALS(convlayer.name(),
                       "drawing_conv" + std::to_string(x.first) + "_fwd");
      TS_ASSERT_EQUALS(convlayer.convolution().outputchannels(), x.second);
      TS_ASSERT_EQUALS(convlayer.convolution().kernelchannels(),
                       x.first == 0 ? 1 : x.second / 2);
      TS_ASSERT_EQUALS(convlayer.convolution().stride(0), 1);
      TS_ASSERT_EQUALS(convlayer.convolution().stride(1), 1);
      TS_ASSERT_EQUALS(convlayer.convolution().kernelsize(0), 3);
      TS_ASSERT_EQUALS(convlayer.convolution().kernelsize(1), 3);
      TS_ASSERT(convlayer.convolution().has_same());

      const auto& relu_layer = nn.layers(layer_index + 1);
      TS_ASSERT_EQUALS(relu_layer.name(),
                       "drawing_relu" + std::to_string(x.first) + "_fwd");
      TS_ASSERT(relu_layer.has_activation());
      TS_ASSERT(relu_layer.activation().has_relu());

      const auto& pool_layer = nn.layers(layer_index + 2);
      TS_ASSERT(pool_layer.has_pooling());
      TS_ASSERT_EQUALS(pool_layer.name(),
                       "drawing_pool" + std::to_string(x.first) + "_fwd");
      TS_ASSERT_EQUALS(pool_layer.pooling().kernelsize(0), 2);
      TS_ASSERT_EQUALS(pool_layer.pooling().kernelsize(1), 2);
      TS_ASSERT_EQUALS(pool_layer.pooling().stride(0), 2);
      TS_ASSERT_EQUALS(pool_layer.pooling().stride(1), 2);
      TS_ASSERT(pool_layer.pooling().has_valid());
    }
  }

  unsigned layer_index = 10;
  {
    const auto& flatten_layer = nn.layers(layer_index);
    TS_ASSERT(flatten_layer.has_flatten());
    TS_ASSERT_EQUALS(flatten_layer.name(), "drawing_flatten0_fwd");
  }

  layer_index++;
  {
    const auto& dense_layer = nn.layers(layer_index);
    TS_ASSERT(dense_layer.has_innerproduct());
    TS_ASSERT_EQUALS(dense_layer.name(), "drawing_dense0_fwd");
    TS_ASSERT_EQUALS(dense_layer.innerproduct().inputchannels(), 64 * 3 * 3);
    TS_ASSERT_EQUALS(dense_layer.innerproduct().outputchannels(), 128);
  }

  layer_index++;
  {
    const auto& dense_layer = nn.layers(layer_index);
    TS_ASSERT_EQUALS(dense_layer.name(), "drawing_dense1_fwd");
    TS_ASSERT(dense_layer.has_innerproduct());
    TS_ASSERT_EQUALS(dense_layer.innerproduct().inputchannels(), 128);
    TS_ASSERT_EQUALS(dense_layer.innerproduct().outputchannels(), num_classes);
  }

  layer_index++;
  TS_ASSERT(layer_index == 13);
  {
    const auto& softmax_layer = nn.layers(layer_index);
    TS_ASSERT(softmax_layer.has_softmax());
    TS_ASSERT_EQUALS(softmax_layer.output(0), target + "Probability");
  }
}

BOOST_AUTO_TEST_CASE(test_export_coreml) {
  // minimum startup code
  const std::string target = "target";
  const std::vector<std::string> features = {"0", "1"};
  const std::vector<std::string> labels = {"0", "1"};

  turi::drawing_classifier::drawing_classifier dc;
  dc.add_or_update_state(
      {{"target", target},
       {"num_classes", labels.size()},
       {"classes", flex_list(labels.begin(), labels.end())},
       {"max_iterations", 300},
       {"warm_start", false},
       {"features", flex_list(features.begin(), features.end())}});

  auto ml_model_wrapper = dc.export_to_coreml("", /* debug no throw */ true);
  TS_ASSERT(ml_model_wrapper != nullptr);

  const auto& my_model_spec = ml_model_wrapper->coreml_model()->getProto();
  TS_ASSERT_EQUALS(my_model_spec.specificationversion(), 1);

  auto& my_model_desc = my_model_spec.description();
  TS_ASSERT_EQUALS(my_model_desc.input_size(), features.size());

  // test input image type
  TS_ASSERT_EQUALS(my_model_desc.input(0).name(), "image");
  TS_ASSERT(my_model_desc.input(0).type().has_imagetype());

  auto input_feature_type = my_model_desc.input(0).type().imagetype();
  TS_ASSERT_EQUALS(input_feature_type.colorspace(),
                   CoreML::Specification::ImageFeatureType::GRAYSCALE);
  TS_ASSERT_EQUALS(input_feature_type.width(), 28);
  TS_ASSERT_EQUALS(input_feature_type.height(), 28);

  TS_ASSERT_EQUALS(my_model_desc.output_size(), 2);
  TS_ASSERT_EQUALS(my_model_desc.output(0).name(), target + "Probability");
  TS_ASSERT_EQUALS(my_model_desc.output(1).name(), target);

  TS_ASSERT_EQUALS(my_model_desc.predictedfeaturename(), target);
  TS_ASSERT_EQUALS(my_model_desc.predictedprobabilitiesname(),
                   target + "Probability");
}

BOOST_AUTO_TEST_CASE(test_save_load) {
  // TODO
}

}  // anonymous namespace
}  // namespace drawing_classifer
}  // namespace turi