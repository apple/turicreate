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
 * \param[in] params  Parameters dictionary
 * \param[in] key     The key in params
 */
std::shared_ptr<supervised_learning_model_base> 
      get_supervised_learning_model(variant_map_type& params,
                                   std::string model_key){
  DASSERT_TRUE(params.count("model_name") > 0);
  std::shared_ptr<supervised_learning_model_base> model;
  model = safe_varmap_get<std::shared_ptr<supervised_learning_model_base>>( 
                                params, model_key);

  // This should not happen.
  if (model == NULL) {
    std::string model_name 
      = (std::string)safe_varmap_get<flexible_type>(params, "model_name");
    log_and_throw("Internal error: " + model_name + 
                                      " is not a supervised learning model.");
  }
  return model;
}


/*
 * Train function init.
 */
variant_map_type train(variant_map_type& params) {
  log_func_entry();
  DASSERT_TRUE(params.count("model_name") > 0);
  DASSERT_TRUE(params.count("target") > 0);
  DASSERT_TRUE(params.count("features") > 0);

  // Get data from Python. 
  sframe X
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            params, "features")->get_underlying_sframe());
  sframe y
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            params, "target")->get_underlying_sframe());
  std::string model_name 
    = safe_varmap_get<std::string>(params, "model_name");

 
  // Remove option names that are not needed.
  variant_map_type kwargs = params;
  kwargs.erase("model_name");
  kwargs.erase("target");
  kwargs.erase("features");

  // Train a model.
  std::shared_ptr<supervised_learning_model_base> model = create(
                          X, y, model_name, kwargs);

  // Return options and model objects.
  variant_map_type ret;
  ret["model"] = to_variant(model);
  return ret;
}

/**
 * Init function for extract_feature.
 */
variant_map_type extract_feature(variant_map_type& params){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(params,
        "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model =
    get_supervised_learning_model(params, "model");
  std::string model_name = model->name();

  // Extract the features
  // --------------------------------------------------------------------------
  variant_map_type ret;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            params, "dataset")->get_underlying_sframe());
  
  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);
  std::shared_ptr<sarray<flexible_type>> py_ptr;

  std::map<std::string, flexible_type> options;
  for (const auto& kv: params) {
    try {
      options[kv.first] = safe_varmap_get<flexible_type>(params, kv.first);
    } catch (...) { }
  }
  py_ptr = model->extract_features(X, options);

  std::shared_ptr<unity_sarray> extracted = std::make_shared<unity_sarray>();
  extracted->construct_from_sarray(py_ptr);
  ret["model"] = to_variant(model);
  ret["extracted"] = to_variant(extracted);
  return ret;
}



/**
 * Init function for predict.
 */
variant_map_type predict(variant_map_type& params){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");
  std::string output_type = variant_get_value<flexible_type>(params["output_type"]);

  // Fill in missing columns
  // --------------------------------------------------------------------------
  variant_map_type ret;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            params, "dataset")->get_underlying_sframe());

  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);

  std::shared_ptr<sarray<flexible_type>> py_ptr;

  ml_data data = setup_ml_data_for_prediction(X, model, missing_value_action);
  py_ptr = model->predict(data, output_type);

  std::shared_ptr<unity_sarray> predicted = std::make_shared<unity_sarray>();
  predicted->construct_from_sarray(py_ptr);

  ret["model"] = to_variant(model);
  ret["predicted"] = to_variant(predicted);
  return ret;
}

/**
 * Init function for predict_topk.
 */
variant_map_type predict_topk(variant_map_type& params){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");
  std::string output_type = variant_get_value<flexible_type>(params["output_type"]);

  int topk = variant_get_value<flexible_type>(params["topk"]);
  if (topk <= 0) log_and_throw("The parameter 'k' must be positive.");

  // Fill in missing columns
  // --------------------------------------------------------------------------
  variant_map_type ret;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            params, "dataset")->get_underlying_sframe());

  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);
  sframe py_ptr;

  ml_data data = setup_ml_data_for_prediction(X, model, missing_value_action);
  py_ptr = model->predict_topk(data, output_type, topk);

  std::shared_ptr<unity_sframe> predicted = std::make_shared<unity_sframe>();
  predicted->construct_from_sframe(py_ptr);

  ret["model"] = to_variant(model);
  ret["predicted"] = to_variant(predicted);
  return ret;
}

/**
 * Init function for predict.
 */
variant_map_type classify(variant_map_type& params) {
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");

  // Fill in missing columns
  // --------------------------------------------------------------------------
  variant_map_type ret;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            params, "dataset")->get_underlying_sframe());
  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);

  sframe classify_sf;

  ml_data data = setup_ml_data_for_prediction(X, model, missing_value_action);
  classify_sf = model->classify(data);

  std::shared_ptr<unity_sframe> classify_out(new unity_sframe());
  classify_out->construct_from_sframe(classify_sf);
  ret["model"] = to_variant(model);
  ret["classify"] = to_variant(classify_out);
  return ret;
}



/**
 * Init function for evaluate.
 */
variant_map_type evaluate(variant_map_type& params){
  log_func_entry();

  // From Python
  // --------------------------------------------------------------------------
  std::string missing_value_action_str
    = (std::string)safe_varmap_get<flexible_type>(params, "missing_value_action");
  ml_missing_value_action missing_value_action = 
      get_missing_value_enum_from_string(missing_value_action_str);

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");
  std::string model_name = model->name();

  // Check types for the data & filter out the columns that we don't need
  // --------------------------------------------------------------------------
  variant_map_type ret;
  std::stringstream ss;
  sframe test_data
    = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
            params, "dataset")->get_underlying_sframe());

  std::string target_name = model->get_target_name();

  sframe X = setup_test_data_sframe(test_data, model, missing_value_action);
  sframe y = test_data.select_columns({target_name});

  variant_map_type results;
  ml_data data = setup_ml_data_for_evaluation(X, y, model, missing_value_action);
  results = model->evaluate(data,
      variant_get_value<std::string>(params["metric"]));

  ret = results;
  return ret;
}


/**
 * List keys
 */
variant_map_type list_fields(variant_map_type& params){
  log_func_entry();
  variant_map_type ret;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");

  for (const auto& k: model->list_fields()) {
    ret[k] = "";
  }
  return ret;
}

/**
 * Get the value of anything from the model
 */
variant_map_type get_value(variant_map_type& params){
  log_func_entry();
  variant_map_type ret;

  flexible_type field = variant_get_value<flexible_type>(params["field"]);
  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");

  ret["value"] = model->get_value_from_state(field);
  return ret;
}

/**
 * Get the value of a particular option.
 */
variant_map_type get_option_value(variant_map_type& params){
  log_func_entry();
  variant_map_type ret;

  flexible_type field = variant_get_value<flexible_type>(params["field"]);
  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");

  ret["value"] = model->get_option_value(field);
  return ret;
}


/**
 * Get the option dictionary.
 */
variant_map_type get_current_options(variant_map_type& params){
  log_func_entry();
  variant_map_type ret;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");

  for (const auto& opt : model->get_current_options()) {
    ret[opt.first] = opt.second;
  }
  return ret;
}

/**
 * Get the training stats dictionary.
 */
variant_map_type get_train_stats(variant_map_type& params){
  log_func_entry();
  variant_map_type ret;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");

  for (const auto& opt : model->get_train_stats()) {
    ret[opt.first] = opt.second;
  }
  return ret;
}



/**
 * Check if the model is trained.
 */
variant_map_type is_trained(variant_map_type& params){
  log_func_entry();
  variant_map_type ret;

  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");

  ret["is_trained"] = model->is_trained();
  return ret;
}


/**
 * Add or Append to the key value pair of the model
 */
variant_map_type add_or_update_state(variant_map_type& params){
  log_func_entry();
  variant_map_type ret;

  std::string key = variant_get_value<flexible_type>(params["key"]);
  flexible_type value = variant_get_value<flexible_type>(params["value"]);
  std::shared_ptr<supervised_learning_model_base> model
                          = get_supervised_learning_model(params, "model");


  model->add_or_update_state({{key, value}});
  ret["model"] = to_variant(model);
  return ret;

}


/**
 * Defines get_toolkit_function_registration for the supervised_learning toolkit
 */
BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(train, "params");
REGISTER_FUNCTION(predict, "params");
REGISTER_FUNCTION(classify, "params");
REGISTER_FUNCTION(predict_topk, "params");
REGISTER_FUNCTION(evaluate, "params");
REGISTER_FUNCTION(get_train_stats, "params");
REGISTER_FUNCTION(get_current_options, "params");
REGISTER_FUNCTION(get_value, "params");
REGISTER_FUNCTION(is_trained, "params");
REGISTER_FUNCTION(add_or_update_state, "params");
REGISTER_FUNCTION(list_fields, "params");
REGISTER_FUNCTION(get_option_value, "params");
REGISTER_FUNCTION(extract_feature, "params");
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
END_FUNCTION_REGISTRATION

} // supervised
} // turicreate
