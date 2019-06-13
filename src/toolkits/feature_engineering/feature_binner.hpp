/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef _FEATURE_BINNER_H_
#define _FEATURE_BINNER_H_
#include <string>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/feature_engineering/transformer_base.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/export.hpp>
namespace turi {
struct bin {
  double left;
  double right;
  size_t bin_id;
};
}
SERIALIZABLE_POD(turi::bin);


namespace turi{
namespace sdk_model {
namespace feature_engineering {


/**
 *
 * This transformation 1) creates a set of named bins and a mapping from
 * the reals to each bin, and 2) for each value returns the name of
 * the assigned bin.
 *
 * int/reals: returns the name of the bin for which bin_l < x < bin_r.
 *
 *
 * Bin creation options include:
 * - quantile: the bins are defined by the quantiles of all of the values
 *   seen by the transformer
 * - exponential: the bins are defined in a logarithmic scale...
 *   [0, 1), [1, 10), [10, 100), ..., [1e6, Inf)
 *
 *
 * Options:
 * bins = 'quantile'=> creates quantile bins?
 * bins = 'exponential'=>
 * bins = [0, 1, 5] => creates 5 bins: (-Inf, 0), [0, 1), [1, 5), [5,Inf)
 */
class EXPORT feature_binner : public transformer_base {

  static constexpr size_t FEATURE_BINNER_VERSION = 1;
  //VERSION 1 disabled support for array, list, and dict types
  // VERSION 1 also enumerates bins as <column_name>_1 instead
  // of making them the string representation of the range
  std::map<std::string, flex_type_enum> feature_types;
  std::vector<std::string> feature_columns;
  flexible_type unprocessed_features;      // Input provided by the user.
  bool fitted = false;
  bool exclude = false;
  std::map<std::string, std::vector<bin>> bins;

  public:

  /**
   * Methods that must be implemented in a new transformer model.
   * -------------------------------------------------------------------------
   */

  virtual inline ~feature_binner() {}

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
  BEGIN_CLASS_MEMBER_REGISTRATION("_FeatureBinner")
  REGISTER_CLASS_MEMBER_FUNCTION(feature_binner::init_transformer, "_options");
  REGISTER_CLASS_MEMBER_FUNCTION(feature_binner::fit, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(feature_binner::fit_transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(feature_binner::transform, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(feature_binner::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(feature_binner::list_fields);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
                                     feature_binner::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
                                     feature_binner::get_value_from_state,
                                     "key");
  END_CLASS_MEMBER_REGISTRATION

};


} // feature_engineering
} // sdk_model
} // turicreate

#endif
