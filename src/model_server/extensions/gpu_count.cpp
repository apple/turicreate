/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/supervised_learning/neuralnet_device.hpp>

using namespace turi;

int get_gpu_count() {
  return supervised::neuralnet_v2::get_gpu_device_ids().size();
}

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(get_gpu_count)
END_FUNCTION_REGISTRATION
