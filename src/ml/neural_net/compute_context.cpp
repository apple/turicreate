/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <ml/neural_net/compute_context.hpp>

#if defined(HAS_MPS) && !defined(TC_BUILD_IOS)
#include <ml/neural_net/mps_compute_context.hpp>
#endif

namespace turi {
namespace neural_net {

// static
std::unique_ptr<compute_context> compute_context::create() {

#if defined(HAS_MPS) && !defined(TC_BUILD_IOS)
  return std::unique_ptr<compute_context>(new mps_compute_context);
#else
  return nullptr;
#endif
}

compute_context::~compute_context() = default;

}  // namespace neural_net
}  // namespace turi
