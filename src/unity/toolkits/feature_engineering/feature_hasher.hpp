/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FEATURE_HASHER_H_
#define TURI_FEATURE_HASHER_H_
#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/toolkits/feature_engineering/transformer_base.hpp>
#include <export.hpp>
namespace turi{
namespace sdk_model{
namespace feature_engineering{

class EXPORT feature_hasher : public transformer_base {

  static constexpr size_t FEATURE_HASHER_VERSION = 0;
  std::vector<std::string> feature_columns;
  flexible_type unprocessed_features;
  bool fitted = false;
  bool exclude = false;
  std::map<std::string, flex_type_enum> feature_types;

  public:

  void init_transformer(const std::map<std::string, flexible_type>& _options);
  size_t get_version() const;
  void save_impl(oarchive& oarc) const;
  void load_version(iarchive& iarc, size_t version);
  void init_options(const std::map<std::string, flexible_type>& _options);
  void fit(gl_sframe data);
  gl_sframe transform(gl_sframe data);


  BEGIN_CLASS_MEMBER_REGISTRATION("_FeatureHasher")
  REGISTER_CLASS_MEMBER_FUNCTION(feature_hasher::init_transformer, "_options")
  REGISTER_CLASS_MEMBER_FUNCTION(feature_hasher::fit, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(feature_hasher::transform, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(feature_hasher::fit_transform, "data")
  REGISTER_CLASS_MEMBER_FUNCTION(feature_hasher::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(feature_hasher::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options", 
                             feature_hasher::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get", 
                             feature_hasher::get_value_from_state, 
                             "key");

  END_CLASS_MEMBER_REGISTRATION

};

} //namespace feature_engineering
} //namespace sdk_model
}// namespace turi
#endif
