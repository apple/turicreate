/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Data structures
#include <flexible_type/flexible_type_base_types.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/variant.hpp>
#include <unity/lib/unity_global.hpp>

// Toolkits
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/toolkit_class_registry.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/toolkits/nearest_neighbors/unity_nearest_neighbors.hpp>
#include <unity/toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <unity/toolkits/nearest_neighbors/brute_force_neighbors.hpp>
#include <unity/toolkits/nearest_neighbors/ball_tree_neighbors.hpp>
#include <unity/toolkits/nearest_neighbors/lsh_neighbors.hpp>

// Miscellaneous
#include <export.hpp>

namespace turi {
namespace nearest_neighbors {


/** 
 * Check if the data is empty! 
 */  
void check_empty_data(const sframe& X) {  
  if (X.num_rows() == 0) { 
    log_and_throw("Input SFrame does not contain any rows.");     
  } 
    
  if (X.num_columns() == 0) {  
    log_and_throw("Input SFrame does not contain any columns."); 
  } 
} 
        

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
toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // get the name of the model to query
  std::string model_name 
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(invoke.params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  // loop through the parameters and record in the return object
  for (const auto& opt : model->get_current_options()) {
    ret_status.params[opt.first] = opt.second;
  }

  // return stuff
  ret_status.success = true;
  return ret_status;
}


/**
 * Get training stats.
 */
toolkit_function_response_type training_stats(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // get the name of the model to query
  std::string model_name 
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(invoke.params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  // loop through the parameters and record in the return object
  for (const auto& opt : model->get_training_stats()) {
    ret_status.params[opt.first] = opt.second;
  }

  // return stuff
  ret_status.success = true;
  return ret_status;
}


/**
 * Get any value from the model.
 */
toolkit_function_response_type get_value(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // get the name of the model to query
  std::string model_name 
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(invoke.params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  flexible_type field = safe_varmap_get<flexible_type>(invoke.params, "field");

  // query the specific field
  ret_status.params["value"] = model->get_value_from_state(field);

  // return stuff
  ret_status.success = true;
  return ret_status;
}


/**
 * List all keys in the model.
  */
toolkit_function_response_type list_keys(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // get the name of the model to query
  std::string model_name 
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "model_name");

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(invoke.params, "model");

  if (model == nullptr) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }

  // loop through the parameters and record in the return object
  for (const auto& k : model->list_keys()) {
    ret_status.params[k] = "";
  }

  // return stuff
  ret_status.success = true;
  return ret_status;
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

  auto ret_status = variant_map_type();
  ret_status["model"] = to_variant(model);

  return ret_status;
}


/** 
 * Query function for the nearest neighbors toolkit. 
 */
toolkit_function_response_type query(toolkit_function_invocation& invoke) {

  log_func_entry();
  toolkit_function_response_type ret_status;
  std::stringstream ss;


  // Make sure the model exists
  // ---------------------------------------------------------------------------
  std::string model_name 
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "model_name");
 

  // retrieve the correct model
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(invoke.params, "model");

  if (model == NULL) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }
 

  // Get features and label, validate, and remove unneeded columns
  // ---------------------------------------------------------------------------
  sframe Q
    = *((safe_varmap_get<std::shared_ptr<unity_sframe>>(
        invoke.params, "features"))->get_underlying_sframe());

  std::shared_ptr<sarray<flexible_type>> sa_query_labels = 
    safe_varmap_get<std::shared_ptr<unity_sarray>>(
    invoke.params, "query_labels")->get_underlying_sarray();

  std::vector<flexible_type> query_labels(Q.num_rows(), flexible_type(0));
  sa_query_labels->get_reader()->read_rows(0, Q.num_rows(), query_labels);

  size_t k = (size_t)safe_varmap_get<flexible_type>(invoke.params, "k");
  double radius = (double)safe_varmap_get<flexible_type>(invoke.params, "radius");

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
  ret_status.params["model"] = to_variant(model);
  ret_status.params["neighbors"] = to_variant(neighbors);
  ret_status.success = true;
  return ret_status;
}



/** 
 * Similarity graph function for the nearest neighbors toolkit. 
 */
toolkit_function_response_type similarity_graph(toolkit_function_invocation& invoke) {

  log_func_entry();
  toolkit_function_response_type ret_status;
  std::stringstream ss;


  // Make sure the model exists and retrieve it
  // ------------------------------------------
  std::string model_name 
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "model_name");
 
  std::shared_ptr<nearest_neighbors_model> model
      = safe_varmap_get<std::shared_ptr<nearest_neighbors_model>>(invoke.params, "model");

  if (model == NULL) {
    log_and_throw(model_name + " is not a nearest neighbors model.");
  }
 

  // Get method inputs.
  // ------------------
  size_t k = (size_t)safe_varmap_get<flexible_type>(invoke.params, "k");
  double radius = (double)safe_varmap_get<flexible_type>(invoke.params, "radius");
  bool include_self_edges = (bool)safe_varmap_get<flexible_type>(invoke.params, "include_self_edges");
  

  // Run the query and return results
  // --------------------------------
  sframe result = model->similarity_graph(k, radius, include_self_edges);

  std::shared_ptr<unity_sframe> neighbors(new unity_sframe());
  neighbors->construct_from_sframe(result);


  // return the pointer to the underlying SFrame resource
  ret_status.params["model"] = to_variant(model);
  ret_status.params["neighbors"] = to_variant(neighbors);
  ret_status.success = true;
  return ret_status;
}




/**
 * Obtain registration for the nearest_neighbors toolkit.
 */
EXPORT std::vector<toolkit_function_specification> get_toolkit_function_registration() {
  log_func_entry();

  toolkit_function_specification get_current_options_spec;
  get_current_options_spec.name = "get_current_options";
  get_current_options_spec.toolkit_execute_function = get_current_options;

  toolkit_function_specification training_stats_spec;
  training_stats_spec.name = "training_stats";
  training_stats_spec.toolkit_execute_function = training_stats;

  toolkit_function_specification get_value_spec;
  get_value_spec.name = "get_value";
  get_value_spec.toolkit_execute_function = get_value;

  toolkit_function_specification list_keys_spec;
  list_keys_spec.name = "list_keys";
  list_keys_spec.toolkit_execute_function = list_keys;

  toolkit_function_specification query_spec;
  query_spec.name = "query";
  query_spec.toolkit_execute_function = query;

  toolkit_function_specification similarity_graph_spec;
  similarity_graph_spec.name = "similarity_graph";
  similarity_graph_spec.toolkit_execute_function = similarity_graph;

  std::vector<toolkit_function_specification> specs{
          get_current_options_spec,
          training_stats_spec,
          get_value_spec,
          list_keys_spec,
          query_spec,
          similarity_graph_spec};

  REGISTER_FUNCTION(train, "params")
  return specs;
}


} // namespace nearest_neighbors
} // namespace turi
