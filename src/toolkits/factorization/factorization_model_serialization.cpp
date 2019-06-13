/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/serialization/serialization_includes.hpp>
#include <toolkits/factorization/factorization_model.hpp>
#include <toolkits/factorization/factorization_model_impl.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <core/logging/assertions.hpp>

namespace turi { namespace factorization {

////////////////////////////////////////////////////////////////////////////////
// Saving the model

/** Saves the factorization_model parameters.
 */
void factorization_model::local_save_impl(turi::oarchive& oarc) const {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Put the rest of the data into a variant map

  std::map<std::string, variant_type> data;

  data["options"]             = to_variant(options);
  data["n_total_dimensions"]  = to_variant(n_total_dimensions);
  data["index_sizes"]         = to_variant(index_sizes);
  data["index_offsets"]       = to_variant(index_offsets);
  data["loss_model_name"]     = to_variant(loss_model_name);
  data["column_shift_scales"] = to_variant(column_shift_scales);
  data["target_mean"]         = to_variant(target_mean);
  data["target_sd"]           = to_variant(target_sd);
  data["random_seed"]         = to_variant(random_seed);

  variant_deep_save(to_variant(data), oarc);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Save the metadata and side information.

  oarc << metadata;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Run the internal save function.

  save_impl(oarc);
}

////////////////////////////////////////////////////////////////////////////////

void factorization_model::local_load_version(turi::iarchive& iarc, size_t version) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Load the data for the base model.

  variant_type data_v;

  variant_deep_load(data_v, iarc);

  auto data = variant_get_value<std::map<std::string, variant_type> >(data_v);

#define __EXTRACT(varname)                                          \
  varname = variant_get_value<decltype(varname)>(data.at(#varname));

  __EXTRACT(options);
  __EXTRACT(n_total_dimensions);
  __EXTRACT(index_sizes);
  __EXTRACT(index_offsets);
  __EXTRACT(loss_model_name);
  __EXTRACT(column_shift_scales);
  __EXTRACT(target_mean);
  __EXTRACT(target_sd);
  __EXTRACT(random_seed);

#undef __EXTRACT

  iarc >> metadata;

  loss_model = get_loss_model_profile(loss_model_name);

  load_version(iarc, version);
}

////////////////////////////////////////////////////////////////////////////////
// Loading the model.

/** Loads and instantiates a factorization_model class with correct
 *  template parameters.  A pointer to the base factorization_model
 *  class is returned.
 */
std::shared_ptr<factorization_model>
factorization_model::factory_load(
    size_t version,
    const std::map<std::string, variant_type>& serialization_parameters,
    turi::iarchive& iarc) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Get the parameters relevant for instantiating the model

  std::string factor_mode_str = variant_get_value<std::string>(serialization_parameters.at("factor_mode"));
  flex_int num_factors_if_known     = variant_get_value<flex_int>(serialization_parameters.at("num_factors_if_known"));

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Instantiate the model

  std::shared_ptr<factorization_model> m;

  if(factor_mode_str == "factorization_machine") {

    if(num_factors_if_known == Eigen::Dynamic) {
      m.reset(new factorization_model_impl<model_factor_mode::factorization_machine, Eigen::Dynamic>());
    } else if(num_factors_if_known == 8) {
      m.reset(new factorization_model_impl<model_factor_mode::factorization_machine, 8>());
    } else {
      ASSERT_MSG(false, ("DESERIALIZE ERROR: For factorization_machine, "
                         "num_factors_if_known must be Eigen::Dynamic or 8."));
    }

  } else if(factor_mode_str == "matrix_factorization") {

    if(num_factors_if_known == Eigen::Dynamic) {
      m.reset(new factorization_model_impl<model_factor_mode::matrix_factorization, Eigen::Dynamic>());
    } else if(num_factors_if_known == 8) {
      m.reset(new factorization_model_impl<model_factor_mode::matrix_factorization, 8>());
    } else {
      ASSERT_MSG(false, ("DESERIALIZE ERROR: For matrix_factorization, "
                         "num_factors_if_known must be Eigen::Dynamic or 8."));
    }

  } else if(factor_mode_str == "pure_linear_model") {

    ASSERT_EQ(num_factors_if_known, 0);

    // This one only uses 0 for the linear model type.
    m.reset(new factorization_model_impl<model_factor_mode::pure_linear_model, 0>());

  } else {

    ASSERT_MSG(false, (std::string("On load: factor_mode_str not recognized: ") + factor_mode_str).c_str());
    return std::shared_ptr<factorization_model>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Deserialize the model

  m->local_load_version(iarc, version);

  return m;
}

}}
