/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_QUADRATIC_FEATURES_H_
#define TURI_QUADRATIC_FEATURES_H_
#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/feature_engineering/transformer_base.hpp>
#include <core/export.hpp>

namespace turi{
namespace sdk_model{
namespace feature_engineering{

class EXPORT quadratic_features : public transformer_base {

  static constexpr size_t QUADRATIC_FEATURES_VERSION = 0;
  std::vector<std::vector<std::string>> unprocessed_features = {};
  std::vector<std::vector<std::string>> feature_pairs;
  std::map<std::string, flex_type_enum> feature_types;
  bool fitted = false;
  bool exclude = false;

  public:

  void init_transformer(const std::map<std::string, flexible_type>& _options) override;
  size_t get_version() const override;
  void save_impl(oarchive& oarc) const override;
  void load_version(iarchive& iarc, size_t version) override;
  void init_options(const std::map<std::string, flexible_type>& _options) override;
  void fit(gl_sframe training_data) override;
  gl_sframe transform(gl_sframe training_data) override;


  BEGIN_CLASS_MEMBER_REGISTRATION("_QuadraticFeatures")
  REGISTER_CLASS_MEMBER_FUNCTION(quadratic_features::init_transformer, "_options")
  REGISTER_CLASS_MEMBER_FUNCTION(quadratic_features::fit, "training_data")
  REGISTER_CLASS_MEMBER_FUNCTION(quadratic_features::transform, "training_data")
  REGISTER_CLASS_MEMBER_FUNCTION(quadratic_features::fit_transform, "training_data")
  REGISTER_CLASS_MEMBER_FUNCTION(quadratic_features::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(quadratic_features::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
                             quadratic_features::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
                             quadratic_features::get_value_from_state,
                             "key");

  END_CLASS_MEMBER_REGISTRATION

};

} //namespace feature_engineering
} //namespace sdk_model
}// namespace turi
#endif
