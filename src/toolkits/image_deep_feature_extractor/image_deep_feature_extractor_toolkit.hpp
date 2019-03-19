/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef IMAGE_DEEP_FEATURE_EXTRACTOR_TOOLKIT_HPP
#define IMAGE_DEEP_FEATURE_EXTRACTOR_TOOLKIT_HPP

#include <memory>

#include <toolkits/image_deep_feature_extractor/image_feature_extractor.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/extensions/ml_model.hpp>
#include <unity/lib/toolkit_class_macros.hpp>


namespace turi {
namespace image_deep_feature_extractor {

  
class EXPORT image_deep_feature_extractor_toolkit : public ml_model_base {
public:

  image_deep_feature_extractor_toolkit() { };

  void init_options(const std::map<std::string, flexible_type>& options) override;

  gl_sarray extract_features(gl_sframe data, const std::string& column_name, bool verbose, size_t batch_size) const;
  gl_sarray sarray_extract_features(gl_sarray data, bool verbose, size_t batch_size) const;

  inline size_t get_version() const override { return -1; }

  void save_impl(turi::oarchive& oarc) const override {
    log_and_throw("Model serialization is not support for this model");
  }

  void load_version(turi::iarchive& iarc, size_t version) override {
    log_and_throw("Model serialization is not support for this model");
  }

  BEGIN_CLASS_MEMBER_REGISTRATION("image_deep_feature_extractor")

  REGISTER_CLASS_MEMBER_FUNCTION(image_deep_feature_extractor_toolkit::init_options, "options");
    
  REGISTER_CLASS_MEMBER_FUNCTION(image_deep_feature_extractor_toolkit::extract_features, "data", "column_name", "verbose", "batch_size");
  REGISTER_CLASS_MEMBER_FUNCTION(image_deep_feature_extractor_toolkit::sarray_extract_features, "data", "verbose", "batch_size");

  END_CLASS_MEMBER_REGISTRATION

private:
  std::unique_ptr<image_feature_extractor> m_feature_extractor;

};


}  // image_deep_feature_extractor
}  // turi

#endif   // IMAGE_DEEP_FEATURE_EXTRACTOR_TOOLKIT_HPP
