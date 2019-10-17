/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/util/training_utils.hpp>

#include <vector>

namespace turi {

void print_training_device(std::vector<std::string> gpu_names) {
  if (gpu_names.empty()) {
    logprogress_stream << "Using CPU to create model";
  } else {
    std::string gpu_names_string = gpu_names[0];
    for (size_t i = 1; i < gpu_names.size(); ++i) {
      gpu_names_string += ", " + gpu_names[i];
    }
    // TODO: Use a better condition to distinguish between hardware
    if (gpu_names_string.find("/") != std::string::npos) {
      logprogress_stream << "Using " << gpu_names.size()
                         << (gpu_names.size() > 1 ? " GPUs" : " GPU")
                         << " to create model (CUDA)";

    } else {
      logprogress_stream << "Using " << (gpu_names.size() > 1 ? "GPUs" : "GPU")
                         << " to create model (" << gpu_names_string << ")";
    }
  }
}

}  // namespace turi