/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_COREML_IMPORT_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_COREML_IMPORT_HPP_

#include <string>

#include <unity/toolkits/neural_net/float_array.hpp>

// Forward declare CoreML::Specification::Model in lieu of including problematic
// protocol buffer headers.

namespace CoreML {
namespace Specification {

class Model;

}  // namespace Specification
}  // namespace CoreML

namespace turi {
namespace neural_net {

/**
 * Destructively converts a CoreML specification into a dictionary mapping
 * layer names and parameters to neural_net::shared_float_array values.
 *
 * \param model CoreML specification that includes at least one neural network
 *              layer. This value is mutated to avoid copies: the returned
 *              float_array_map takes ownership of desired components.
 * \return A dictionary whose keys are of the form "$layername_$paramname". The
 *         layer names are taken from the name field of each NeuralNetworkLayer
 *         containing a supported layer.  The supported layers are
 *         ConvolutionLayerParams (with params "weight" (in NCHW order) and
 *         "bias") and BatchnormLayerParams (with params "gamma", "beta",
 *         "running_mean", and "running_var").
 * \throw If a NeuralNetworkLayer in the provided specification seems malformed
 *        (e.g. WeightParams with size inconsistent with declared layer shape).
 */
float_array_map extract_network_params(CoreML::Specification::Model* model);

/**
 * Convenience function that loads a CoreML specification from disk and
 * extracts the layer names and parameters found.
 *
 * \param mlmodel_path Path to a CoreML proto on disk.
 * \return A dictionary as returned by `extract_network_params`.
 * \throw If the indicated path could not be read or parsed.
 * \see extract_network_params()
 */
float_array_map load_network_params(const std::string& mlmodel_path);

}  // neural_net
}  // turi

#endif  // UNITY_TOOLKITS_NEURAL_NET_COREML_IMPORT_HPP_
