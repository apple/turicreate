/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Data structures
#include <flexible_type/flexible_type_base_types.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <sframe/sframe.hpp>
#include <sframe/sframe_iterators.hpp>

// Unity library
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/toolkit_function_specification.hpp>

// ML-Data Utils
#include <ml_data/ml_data.hpp>

// Toolkits
#include <unity/toolkits/ml_model/ml_model.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <toolkits/supervised_learning/unity_supervised_learning.hpp>
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>

/// SDK
#include <unity/lib/toolkit_function_macros.hpp>
#include <export.hpp>
namespace turi {
namespace supervised {

// TODO: List of todo's for this file
//------------------------------------------------------------------------------


/**
 * Obtain a supervised learning object type from the python dictionary.
 *
 * \param[in] invoke  Invocation object
 * \param[in] key     The key in invoke.params
 */
std::shared_ptr<supervised_learning_model_base> 
      get_supervised_learning_model(toolkit_function_invocation& invoke,
                                   std::string model_key){
  DASSERT_TRUE(invoke.params.count("model_name") > 0);
  std::shared_ptr<supervised_learning_model_base> model;
  model = safe_varmap_get<std::shared_ptr<supervised_learning_model_base>>( 
                                invoke.params, model_key);

  // This should not happen.
  if (model == NULL) {
    std::string model_name 
      = (std::string)safe_varmap_get<flexible_type>(invoke.params, "model_name");
    log_and_throw("Internal error: " + model_name + 
                                      " is not a supervised learning model.");
  }
  return model;
}


/*
 * Train function init.
 */
toolkit_function_response_type train(toolkit_function_invocation& invoke) {
  log_func_entry();
  DASSERT_TRUE(invoke.params.count("model_name") > 0);
  DASSERT_TRUE(invoke.params.count("target") > 0);
  DASSERT_TRUE(invoke.params.count("features") > 0);

  // Get data from Python. 
  sframe X
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            invoke.params, "features")->get_underlying_sframe());
  sframe y
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            invoke.params, "target")->get_underlying_sframe());   
  std::string model_name 
    = safe_varmap_get<std::string>(invoke.params, "model_name");

 
  // Remove option names that are not needed.
  variant_map_type kwargs = invoke.params;
  kwargs.erase("model_name");
  kwargs.erase("target");
  kwargs.erase("features");

  // Train a model.
  std::shared_ptr<supervised_learning_model_base> model = create(
                          X, y, model_name, kwargs);

  // Return options and model objects.
  toolkit_function_response_type ret_status;
  ret_status.params["model"] = to_variant(model);
  ret_status.success = true;
  return ret_status;
}

/**
 * Init function for extract_feature.
 */
toolkit_function_response_type extract_feature(toolkit_function_invocation& invoke){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(invoke.params,
        "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model =
    get_supervised_learning_model(invoke, "model");
  std::string model_name = model->name();

  // Extract the features
  // --------------------------------------------------------------------------
  toolkit_function_response_type ret_status;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            invoke.params, "dataset")->get_underlying_sframe());
  
  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);
  std::shared_ptr<sarray<flexible_type>> py_ptr;

  std::map<std::string, flexible_type> options;
  for (const auto& kv: invoke.params) {
    try {
      options[kv.first] = safe_varmap_get<flexible_type>(invoke.params, kv.first);
    } catch (...) { }
  }
  py_ptr = model->extract_features(X, options);

  std::shared_ptr<unity_sarray> extracted = std::make_shared<unity_sarray>();
  extracted->construct_from_sarray(py_ptr);
  ret_status.params["model"] = to_variant(model);
  ret_status.params["extracted"] = to_variant(extracted);
  ret_status.success = true;
  return ret_status;
}



/**
 * Init function for predict.
 */
toolkit_function_response_type predict(toolkit_function_invocation& invoke){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");
  std::string output_type = variant_get_value<flexible_type>(invoke.params["output_type"]);

  // Fill in missing columns
  // --------------------------------------------------------------------------
  toolkit_function_response_type ret_status;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            invoke.params, "dataset")->get_underlying_sframe());

  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);

  std::shared_ptr<sarray<flexible_type>> py_ptr;

  ml_data data = setup_ml_data_for_prediction(X, model, missing_value_action);
  py_ptr = model->predict(data, output_type);

  std::shared_ptr<unity_sarray> predicted = std::make_shared<unity_sarray>();
  predicted->construct_from_sarray(py_ptr);

  ret_status.params["model"] = to_variant(model);
  ret_status.params["predicted"] = to_variant(predicted);
  ret_status.success = true;
  return ret_status;
}

/**
 * Init function for predict_topk.
 */
toolkit_function_response_type predict_topk(toolkit_function_invocation& invoke){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");
  std::string output_type = variant_get_value<flexible_type>(invoke.params["output_type"]);

  int topk = variant_get_value<flexible_type>(invoke.params["topk"]);
  if (topk <= 0) log_and_throw("The parameter 'k' must be positive.");

  // Fill in missing columns
  // --------------------------------------------------------------------------
  toolkit_function_response_type ret_status;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            invoke.params, "dataset")->get_underlying_sframe());

  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);
  sframe py_ptr;

  ml_data data = setup_ml_data_for_prediction(X, model, missing_value_action);
  py_ptr = model->predict_topk(data, output_type, topk);

  std::shared_ptr<unity_sframe> predicted = std::make_shared<unity_sframe>();
  predicted->construct_from_sframe(py_ptr);

  ret_status.params["model"] = to_variant(model);
  ret_status.params["predicted"] = to_variant(predicted);
  ret_status.success = true;
  return ret_status;
}

/**
 * Init function for predict.
 */
toolkit_function_response_type classify(toolkit_function_invocation& invoke) {
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");

  // Fill in missing columns
  // --------------------------------------------------------------------------
  toolkit_function_response_type ret_status;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            invoke.params, "dataset")->get_underlying_sframe());
  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);

  sframe classify_sf;

  ml_data data = setup_ml_data_for_prediction(X, model, missing_value_action);
  classify_sf = model->classify(data);

  std::shared_ptr<unity_sframe> classify_out(new unity_sframe());
  classify_out->construct_from_sframe(classify_sf);
  ret_status.params["model"] = to_variant(model);
  ret_status.params["classify"] = to_variant(classify_out);
  ret_status.success = true;
  return ret_status;
}



/**
 * Init function for evaluate.
 */
toolkit_function_response_type evaluate(toolkit_function_invocation& invoke){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(invoke.params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");
  std::string model_name = model->name();

  // Check types for the data & filter out the columns that we don't need
  // --------------------------------------------------------------------------
  toolkit_function_response_type ret_status;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            invoke.params, "dataset")->get_underlying_sframe());

  std::string target_name = model->get_target_name();

  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);
  sframe y = test_data.select_columns({target_name});

  variant_map_type results;
  ml_data data = setup_ml_data_for_evaluation(X, y, model, missing_value_action);
  results = model->evaluate(data,
      variant_get_value<std::string>(invoke.params["metric"]));

  ret_status.params = results;
  ret_status.success = true;
  return ret_status;
}


/**
 * List keys
 */
toolkit_function_response_type list_keys(toolkit_function_invocation& invoke){
  log_func_entry();
  toolkit_function_response_type ret_status;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");

  for (const auto& k: model->list_keys()) {
    ret_status.params[k] = "";
  }
  ret_status.success = true;
  return ret_status;
}

/**
 * Get the value of anything from the model
 */
toolkit_function_response_type get_value(toolkit_function_invocation& invoke){
  log_func_entry();
  toolkit_function_response_type ret_status;

  flexible_type field = variant_get_value<flexible_type>(invoke.params["field"]);
  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");

  ret_status.params["value"] = model->get_value_from_state(field);
  ret_status.success = true;
  return ret_status;
}

/**
 * Get the value of a particular option.
 */
toolkit_function_response_type get_option_value(toolkit_function_invocation& invoke){
  log_func_entry();
  toolkit_function_response_type ret_status;

  flexible_type field = variant_get_value<flexible_type>(invoke.params["field"]);
  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");

  ret_status.params["value"] = model->get_option_value(field);
  ret_status.success = true;
  return ret_status;
}


/**
 * Get the option dictionary.
 */
toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke){
  log_func_entry();
  toolkit_function_response_type ret_status;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");

  for (const auto& opt : model->get_current_options()) {
    ret_status.params[opt.first] = opt.second;
  }
  ret_status.success = true;
  return ret_status;
}

/**
 * Get the training stats dictionary.
 */
toolkit_function_response_type get_train_stats(toolkit_function_invocation& invoke){
  log_func_entry();
  toolkit_function_response_type ret_status;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");

  for (const auto& opt : model->get_train_stats()) {
    ret_status.params[opt.first] = opt.second;
  }
  ret_status.success = true;
  return ret_status;
}



/**
 * Check if the model is trained.
 */
toolkit_function_response_type is_trained(toolkit_function_invocation& invoke){
  log_func_entry();
  toolkit_function_response_type ret_status;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");

  ret_status.params["is_trained"] = model->is_trained();
  ret_status.success = true;
  return ret_status;
}


/**
 * Add or Append to the key value pair of the model
 */
toolkit_function_response_type add_or_update_state(toolkit_function_invocation& invoke){
  log_func_entry();
  toolkit_function_response_type ret_status;

  std::string key = variant_get_value<flexible_type>(invoke.params["key"]);
  flexible_type value = variant_get_value<flexible_type>(invoke.params["value"]);
  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(invoke, "model");


  model->add_or_update_state({{key, value}});
  ret_status.params["model"] = to_variant(model);
  ret_status.success = true;
  return ret_status;

}


/**
 * Obtains the registration for the svm Toolkit.
 */
EXPORT std::vector<toolkit_function_specification> get_toolkit_function_registration() {
  log_func_entry();


  std::vector<toolkit_function_specification>  specs;
  toolkit_function_specification supervised_learning_train_spec;
  supervised_learning_train_spec.name = "supervised_learning_train";
  supervised_learning_train_spec.toolkit_execute_function = train;

  toolkit_function_specification supervised_learning_feature_extraction_spec;
  supervised_learning_feature_extraction_spec.name = "supervised_learning_feature_extraction";
  supervised_learning_feature_extraction_spec.toolkit_execute_function = extract_feature; 

  toolkit_function_specification supervised_learning_predict_spec;
  supervised_learning_predict_spec.name = "supervised_learning_predict";
  supervised_learning_predict_spec.toolkit_execute_function = predict;

  toolkit_function_specification supervised_learning_predict_topk_spec;
  supervised_learning_predict_topk_spec.name = "supervised_learning_predict_topk";
  supervised_learning_predict_topk_spec.toolkit_execute_function = predict_topk;

  toolkit_function_specification supervised_learning_classify_spec;
  supervised_learning_classify_spec.name = "supervised_learning_classify";
  supervised_learning_classify_spec.toolkit_execute_function = classify;

  toolkit_function_specification supervised_learning_evaluate_spec;
  supervised_learning_evaluate_spec.name = "supervised_learning_evaluate";
  supervised_learning_evaluate_spec.toolkit_execute_function = evaluate;

  toolkit_function_specification supervised_learning_get_train_stats_spec;
  supervised_learning_get_train_stats_spec.name = "supervised_learning_get_train_stats";
  supervised_learning_get_train_stats_spec.toolkit_execute_function = get_train_stats;

  toolkit_function_specification supervised_learning_get_current_options_spec;
  supervised_learning_get_current_options_spec.name = "supervised_learning_get_current_options";
  supervised_learning_get_current_options_spec.toolkit_execute_function
    = get_current_options;

  toolkit_function_specification supervised_learning_get_option_value_spec;
  supervised_learning_get_option_value_spec.name = "supervised_learning_get_option_value";
  supervised_learning_get_option_value_spec.toolkit_execute_function = get_option_value;

  toolkit_function_specification supervised_learning_get_value_spec;
  supervised_learning_get_value_spec.name = "supervised_learning_get_value";
  supervised_learning_get_value_spec.toolkit_execute_function = get_value;

  toolkit_function_specification supervised_learning_list_keys_spec;
  supervised_learning_list_keys_spec.name = "supervised_learning_list_keys";
  supervised_learning_list_keys_spec.toolkit_execute_function = list_keys;

  toolkit_function_specification supervised_learning_is_trained_spec;
  supervised_learning_is_trained_spec.name = "supervised_learning_is_trained";
  supervised_learning_is_trained_spec.toolkit_execute_function = is_trained;

  toolkit_function_specification supervised_learning_add_or_update_state_spec;
  supervised_learning_add_or_update_state_spec.name = "supervised_learning_add_or_update_state";
  supervised_learning_add_or_update_state_spec.toolkit_execute_function
    = add_or_update_state;

  // FIXME: This hack is a roundabout because the toolkit SDK and the
  // unity function registration can't both be done in the same namespace.
  specs.push_back(supervised_learning_train_spec);
  specs.push_back(supervised_learning_predict_spec);
  specs.push_back(supervised_learning_classify_spec);
  specs.push_back(supervised_learning_predict_topk_spec);
  specs.push_back(supervised_learning_evaluate_spec);
  specs.push_back(supervised_learning_get_train_stats_spec);
  specs.push_back(supervised_learning_get_current_options_spec);
  specs.push_back(supervised_learning_get_value_spec);
  specs.push_back(supervised_learning_add_or_update_state_spec);
  specs.push_back(supervised_learning_list_keys_spec);
  specs.push_back(supervised_learning_get_option_value_spec);
  specs.push_back(supervised_learning_feature_extraction_spec);
  REGISTER_FUNCTION(_fast_predict, "model", "rows", "output_type", 
                        "missing_value_action");
  REGISTER_FUNCTION(_fast_predict_topk, "model", "rows", "output_type", 
                        "missing_value_action", "topk");
  REGISTER_FUNCTION(_fast_classify, "model", "rows", 
                        "missing_value_action");
  REGISTER_FUNCTION(_regression_model_selector, "_X");
  REGISTER_FUNCTION(_classifier_model_selector, "_X");
  REGISTER_FUNCTION(_classifier_available_models, "num_classes", "_X");
  REGISTER_FUNCTION(_get_metadata_mapping, "model");

  return specs;

}

} // supervised
} // turicreate
