/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Data structures
#include <core/data/flexible_type/flexible_type_base_types.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/unity_global.hpp>

// Toolkits
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/toolkit_class_registry.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <toolkits/nearest_neighbors/unity_nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/brute_force_neighbors.hpp>
#include <toolkits/nearest_neighbors/ball_tree_neighbors.hpp>
#include <toolkits/nearest_neighbors/lsh_neighbors.hpp>

// Miscellaneous
#include <core/export.hpp>

namespace turi {
namespace nearest_neighbors {


/**
* Get the list of options that are relevant to each model.
*/
std::vector<std::string> get_model_option_keys(std::string model_name) {

  // Brute-force
  if (model_name == "nearest_neighbors_brute_force") {
    return {"label"};
  } else if (model_name == "nearest_neighbors_ball_tree") {
    return {"leaf_size", "label"};
  } else if (model_name == "nearest_neighbors_lsh") {
    return {"num_tables", "num_projections_per_table", "label"};
  } else { // Not a nearest neighbors model. This should never happen.
    log_and_throw(model_name + " is not a nearest neighbors model.");
    return {};
  }
}


/**
 * Get the current set of options.
 */
variant_map_type get_current_options(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // get the name of the model to query
  std::string model_name
    = (std::string)safe_varmap_get<flexible_type>(params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  // loop through the parameters and record in the return object
  for (const auto& opt : model->get_current_options()) {
    ret[opt.first] = opt.second;
  }

  // return stuff
  return ret;
}


/**
 * Get training stats.
 */
variant_map_type training_stats(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // get the name of the model to query
  std::string model_name
    = (std::string)safe_varmap_get<flexible_type>(params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  // loop through the parameters and record in the return object
  for (const auto& opt : model->get_training_stats()) {
    ret[opt.first] = opt.second;
  }

  // return stuff
  return ret;
}


/**
 * Get any value from the model.
 */
variant_map_type get_value(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // get the name of the model to query
  std::string model_name
    = (std::string)safe_varmap_get<flexible_type>(params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  flexible_type field = safe_varmap_get<flexible_type>(params, "field");

  // query the specific field
  ret["value"] = model->get_value_from_state(field);

  // return stuff
  return ret;
}


/**
 * List all keys in the model.
  */
variant_map_type list_fields(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // get the name of the model to query
  std::string model_name
    = (std::string)safe_varmap_get<flexible_type>(params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  // loop through the parameters and record in the return object
  for (const auto& k : model->list_fields()) {
    ret[k] = "";
  }

  // return stuff
  return ret;
}


/**
 * Creation function for nearest neighbors reference objects. Checks for errors
 * in inputs and makes sure all options provided by the user overwrite default
 * options.
 */
variant_map_type train(variant_map_type& params) {
  log_func_entry();

  /*** Unpack the inputs ***/

  // Model name and model
  std::string model_name
    = (std::string)safe_varmap_get<flexible_type>(params, "model_name");

  // Construct a model
  std::shared_ptr<nearest_neighbors_model> model;
  if (model_name == "nearest_neighbors_brute_force") {
    model.reset(new brute_force_neighbors);
  } else if (model_name == "nearest_neighbors_ball_tree"){
    model.reset(new ball_tree_neighbors);
  } else if (model_name == "nearest_neighbors_lsh"){
    model.reset(new lsh_neighbors);
  } else {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  // Data
  sframe X
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
    params, "sf_features")->get_underlying_sframe());

  std::shared_ptr<sarray<flexible_type>> sa_ref_labels =
    safe_varmap_get<std::shared_ptr<unity_sarray>>
    (params, "ref_labels")->get_underlying_sarray();

  std::vector<flexible_type> ref_labels(X.num_rows(), flexible_type(0));
  sa_ref_labels->get_reader()->read_rows(0, X.num_rows(), ref_labels);

  // Composite distances
  std::vector<dist_component_type> composite_distance_params =
    safe_varmap_get<std::vector<dist_component_type> >(params,
                                                       "composite_params");

  // Model options
  std::map<std::string, flexible_type> opts;
  std::vector<std::string> model_option_keys;

  model_option_keys = get_model_option_keys(model_name);
  for (const auto& key: model_option_keys) {
    if (params.count(key) > 0) {
      opts[key] = safe_varmap_get<flexible_type>(params, key);
    }
  }

  /*** Initialize and train the model ***/
  model->train(X, ref_labels, composite_distance_params, opts);

  variant_map_type ret;
  ret["model"] = to_variant(model);
  return ret;
}


/**
 * Query function for the nearest neighbors toolkit.
 */
variant_map_type query(variant_map_type& params) {

  log_func_entry();
  variant_map_type ret;
  std::stringstream ss;


  // Make sure the model exists
  // ---------------------------------------------------------------------------
  std::string model_name
    = (std::string)safe_varmap_get<flexible_type>(params, "model_name");


  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(params, "model");

  if (model == NULL) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }


  // Get features and label, validate, and remove unneeded columns
  // ---------------------------------------------------------------------------
  sframe Q
    = *((safe_varmap_get<std::shared_ptr<unity_sframe>>(
        params, "features"))->get_underlying_sframe());

  std::shared_ptr<sarray<flexible_type>> sa_query_labels =
    safe_varmap_get<std::shared_ptr<unity_sarray>>(
    params, "query_labels")->get_underlying_sarray();

  std::vector<flexible_type> query_labels(Q.num_rows(), flexible_type(0));
  sa_query_labels->get_reader()->read_rows(0, Q.num_rows(), query_labels);

  size_t k = (size_t)safe_varmap_get<flexible_type>(params, "k");
  double radius = (double)safe_varmap_get<flexible_type>(params, "radius");

  // Run the query and return results
  // --------------------------------
  sframe result = model->query(Q, query_labels, k, radius);
  std::shared_ptr<unity_sframe> neighbors(new unity_sframe());
  neighbors->construct_from_sframe(result);

  /*
  if (model_name == "nearest_neighbors_lsh") {
    neighbors = std::dynamic_pointer_cast<unity_sframe>(
        neighbors->sort({"query_label", "rank"}, {1, 1}));
  }
  */

  // return the pointer to the underlying SFrame resource
  ret["model"] = to_variant(model);
  ret["neighbors"] = to_variant(neighbors);
  return ret;
}



/**
 * Similarity graph function for the nearest neighbors toolkit.
 */
variant_map_type similarity_graph(variant_map_type& params) {

  log_func_entry();
  variant_map_type ret;
  std::stringstream ss;


  // Make sure the model exists and retrieve it
  // ------------------------------------------
  std::string model_name
    = (std::string)safe_varmap_get<flexible_type>(params, "model_name");

  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(params, "model");

  if (model == NULL) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }


  // Get method inputs.
  // ------------------
  size_t k = (size_t)safe_varmap_get<flexible_type>(params, "k");
  double radius = (double)safe_varmap_get<flexible_type>(params, "radius");
  bool include_self_edges = (bool)safe_varmap_get<flexible_type>(params, "include_self_edges");


  // Run the query and return results
  // --------------------------------
  sframe result = model->similarity_graph(k, radius, include_self_edges);

  std::shared_ptr<unity_sframe> neighbors(new unity_sframe());
  neighbors->construct_from_sframe(result);


  // return the pointer to the underlying SFrame resource
  ret["model"] = to_variant(model);
  ret["neighbors"] = to_variant(neighbors);
  return ret;
}




/**
 * Obtain registration for the nearest_neighbors toolkit.
 */
BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(get_current_options, "params")
REGISTER_FUNCTION(training_stats, "params")
REGISTER_FUNCTION(get_value, "params")
REGISTER_FUNCTION(list_fields, "params")
REGISTER_FUNCTION(train, "params")
REGISTER_FUNCTION(query, "params")
REGISTER_FUNCTION(similarity_graph, "params")
REGISTER_FUNCTION(_nn_get_reference_data, "model");
END_FUNCTION_REGISTRATION


} // namespace nearest_neighbors
} // namespace turi
