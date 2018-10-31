/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
#define TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_

#include <functional>
#include <memory>

#include <unity/lib/extensions/ml_model.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/toolkits/neural_net/cnn_module.hpp>

namespace turi {
namespace object_detection {

class EXPORT object_detector: public ml_model_base {
 public:
  object_detector();

  // ml_model_base interface

  void init_options(const std::map<std::string,
                    flexible_type>& options) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;

  // Interface exposed via Unity server

  void train(gl_sframe data, std::string annotations_column_name,
             std::string image_column_name,
             std::map<std::string, flexible_type> options);

  // Register with Unity server

  BEGIN_CLASS_MEMBER_REGISTRATION("object_detector")

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::list_fields);

  REGISTER_CLASS_MEMBER_FUNCTION(object_detector::train, "data",
                                 "annotations_column_name",
                                 "image_column_name", "options");
  REGISTER_CLASS_MEMBER_DOCSTRING(
      object_detector::train,
      "\n"
      "Options\n"
      "-------\n"
      "model_params_path : string\n"
      "    Path to the CoreML specification with the pre-trained model parameters.\n"
  );
  // TODO: Addition training options: batch_size, max_iterations, etc.

  // TODO: Remainder of interface: predict, export_to_coreml, etc.

  END_CLASS_MEMBER_REGISTRATION

 protected:
  // Support for dependency injection, for testing purposes.
  using coreml_importer =
      std::function<neural_net::float_array_map(const std::string&)>;
  using module_factory =
      std::function<std::unique_ptr<neural_net::cnn_module>(
          int n, int c_in, int h_in, int w_in, int c_out, int h_out, int w_out,
          const neural_net::float_array_map& config,
          const neural_net::float_array_map& weights)>;
  object_detector(coreml_importer coreml_importer_fn,
                  module_factory module_factory_fn);

 private:
  // Injected dependencies
  coreml_importer coreml_importer_fn_;
  module_factory module_factory_fn_;

  std::unique_ptr<neural_net::cnn_module> training_module_;
};

}  // object_detection
}  // turi 

#endif  // TURI_OBJECT_DETECTION_OBJECT_DETECTOR_H_
