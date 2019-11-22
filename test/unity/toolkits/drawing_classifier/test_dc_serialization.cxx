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
  drawing_classifier_mock() = default;

  drawing_classifier_mock(std::unique_ptr<neural_net::model_spec> ms)
      : drawing_classifier({}, std::move(ms), nullptr, nullptr, nullptr) {}

  std::unique_ptr<neural_net::model_spec> get_model_spec_copy() const {
    return clone_model_spec_for_test();
  }

  std::unique_ptr<neural_net::model_spec> get_model_spec() const {
    return init_model(true);
  }

};

BOOST_AUTO_TEST_CASE(test_dc_init_model) {
  // states
  constexpr unsigned int num_classes = 10;
  const std::string target = "target";
  // drawing_classifier enforces feature to be 1
  const std::vector<std::string> features = {"0"};

  // init_model
  drawing_classifier_mock dc;
  dc.add_or_update_state(
      {{"target", target},
       {"num_classes", num_classes},
       {"random_seed", 11},
       {"feature", features[0]}});

  auto nn_spec = dc.get_model_spec();

  const CoreML::Specification::NeuralNetwork& nn = nn_spec->get_coreml_spec();
  /*
   * 1 input,
   * 3 (conv layers, relu, maxpool), 1 flatten, 2 dense, 1 softmax
   * in total: 14 layers
   */
  TS_ASSERT_EQUALS(nn.layers_size(), 14);

  const std::string _suffix = "";

  /* layer 0: concat layer */
  {
    const std::map<int, int> layer_num_to_channels = {
        {0, 16}, {1, 32}, {2, 64}};
    for (auto& x : layer_num_to_channels) {
      int layer_index = x.first * 3;
      const auto& convlayer = nn.layers(layer_index);
      TS_ASSERT(convlayer.has_convolution());
      TS_ASSERT_EQUALS(convlayer.name(),
                       "drawing_conv" + std::to_string(x.first) + _suffix);
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
                       "drawing_relu" + std::to_string(x.first) + _suffix);
      TS_ASSERT(relu_layer.has_activation());
      TS_ASSERT(relu_layer.activation().has_relu());

      const auto& pool_layer = nn.layers(layer_index + 2);
      TS_ASSERT(pool_layer.has_pooling());
      TS_ASSERT_EQUALS(pool_layer.name(),
                       "drawing_pool" + std::to_string(x.first) + _suffix);
      TS_ASSERT_EQUALS(pool_layer.pooling().kernelsize(0), 2);
      TS_ASSERT_EQUALS(pool_layer.pooling().kernelsize(1), 2);
      TS_ASSERT_EQUALS(pool_layer.pooling().stride(0), 2);
      TS_ASSERT_EQUALS(pool_layer.pooling().stride(1), 2);
      TS_ASSERT(pool_layer.pooling().has_valid());
    }
  }

  unsigned layer_index = 9;
  {
    const auto& flatten_layer = nn.layers(layer_index);
    TS_ASSERT(flatten_layer.has_flatten());
    TS_ASSERT_EQUALS(flatten_layer.name(), "drawing_flatten0" + _suffix);
  }

  layer_index++;
  {
    const auto& dense_layer = nn.layers(layer_index);
    TS_ASSERT(dense_layer.has_innerproduct());
    TS_ASSERT_EQUALS(dense_layer.name(), "drawing_dense0" + _suffix);
    TS_ASSERT_EQUALS(dense_layer.innerproduct().inputchannels(), 64 * 3 * 3);
    TS_ASSERT_EQUALS(dense_layer.innerproduct().outputchannels(), 128);
  }

  layer_index++;
  {
      const auto& relu_layer = nn.layers(layer_index);
      TS_ASSERT_EQUALS(relu_layer.name(),
                       "drawing_dense0_relu" + _suffix);
      TS_ASSERT(relu_layer.has_activation());
      TS_ASSERT(relu_layer.activation().has_relu());
  }

  layer_index++;
  {
    const auto& dense_layer = nn.layers(layer_index);
    TS_ASSERT_EQUALS(dense_layer.name(), "drawing_dense1" + _suffix);
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
  const std::vector<std::string> features = {"0"};
  // drawing_classifier enforces the input feature to be 1,
  // in case someday we want more
  TS_ASSERT_EQUALS(features.size(), 1);
  const flex_list labels = {"0", "1"};

  turi::drawing_classifier::drawing_classifier dc;
  dc.add_or_update_state(
      {{"target", target},
       {"num_classes", labels.size()},
       {"classes", labels},
       {"max_iterations", 300},
       {"random_seed", 11},
       {"warm_start", ""},
       {"feature", features[0]}});

  auto ml_model_wrapper = dc.export_to_coreml("", "", {},
                                              /* debug no throw */ true);
  TS_ASSERT(ml_model_wrapper != nullptr);

  const auto& my_model_spec = ml_model_wrapper->coreml_model()->getProto();
  TS_ASSERT_EQUALS(my_model_spec.specificationversion(), 1);

  auto& my_model_desc = my_model_spec.description();
  TS_ASSERT_EQUALS(my_model_desc.input_size(), features.size());

  // test input image type
  TS_ASSERT_EQUALS(my_model_desc.input(0).name(), features[0]);
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
  auto remove_if_exist = [](const char* fname) {
    std::FILE* fp = nullptr;
    if ((fp = std::fopen(fname, "r")) != NULL) {
      if (std::fclose(fp) != 0) {
        std::stringstream ss;
        ss << "cannot close file: " << fname << std::endl;
        throw(turi::error::io_error(ss.str()));
      }

      if (std::remove(fname) != 0) {
        std::stringstream ss;
        ss << "cannot remove file: " << fname << std::endl;
        throw(turi::error::io_error(ss.str()));
      }
    }
  };

  auto load_save_compare = [&remove_if_exist](
                               drawing_classifier_mock& dc,
                               drawing_classifier_mock& dc_other) {
    constexpr auto my_file = "./test_dc_serialization.cxx.save.txt";
    remove_if_exist(my_file);
    std::stringstream ss;

    // save impl from the first mock
    {
      std::fstream out_file(my_file,
                            std::ios_base::out | std::ios_base::binary);
      turi::oarchive oarch(out_file);
      dc.save_impl(oarch);
      out_file.close();
    }

    // load impl to other dc mock instance
    {
      std::fstream in_file(my_file, std::ios_base::in | std::ios_base::binary);
      turi::iarchive iarch(in_file);
      dc_other.load_version(iarch, dc_other.get_version());
      in_file.close();
    }

    // clean myself
    if (std::remove(my_file)) {
      std::stringstream ss;
      ss << "fail to remove file:" << my_file << std::endl;
      std::perror(ss.str().c_str());
    }

    // compare weights in memory
    auto original_spec = dc.get_model_spec_copy();
    TS_ASSERT(original_spec != nullptr);
    auto original_view = original_spec->export_params_view();

    auto loaded_spec = dc_other.get_model_spec_copy();
    TS_ASSERT(loaded_spec != nullptr);
    auto loaded_view = loaded_spec->export_params_view();

    TS_ASSERT(original_view.size() > 1);
    TS_ASSERT_EQUALS(original_view.size(), loaded_view.size());

    for (const auto& entry : original_view) {
      // std::cout << entry.first << std::endl;
      auto& loaded_weights = loaded_view.at(entry.first);
      auto& original_weights = entry.second;
      TS_ASSERT(loaded_weights.size() > 0);
      TS_ASSERT_EQUALS(original_weights.size(), loaded_weights.size());
      TS_ASSERT_EQUALS(
       std::memcmp(loaded_weights.data(), original_weights.data(),
                       loaded_weights.size()),
          0);
    }
  };

  // states
  constexpr unsigned int num_classes = 10;
  const std::string target = "target";
  const std::vector<std::string> features = {"0"};
  // drawing_classifier enforces the input feature to be 1, in case someday we
  // want more
  TS_ASSERT_EQUALS(features.size(), 1);

  drawing_classifier_mock dummy;
  dummy.add_or_update_state(
      {{"target", target},
       {"num_classes", num_classes},
       {"random_seed", 1},
       {"feature", features[0]}});

  // model spec should be different since all weights are random
  // generated
  auto spec1 = dummy.get_model_spec();
  TS_ASSERT(spec1 != nullptr);

  dummy.add_or_update_state({{"random_seed", 2}});
  auto spec2 = dummy.get_model_spec();
  TS_ASSERT(spec2 != nullptr);

  auto view1 = spec1->export_params_view();
  auto view2 = spec2->export_params_view();

  bool is_same = true;

  for (const auto& entry : view1) {
    auto& weights2 = view2.at(entry.first);
    auto& weights1 = entry.second;
    TS_ASSERT(weights1.size() > 0);
    TS_ASSERT_EQUALS(weights1.size(), weights2.size());
    if (std::memcmp(weights1.data(), weights2.data(), weights1.size())) {
      is_same = false;
      break;
    }
  }

  // make sure 2 models params view are not same
  // due to the fact that they use different seed!
  TS_ASSERT(!is_same);

  // start from 2 model spec with different weights view
  drawing_classifier_mock dc(std::move(spec1));
  dc.add_or_update_state(
      {{"target", target},
       {"num_classes", num_classes},
       {"random_seed", 11},
       {"feature", features[0]}});

  // load from a different instance
  drawing_classifier_mock dc_other(std::move(spec2));
  dc_other.add_or_update_state(
      {{"target", target},
       {"num_classes", num_classes},
       {"random_seed", 11},
       {"feature", features[0]}});

  load_save_compare(dc, dc_other);
}

}  // anonymous namespace
}  // namespace drawing_classifer
}  // namespace turi