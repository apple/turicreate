/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/style_transfer/style_transfer_model_definition.hpp>

#include <ml/neural_net/weight_init.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>

namespace turi {
namespace style_transfer {

using CoreML::Specification::InnerProductLayerParams;
using CoreML::Specification::NeuralNetwork;
using CoreML::Specification::NeuralNetworkLayer;

using turi::neural_net::model_spec;
using turi::neural_net::ones_weight_initializer;
using turi::neural_net::zero_weight_initializer;

namespace {

std::unique_ptr<neural_net::model_spec> update_num_styles(
    std::unique_ptr<model_spec>& spec, size_t num_styles) {
  CoreML::Specification::NeuralNetwork neural_net = spec->get_coreml_spec();

  for (NeuralNetworkLayer& layer : *neural_net.mutable_layers()) {
    if (layer.name().find("_inst_") != std::string::npos) {
      InnerProductLayerParams* params = layer.mutable_innerproduct();
      params->set_inputchannels(num_styles);

      size_t weights_size = params->inputchannels() * params->outputchannels();

      if (layer.name().find("gamma") != std::string::npos) {
        init_weight_params(params->mutable_weights(), weights_size,
                           ones_weight_initializer());
      } else  {
        init_weight_params(params->mutable_weights(), weights_size,
                           zero_weight_initializer());
      }
    }
  }

  return std::unique_ptr<model_spec>(new model_spec(neural_net));
}

}  // namespace

std::unique_ptr<model_spec> init_resnet(std::string& path) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return spec;
}

std::unique_ptr<neural_net::model_spec> init_resnet(std::string& path,
                                                    size_t num_styles) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return update_num_styles(spec, num_styles);
}

std::unique_ptr<model_spec> init_vgg_16(std::string& path) {
  std::unique_ptr<model_spec> spec(new model_spec(path));
  return spec;
}

}  // namespace style_transfer
}  // namespace turi