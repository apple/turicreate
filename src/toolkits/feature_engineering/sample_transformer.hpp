/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SAMPLE_TRANSFORMER_H_
#define TURI_SAMPLE_TRANSFORMER_H_
#include <string>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/feature_engineering/transformer_base.hpp>
#include <core/export.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * transformer_base example toolkit
 * ---------------------------------------------
 *
 * This example class also serves as test cases for the transformer
 * base class. Copy the class verbatim and change things for your
 * transformer goodness.
 *
 * Methods that must be implemented
 * -----------------------------------------
 *
 * Functions that should always be implemented.
 *
 *  init_transformers
 *  fit
 *  transform
 *  init_options
 *  get_version
 *  save_impl
 *  load_version
 *
 * This class interfaces with the TransformerBaseNative class in Python and works
 * end to end once the following set of fuctions are implemented by the user.
 *
 *
 * Example Class
 * -----------------------------------------------------------------------------
 *
 * This class does the wonderously complicated task of transforming your data
 * to a constant, no matter what you give it.
 *
 */
class EXPORT sample_transformer : public transformer_base {

  static constexpr size_t SAMPLE_TRANSFORMER_VERSION = 0;
  double constant = 0;                    /**< Transformation */

  public:

  /**
   * Methods that must be implemented in a new transformer model.
   * -------------------------------------------------------------------------
   */

  virtual inline ~sample_transformer() {}

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

  // Helper functions.
  // --------------------------------------------------------------------------
  inline double get_constant() { return constant;}

  // Functions that all transformers need to register. Can be copied verbatim
  // for other classes.
  // --------------------------------------------------------------------------
  BEGIN_CLASS_MEMBER_REGISTRATION("_SampleTransformer")
  REGISTER_CLASS_MEMBER_FUNCTION(sample_transformer::init_transformer, "_options");
  REGISTER_CLASS_MEMBER_FUNCTION(sample_transformer::fit, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(sample_transformer::fit_transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(sample_transformer::transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(sample_transformer::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(sample_transformer::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
                                     sample_transformer::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
                                     sample_transformer::get_value_from_state,
                                     "key");
  END_CLASS_MEMBER_REGISTRATION

};


} // feature_engineering
} // sdk_model
} // turicreate
#endif
