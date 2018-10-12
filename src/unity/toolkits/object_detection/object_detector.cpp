/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <unity/toolkits/object_detection/object_detector.hpp>

using turi::mps::cnn_module;

namespace turi {
namespace object_detection {

namespace {

constexpr size_t OBJECT_DETECTOR_VERSION = 1;

}  // namespace

void object_detector::init_options(const std::map<std::string,
                                   flexible_type>& options) {
  // TODO: Push platform dependency into (likely renamed) TCMPS library.
#ifdef HAS_MPS
  cnn_module_ = cnn_module::create_object_detector(
      0, 0, 0, 0, 0, 0, 0, mps::float_array_map(),
      mps::float_array_map());
#endif  // HAS_MPS
}

size_t object_detector::get_version() const {
  return OBJECT_DETECTOR_VERSION;
}

void object_detector::save_impl(oarchive& oarc) const {
}

void object_detector::load_version(iarchive& iarc, size_t version) {
}


}  // object_detection
}  // turi 

