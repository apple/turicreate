/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
#define TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_

#include <memory>

#include <unity/lib/extensions/ml_model.hpp>
#include <unity/toolkits/neural_net/cnn_module.hpp>

namespace turi {
namespace object_detection {

class EXPORT object_detector: public ml_model_base {
 public:
  // ml_model_base interface

  void init_options(const std::map<std::string,
                    flexible_type>& options) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  // Interface exposed via Unity server

  BEGIN_CLASS_MEMBER_REGISTRATION("object_detector")

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::init_options, "options");
  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::list_fields)

  // TODO: Remainder of interface: train, predict, etc.

  END_CLASS_MEMBER_REGISTRATION

 private:
  std::unique_ptr<neural_net::cnn_module> cnn_module_;
};

}  // object_detection
}  // turi 

#endif  // TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
