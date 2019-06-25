/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SIMPLE_MODEL_HPP
#define TURI_UNITY_SIMPLE_MODEL_HPP

#include <model_server/lib/variant.hpp>
#include <model_server/lib/extensions/model_base.hpp>

namespace turi {
/**
 * \ingroup unity
 * The simple_model is the simplest implementation of the \ref model_base
 * object, containing just a map from string to variant and permitting
 * query operations on the map.
 */
class simple_model: public model_base {

 public:

  static constexpr size_t SIMPLE_MODEL_VERSION = 0;

  /// Default constructor
  simple_model() {}

  /**
   * Constructs a simple_model from a variant map.
   * A copy of the map is taken and stored.
   */
  explicit simple_model(const variant_map_type& params) : params(params) {}


  /// Lists all the keys stored in the variant map
  std::vector<std::string> list_fields();

  /**
   * Gets the value of a key in the variant map. Throws an error if the key
   * is not found. opts is ignored.
   */
  variant_type get_value(std::string key, variant_map_type& opts);

  /**
   * Returns the current model version
   */
  size_t get_version() const override;

  /**
   * Serializes the model. Must save the model to the file format version
   * matching that of get_version()
   */
  void save_impl(oarchive& oarc) const override;

  /**
   * Loads a model previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  void load_version(iarchive& iarc, size_t version) override;

  /// Destructor
  ~simple_model();

  /// Internal map
  variant_map_type params;

  BEGIN_CLASS_MEMBER_REGISTRATION("simple_model")
  REGISTER_CLASS_MEMBER_FUNCTION(simple_model::list_fields)
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get", simple_model::get_value, "key")
  END_CLASS_MEMBER_REGISTRATION
};

}

#endif // TURI_UNITY_SIMPLE_MODEL
