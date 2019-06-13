/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef _count_thresholder_INDEXER_H_
#define _count_thresholder_INDEXER_H_
#include <string>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/feature_engineering/transformer_base.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

class count_thresholder : public transformer_base {

  static constexpr size_t COUNT_THRESHOLDER_VERSION = 0;
  std::map<std::string, std::shared_ptr<topk_indexer>> index_map;
  std::map<std::string, flex_type_enum> feature_types;
  flexible_type feature_columns;               // Input provided by the user.
  bool exclude = false;

  public:

  /**
   * Methods that must be implemented in a new transformer model.
   * -------------------------------------------------------------------------
   */

  virtual inline ~count_thresholder() {}

  /**
   * Set one of the options in the model. Use the option manager to set
   * these options. If the option does not satisfy the conditions that the
   * option manager has imposed on it. Errors will be thrown.
   *
   * \param[in] options Options to set
   */
  void init_options(const std::map<std::string, flexible_type>&_options) override;

  /**
   * Get a version for the object.
   */
  size_t get_version() const override;

  /**
   * Save the object using Turi's oarc.
   */
  void save_impl(turi::oarchive& oarc) const override;

  /**
   * Load the object using Turi's iarc.
   */
  void load_version(turi::iarchive& iarc, size_t version) override;


  /**
   * Initialize the transformer.
   */
  void init_transformer(const std::map<std::string,
                        flexible_type>& _options) override;

  /**
   * Set constant.
   *
   * \param[in] data  (SFrame of data)
   */
  void fit(gl_sframe data) override;

  /**
   * Transform the given data.
   *
   * \param[in] data  (SFrame of data)
   *
   * Python side interface
   * ------------------------
   * This function directly interfaces with "transform" in python.
   *
   */
  gl_sframe transform(gl_sframe data) override;

  /**
   * Fit and transform the given data. Intended as an optimization because
   * fit and transform are usually always called together. The default
   * implementaiton calls fit and then transform.
   *
   * \param[in] data  (SFrame of data)
   */
  gl_sframe fit_transform(gl_sframe data) {
     data.materialize();
     fit(data);
     return transform(data);
  }



  // Functions that all transformers need to register. Can be copied verbatim
  // for other classes.
  // --------------------------------------------------------------------------
  BEGIN_CLASS_MEMBER_REGISTRATION("_CountThresholder")
  REGISTER_CLASS_MEMBER_FUNCTION(count_thresholder::init_transformer, "_options");
  REGISTER_CLASS_MEMBER_FUNCTION(count_thresholder::fit, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(count_thresholder::fit_transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(count_thresholder::transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(count_thresholder::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(count_thresholder::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
                                     count_thresholder::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
                                     count_thresholder::get_value_from_state,
                                     "key");
  END_CLASS_MEMBER_REGISTRATION

};


} // feature_engineering
} // sdk_model
} // turicreate
#endif
