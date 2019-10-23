/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_dc_serialization

#include <boost/algorithm/hex.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/uuid/detail/md5.hpp>
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
  std::unique_ptr<neural_net::model_spec> my_get_model_spec() const {
    return init_model();
  }

  std::unique_ptr<neural_net::model_spec> my_model_init_and_copy() {
    init_model_spec();
    return get_model_spec_copy();
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

  auto nn_spec = dc.my_get_model_spec();

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

  unsigned layer_index = 10;
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

BOOST_AUTO_TEST_CASE(test_load_version) {
  turi::drawing_classifier::drawing_classifier dc;
  TS_ASSERT_EQUALS(dc.get_version(), dc.DRAWING_CLASSIFIER_VERSION);
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

  /**
   * This is a hack because nn_spec_ is a private member. Thus, I can't
   * compare it directly after loading from file (load_version call).
   *
   * The solution is to call `load_version` first and write serialized
   * `nn_spec_` to disk. In this way, I can indirectly verify the `nn_spec_`
   * successively loads all params from the source model spec file.
   */
  auto load_save_compare = [&remove_if_exist](
                               drawing_classifier_mock& dc,
                               drawing_classifier_mock& dc_other) {
    // random init; avoid segfault
    dc.my_model_init_and_copy();
    dc_other.my_model_init_and_copy();

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

    // loading params from model spec
    // saved file will be used to compare with the model spec saved before
    constexpr auto my_file_loaded = "./test_dc_serialization.cxx.load.txt";
    remove_if_exist(my_file_loaded);

    {
      std::fstream out_file(my_file_loaded,
                            std::ios_base::out | std::ios_base::binary);
      turi::oarchive oarch(out_file);
      dc_other.save_impl(oarch);
      out_file.close();
    }

    // validation
    std::ifstream file(my_file,
                       std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::stringstream ss;
      ss << "fail to open" << my_file << std::endl;
      throw std::ios_base::failure(ss.str());
    }
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> model_spec_src(file_size);
    file.read(model_spec_src.data(), file_size);
    file.close();

    // open the saved model file and seek the cursor to EOF
    // in order to tell the size of the file
    file.open(my_file_loaded, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::stringstream ss;
      ss << "fail to open" << my_file_loaded << std::endl;
      throw std::ios_base::failure(ss.str());
    }
    TS_ASSERT_EQUALS(file_size, file.tellg());
    file_size = file.tellg();
    // reset the cursor
    file.seekg(0, std::ios::beg);

    std::vector<char> model_spec_loaded(file_size);
    file.read(model_spec_loaded.data(), file_size);
    file.close();

    // compare
    TS_ASSERT_EQUALS(std::strncmp(model_spec_loaded.data(),
                                  model_spec_src.data(), file_size),
                     0);

    // clean myself
    if (std::remove(my_file)) {
      std::stringstream ss;
      ss << "fail to remove file:" << my_file << std::endl;
      std::perror(ss.str().c_str());
    }
    if (std::remove(my_file_loaded)) {
      std::stringstream ss;
      ss << "fail to remove file:" << my_file_loaded << std::endl;
      std::perror(ss.str().c_str());
    }
  };

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

  // idenetity check; or double load
  load_save_compare(dc, dc);

  drawing_classifier_mock dc_other;
  dc_other.add_or_update_state(
      {{"target", target},
       {"num_classes", num_classes},
       {"features", flex_list(features.begin(), features.end())}});

  // load from a different instance
  load_save_compare(dc, dc_other);
}

}  // anonymous namespace
}  // namespace drawing_classifer
}  // namespace turi