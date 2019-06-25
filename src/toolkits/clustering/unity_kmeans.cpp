/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Data structures
#include <core/data/flexible_type/flexible_type_base_types.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/sframe_data/sframe.hpp>

// ML-Data Utils

// Toolkits
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <toolkits/clustering/unity_kmeans.hpp>
#include <toolkits/clustering/kmeans.hpp>

// Miscellaneous
#include <core/export.hpp>

namespace turi {
namespace kmeans {


/**
 * Gets the current set of options
 */
variant_map_type get_current_options(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // get the name of the model to query
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(params, "model_name");

  // retrieve the correct model
  std::shared_ptr<kmeans_model> model =
    safe_varmap_get<std::shared_ptr<kmeans_model>>(params, "model");

  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");

  // loop through the parameters and record in the return object
  for (const auto& opt : model->get_current_options())
    ret[opt.first] = opt.second;

  // return stuff
  return ret;
}

/**
 * Gets any value from the model
 */
variant_map_type get_value(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // get the name of the model to query
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(params, "model_name");

  // retrieve the correct model
  std::shared_ptr<kmeans_model> model =
    safe_varmap_get<std::shared_ptr<kmeans_model>>(params, "model");

  // return stuff
  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");

  flexible_type field = safe_varmap_get<flexible_type>(params, "field");

  // query the specific field
  ret["value"] = model->get_value_from_state(field);

  // return stuff
  return ret;
}


/**
 * Creates K-means clustering model.
 */
variant_map_type train(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // Get the model type
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(params, "model_name");

  std::shared_ptr<kmeans_model> model;
  if (model_name == "kmeans") {
    model = std::make_shared<kmeans_model>();
  }
  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");


  // Get the data parameters
  sframe X = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
    params, "features")->get_underlying_sframe());

  sframe init_centers = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
    params, "initial_centers")->get_underlying_sframe());

  // Get the row labels.
  std::shared_ptr<sarray<flexible_type>> sa_row_labels =
    safe_varmap_get<std::shared_ptr<unity_sarray>>
    (params, "row_labels")->get_underlying_sarray();

  std::vector<flexible_type> row_labels(X.num_rows(), flexible_type(0));
  sa_row_labels->get_reader()->read_rows(0, X.num_rows(), row_labels);

  // Get the row label name
  std::map<std::string, flexible_type> opts = varmap_to_flexmap(params);
  std::string row_label_name;
  row_label_name = (std::string) opts.at("row_label_name");

  // Initialize model options
  opts.erase("model_name");
  opts.erase("row_label_name");
  model->init_options(opts);

  // Train the model
  model->train(X, init_centers, opts.at("method"), row_labels, row_label_name);


  // Set model objects in the 'state' variable to be visible to the Python API.
  sframe assignments = model->get_cluster_assignments();
  std::shared_ptr<unity_sframe> unity_assignments(new unity_sframe);
  unity_assignments->construct_from_sframe(assignments);

  sframe cluster_info = model->get_cluster_info();
  std::shared_ptr<unity_sframe> unity_cluster_info(new unity_sframe);
  unity_cluster_info->construct_from_sframe(cluster_info);

  model->add_or_update_state({ {"cluster_id", to_variant(unity_assignments)},
                               {"cluster_info", to_variant(unity_cluster_info)} });


  // Return the model with all stats
  ret["model"] = to_variant(model);
  return ret;
}

/**
 * Predict method for the Kmeans clustering model.
 */
variant_map_type predict(variant_map_type& params) {

  log_func_entry();
  variant_map_type ret;

  // Retrieve the model.
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(params, "model_name");

  std::shared_ptr<kmeans_model> model =
    safe_varmap_get<std::shared_ptr<kmeans_model>>(params, "model");

  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");

  // Retrieve the prediction data.
  sframe X = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
    params, "dataset")->get_underlying_sframe());

  // Compute predictions and return the results.
  sframe result = model->predict(X);
  std::shared_ptr<unity_sframe> predictions(new unity_sframe());
  predictions->construct_from_sframe(result);

  ret["model"] = to_variant(model);
  ret["predictions"] = to_variant(predictions);

  return ret;
}

/**
 * Obtain registration for the kmeans toolkit.
 */
BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(get_current_options, "params")
REGISTER_FUNCTION(get_value, "params")
REGISTER_FUNCTION(train, "params")
REGISTER_FUNCTION(predict, "params")
END_FUNCTION_REGISTRATION


} // namespace kmeans
} // namespace turi
