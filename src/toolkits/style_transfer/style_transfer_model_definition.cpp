/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/style_transfer_model_definition.hpp>

namespace turi {
namespace style_transfer {

using CoreML::Specification::NeuralNetwork;
using turi::neural_net::model_spec;

namespace {

void change_num_styles(std::unique_ptr<model_spec>& spec, size_t num_styles) {}

}

std::unique_ptr<model_spec> init_resnet(std::string& path) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return spec;
}

std::unique_ptr<neural_net::model_spec> init_resnet(std::string& path,
                                                    size_t num_styles) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  change_num_styles(spec, num_styles);
  return spec;
}

std::unique_ptr<model_spec>  init_vgg_16(std::string& path) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return spec;
}

}  // namespace style_transfer
}  // namespace turi