/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/object_detection/od_serialization.hpp>
#include <cstdio>

namespace turi {
namespace object_detection {

using turi::neural_net::model_spec;
using padding_type = model_spec::padding_type;
using turi::neural_net::float_array_map;
using turi::neural_net::xavier_weight_initializer;

namespace {

// Each bounding box is evaluated relative to a list of pre-defined sizes.
const std::vector<std::pair<float, float>>& anchor_boxes() {
  static const std::vector<std::pair<float, float>>* const default_boxes =
      new std::vector<std::pair<float, float>>({
          {1.f, 2.f}, {1.f, 1.f}, {2.f, 1.f},
          {2.f, 4.f}, {2.f, 2.f}, {4.f, 2.f},
          {4.f, 8.f}, {4.f, 4.f}, {8.f, 4.f},
          {8.f, 16.f}, {8.f, 8.f}, {16.f, 8.f},
          {16.f, 32.f}, {16.f, 16.f}, {32.f, 16.f},
      });
  return *default_boxes;
};

}  // namespace

void _save_impl(oarchive& oarc, const neural_net::model_spec& nn_spec_, const std::map<std::string, variant_type>& state) {

  // Save model attributes.
  variant_deep_save(state, oarc);

  // Save neural net weights.
  oarc << nn_spec_.export_params_view();

}

void _load_version(iarchive& iarc, size_t version, neural_net::model_spec& nn_spec_, std::map<std::string, variant_type>& state) {
  
  // Load model attributes.
  variant_deep_load(state, iarc);

  // Load neural net weights.
  float_array_map nn_params;
  iarc >> nn_params;
  init_darknet_yolo(nn_spec_,state);
  nn_spec_.update_params(nn_params);

}

void init_darknet_yolo(neural_net::model_spec& nn_spec, const std::map<std::string, variant_type>& state) { 

  // Initialize a random number generator for weight initialization.
  std::seed_seq seed_seq = { variant_get_value<int>(state.at("random_seed")) };
  std::mt19937 random_engine(seed_seq);

  // Initilize Layer 0
  int num_features = 3;
  xavier_weight_initializer conv0_init_fn(16*3*3, 16*3*3, &random_engine);
  nn_spec.add_convolution(/* name */                "conv0_fwd",
                           /* input */               "leakyrelu6_fwd",
                           /* num_output_channels */ 16,
                           /* num_kernel_channels */ num_features,
                           /* kernel_height */       3,
                           /* kernel_width */        3,
                           /* stride_height */       1,
                           /* stride_width */        1,
                           /* padding */             padding_type::SAME,
                           /* weight_init_fn */      conv0_init_fn);

  // Append batchnorm7.
  nn_spec.add_batchnorm(/* name */                  "batchnorm0_fwd",
                         /* input */                 "conv0_fwd",
                         /* num_channels */          16,
                         /* epsilon */               0.00001f);

  // Append leakyrelu7.
  nn_spec.add_leakyrelu(/* name */                  "leakyrelu0_fwd",
                         /* input */                 "batchnorm0_fwd",
                         /* alpha */                 0.1f);

  num_features = 16;
  // Initilize Layer 1 to Layer 6
  const std::map<int, int> layer_num_to_channels = {{1, 32}, {2,64}, {3, 128}, {4, 256}, {5,512}, {6, 1024}};
  for (auto const& x : layer_num_to_channels)
  {
      nn_spec.add_convolution(/* name */                "conv"+std::to_string(x.first)+"_fwd",
                               /* input */               "leakyrelu"+std::to_string(x.first-1)+"_fwd",
                               /* num_output_channels */ x.second,
                               /* num_kernel_channels */ num_features,
                               /* kernel_height */       3,
                               /* kernel_width */        3,
                               /* stride_height */       1,
                               /* stride_width */        1,
                               /* padding */             padding_type::SAME,
                               /* weight_init_fn */      xavier_weight_initializer(x.second*3*3, x.second*3*3, &random_engine));

      // Append batchnorm7.
      nn_spec.add_batchnorm(/* name */                  "batchnorm"+std::to_string(x.first)+"_fwd",
                             /* input */                 "conv"+std::to_string(x.first)+"_fwd",
                             /* num_channels */          x.second,
                             /* epsilon */               0.00001f);

      // Append leakyrelu7.
      nn_spec.add_leakyrelu(/* name */                  "leakyrelu"+std::to_string(x.first)+"_fwd",
                             /* input */                 "batchnorm"+std::to_string(x.first)+"_fwd",
                             /* alpha */                 0.1f);

      num_features = x.second;
  }

  // Append conv7, initialized using the Xavier method (with base magnitude 3).
  // The conv7 weights have shape [1024, 1024, 3, 3], so fan in and fan out are
  // both 1024*3*3.
  xavier_weight_initializer conv7_init_fn(1024*3*3, 1024*3*3, &random_engine);
  nn_spec.add_convolution(/* name */                "conv7_fwd",
                           /* input */               "leakyrelu6_fwd",
                           /* num_output_channels */ 1024,
                           /* num_kernel_channels */ 1024,
                           /* kernel_height */       3,
                           /* kernel_width */        3,
                           /* stride_height */       1,
                           /* stride_width */        1,
                           /* padding */             padding_type::SAME,
                           /* weight_init_fn */      conv7_init_fn);

  // Append batchnorm7.
  nn_spec.add_batchnorm(/* name */                  "batchnorm7_fwd",
                         /* input */                 "conv7_fwd",
                         /* num_channels */          1024,
                         /* epsilon */               0.00001f);

  // Append leakyrelu7.
  nn_spec.add_leakyrelu(/* name */                  "leakyrelu7_fwd",
                         /* input */                 "batchnorm7_fwd",
                         /* alpha */                 0.1f);

  // Append conv8.
  static constexpr float CONV8_MAGNITUDE = 0.00005f;
  const size_t num_classes = variant_get_value<size_t>(state.at("num_classes"));
  const size_t num_predictions = 5 + num_classes;  // Per anchor box
  const size_t conv8_c_out = anchor_boxes().size() * num_predictions;
  auto conv8_weight_init_fn = [&random_engine](float* w, float* w_end) {
    std::uniform_real_distribution<float> dist(-CONV8_MAGNITUDE,
                                               CONV8_MAGNITUDE);
    while (w != w_end) {
      *w++ = dist(random_engine);
    }
  };
  auto conv8_bias_init_fn = [num_predictions](float* w, float* w_end) {
    while (w < w_end) {
      // Initialize object confidence low, preventing an unnecessary adjustment
      // period toward conservative estimates
      w[4] = -6.f;

      // Iterate through each anchor box.
      w += num_predictions;
    }
  };
  nn_spec.add_convolution(/* name */                "conv8_fwd",
                           /* input */               "leakyrelu7_fwd",
                           /* num_output_channels */ conv8_c_out,
                           /* num_kernel_channels */ 1024,
                           /* kernel_height */       1,
                           /* kernel_width */        1,
                           /* stride_height */       1,
                           /* stride_width */        1,
                           /* padding */             padding_type::SAME,
                           /* weight_init_fn */      conv8_weight_init_fn,
                           /* bias_init_fn */        conv8_bias_init_fn);

}

}  // object_detection
}  // turi
