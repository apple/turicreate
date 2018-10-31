/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "mps_cnn_module_factory.h"

#include "mps_graph_cnnmodule.h"

namespace turi {
namespace neural_net {

std::unique_ptr<cnn_module> create_mps_object_detector(
    int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
    const float_array_map& config, const float_array_map& weights) {

  std::unique_ptr<mps_graph_cnn_module> result(new mps_graph_cnn_module);

  result->init(/* netword_id */ kODGraphNet, n, c_in, h_in, w_in, c_out, h_out,
               w_out, config, weights);

  return result;
}

}  // namespace neural_net
}  // namespace turi
