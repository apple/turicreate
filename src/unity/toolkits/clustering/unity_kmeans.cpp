/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Data structures
#include <flexible_type/flexible_type_base_types.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <sframe/sframe.hpp>

// ML-Data Utils

// Toolkits
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/toolkits/clustering/unity_kmeans.hpp>
#include <unity/toolkits/clustering/kmeans.hpp>

// Miscellaneous
#include <export.hpp>

namespace turi {
namespace kmeans {


/**
 * Gets the current set of options
 */
toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // get the name of the model to query
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(invoke.params, "model_name");

  // retrieve the correct model
  std::shared_ptr<kmeans_model> model =
    safe_varmap_get<std::shared_ptr<kmeans_model>>(invoke.params, "model");

  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");

  // loop through the parameters and record in the return object
  for (const auto& opt : model->get_current_options())
    ret_status.params[opt.first] = opt.second;

  // return stuff
  ret_status.success = true;
  return ret_status;
}

/**
 * Gets any value from the model
 */
toolkit_function_response_type get_value(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // get the name of the model to query
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(invoke.params, "model_name");

  // retrieve the correct model
  std::shared_ptr<kmeans_model> model =
    safe_varmap_get<std::shared_ptr<kmeans_model>>(invoke.params, "model");

  // return stuff
  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");

  flexible_type field = safe_varmap_get<flexible_type>(invoke.params, "field");

  // query the specific field
  ret_status.params["value"] = model->get_value_from_state(field);

  // return stuff
  ret_status.success = true;
  return ret_status;
}


/**
 * Creates K-means clustering model.
 */
toolkit_function_response_type train(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // Get the model type
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(invoke.params, "model_name");

  std::shared_ptr<kmeans_model> model =
    std::dynamic_pointer_cast<kmeans_model>(invoke.classes->get_toolkit_class(model_name));

  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");


  // Get the data parameters
  sframe X = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
    invoke.params, "features")->get_underlying_sframe());

  sframe init_centers = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
    invoke.params, "initial_centers")->get_underlying_sframe());

  // Get the row labels.
  std::shared_ptr<sarray<flexible_type>> sa_row_labels =
    safe_varmap_get<std::shared_ptr<unity_sarray>>
    (invoke.params, "row_labels")->get_underlying_sarray();

  std::vector<flexible_type> row_labels(X.num_rows(), flexible_type(0));
  sa_row_labels->get_reader()->read_rows(0, X.num_rows(), row_labels);

  // Get the row label name
  std::map<std::string, flexible_type> opts = varmap_to_flexmap(invoke.params);
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
  ret_status.params["model"] = to_variant(model);
  ret_status.success = true;
  return ret_status;
}

/**
 * Predict method for the Kmeans clustering model.
 */
toolkit_function_response_type predict(toolkit_function_invocation& invoke) {

  log_func_entry();
  toolkit_function_response_type ret_status;

  // Retrieve the model.
  std::string model_name =
    (std::string) safe_varmap_get<flexible_type>(invoke.params, "model_name");

  std::shared_ptr<kmeans_model> model =
    safe_varmap_get<std::shared_ptr<kmeans_model>>(invoke.params, "model");

  if (model == nullptr)
    log_and_throw("Internal error: " + model_name + " is not a valid clustering model.");

  // Retrieve the prediction data.
  sframe X = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
    invoke.params, "dataset")->get_underlying_sframe());

  // Compute predictions and return the results.
  sframe result = model->predict(X);
  std::shared_ptr<unity_sframe> predictions(new unity_sframe());
  predictions->construct_from_sframe(result);

  ret_status.params["model"] = to_variant(model);
  ret_status.params["predictions"] = to_variant(predictions);
  ret_status.success = true;

  return ret_status;
}

/**
 * Obtain registration for the kmeans toolkit.
 */
EXPORT std::vector<toolkit_function_specification> get_toolkit_function_registration() {
  log_func_entry();

  toolkit_function_specification get_current_options_spec;
  get_current_options_spec.name = "kmeans_get_current_options";
  get_current_options_spec.toolkit_execute_function = get_current_options;

  toolkit_function_specification get_value_spec;
  get_value_spec.name = "kmeans_get_value";
  get_value_spec.toolkit_execute_function = get_value;

  toolkit_function_specification train_spec;
  train_spec.name = "kmeans_train";
  train_spec.toolkit_execute_function = train;

  toolkit_function_specification predict_spec;
  predict_spec.name = "kmeans_predict";
  predict_spec.toolkit_execute_function = predict;

  // toolkit_function_specification predict_spec;
  return {get_current_options_spec,
          get_value_spec,
          train_spec,
          predict_spec};
}


} // namespace kmeans
} // namespace turi
