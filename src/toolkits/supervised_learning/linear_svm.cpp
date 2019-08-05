/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// ML Data
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

#include <core/storage/sframe_data/algorithm.hpp>
#include <ml/ml_data/metadata.hpp>

// Toolkits
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <toolkits/supervised_learning/linear_svm_opt_interface.hpp>

// CoreML
#include <toolkits/coreml_export/linear_models_exporter.hpp>


// Solvers
#include <ml/optimization/utils.hpp>
#include <ml/optimization/constraints-inl.hpp>
#include <limits>

// Regularizer
#include <ml/optimization/regularizers-inl.hpp>
#include <ml/optimization/lbfgs.hpp>
#include <ml/optimization/newton_method-inl.hpp>
#include <ml/optimization/accelerated_gradient-inl.hpp>

// Utilities
#include <Eigen/SparseCore>
#include <cmath>


namespace turi {
namespace supervised {

/*
 * Linear SVM Model
 * ****************************************************************************
 */


/**
 * Destructor. Make sure bad things don't happen
 */
linear_svm::~linear_svm(){
}

/**
 * SVM supervised_learning init.
 */
void linear_svm::model_specific_init(const ml_data& data,
                                     const ml_data& valid_data) {

  if (ml_mdata->target_index_size() != 2){
    std::stringstream ss;
    ss << "Linear SVM currently only supports binary classification. "
       << "Use the boosted_trees_classifier for multi-class classification."
       << std::endl;
    log_and_throw(ss.str());
  }

  // Count the number of variables
  // --------------------------------------------------------------------------
  // Create an interface to the solver.
  size_t variables = get_number_of_coefficients(ml_mdata);

  // Examples per class
  state["num_classes"] = 2;
  state["num_examples_per_class"] =
                    to_variant(get_num_examples_per_class(ml_mdata));
  state["num_coefficients"] = variables;

  // Create an interface to the solver.
  // --------------------------------------------------------------------------
  scaled_logistic_svm_interface.reset(new
      linear_svm_scaled_logistic_opt_interface(data, valid_data, *this));
  coefs = DenseVector::Zero(variables);
}


/**
 * Set options
 */
void linear_svm::init_options(const std::map<std::string,
    flexible_type>&_opts){

  options.create_real_option(
      "convergence_threshold",
      "Convergence threshold for training",
      0.01,
      optimization::OPTIMIZATION_ZERO,
      optimization::OPTIMIZATION_INFTY,
      false);

  options.create_integer_option(
      "max_iterations",
      "Maximum number of iterations to perform during training",
      10,
      1,
      std::numeric_limits<int>::max(),
      false);

  options.create_categorical_option(
      "solver",
      "Solver used for training",
      "auto",
      {flexible_type("auto"), flexible_type("lbfgs")},
      false);

  options.create_real_option(
      "penalty",
      "Penalty on the mis-classification loss",
      1.0,
      optimization::OPTIMIZATION_ZERO,
      optimization::OPTIMIZATION_INFTY,
      false);

  options.create_integer_option(
      "lbfgs_memory_level",
      "Number of previous iterations to cache for LBFGS",
      11,
      1,
      std::numeric_limits<int>::max(),
      false);

  options.create_boolean_option(
      "feature_rescaling",
      "Rescale features to have unit L2-Norm",
      true,
      false);

  options.create_flexible_type_option(
      "class_weights",
      "Weights (during training) assigned to each class.",
      flex_undefined(),
      true);

  options.create_boolean_option(
      "disable_posttrain_evaluation",
      "Disable automatic computation of an evaluation report following training.",
      false,
      false);

  // Set options!
  options.set_options(_opts);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));

}


/**
 * Train a linear svm model.
 */
void linear_svm::train() {
  DASSERT_TRUE(scaled_logistic_svm_interface->num_variables() >= 0);
  if(get_option_value("feature_rescaling")){
    scaled_logistic_svm_interface->init_feature_rescaling();
  }

  // Set class weights
  flexible_type class_weights =
                      get_class_weights_from_options(options, ml_mdata);
  state["class_weights"] =  to_variant(class_weights);
  size_t i = 0;
  flex_dict _class_weights(variant_get_value<size_t>(state.at("num_classes")));
  for(const auto& kvp: class_weights.get<flex_dict>()){
    _class_weights[i++] =
        {ml_mdata->target_indexer()->immutable_map_value_to_index(kvp.first),
         kvp.second.get<flex_float>()};
  }
  scaled_logistic_svm_interface->set_class_weights(_class_weights);

  display_classifier_training_summary("SVM");
  size_t variables =  variant_get_value<size_t>(state.at("num_coefficients"));
  logprogress_stream << "Number of coefficients    : " << variables << std::endl;


  // Set the initial point and write initial output to screen
  // ---------------------------------------------------------------------------
  DenseVector init_point(variables);
  init_point.setZero();

  // Box constriants for L1 loss SVM
  float penalty = options.value("penalty");
  std::string solver = options.value("solver");

  // Auto solver
  if(solver == "auto"){
    solver = "lbfgs";
  }
  this->set_options({{"solver", solver}});

  // Call the solvers
  // ---------------------------------------------------------------------------
  optimization::solver_return stats;
  DenseVector is_regularized(variables);
  is_regularized.setOnes();
  is_regularized(variables - 1) = 0;

  // Set the regularization penalty
  DenseVector beta = is_regularized * 0.5 / penalty;
  std::shared_ptr<optimization::smooth_regularizer_interface> smooth_reg;
  smooth_reg.reset(new optimization::l2_norm(beta));

  // Make a copy of the options to use as solver options.
  std::map<std::string, flexible_type> solver_opts
    = options.current_option_values();
  if (solver == "lbfgs") {
    stats = turi::optimization::lbfgs_compat(scaled_logistic_svm_interface, init_point,
        solver_opts, smooth_reg);
  } else {
    std::ostringstream msg;
    msg << "Solver " << solver << " is not supported." << std::endl;
    msg << "Supported solvers are (auto, lbfgs)" << std::endl;
    log_and_throw(msg.str());
  }

  // Save final accuracies
  if(scaled_logistic_svm_interface->num_validation_examples() > 0) {
    state["validation_accuracy"] = scaled_logistic_svm_interface->get_validation_accuracy();
  }
  state["training_accuracy"] = scaled_logistic_svm_interface->get_training_accuracy();

  // Store the coefficients in the model
  // ---------------------------------------------------------------------------
  coefs = stats.solution;
  scaled_logistic_svm_interface->rescale_solution(coefs);
  sframe sf_coef = get_coefficients_as_sframe(coefs, ml_mdata);
  std::shared_ptr<unity_sframe> unity_coef = std::make_shared<unity_sframe>();
  unity_coef->construct_from_sframe(sf_coef);
  state["coefficients"] = to_variant(unity_coef);

  // Copy the training stats into the model
  // ---------------------------------------------------------------------------
  state["training_iterations"] = stats.iters;
  state["training_time"] = stats.solve_time;
  state["training_loss"] = stats.func_value;  // minimized negative log-likelihood
  state["training_solver_status"] = (std::string)
    translate_solver_status(stats.status);

  // Store progress table.
  std::shared_ptr<unity_sframe> unity_progress = std::make_shared<unity_sframe>();
  unity_progress->construct_from_sframe(stats.progress_table);
  state["progress"] = to_variant(unity_progress);

  smooth_reg.reset();

}


/**
 * Predict for a single example.
 */
flexible_type linear_svm::predict_single_example(
         const DenseVector& x,
         const prediction_type_enum& output_type){
  double margin = x.dot(coefs);
  switch (output_type) {
    // Margin
    case prediction_type_enum::MARGIN:
      return margin;

    // Class Index
    case prediction_type_enum::CLASS_INDEX:
      return (margin >= 0.0);

    // Class
    case prediction_type_enum::NA:
    case prediction_type_enum::CLASS:
    {
      size_t class_id = (margin >= 0.0);
      return  ml_mdata->target_indexer()->map_index_to_value(class_id);
    }

    // Not supported types
    case prediction_type_enum::PROBABILITY:
    case prediction_type_enum::MAX_PROBABILITY:
    case prediction_type_enum::RANK:
    case prediction_type_enum::PROBABILITY_VECTOR:
      log_and_throw("Output type not supported.");

  }

  log_and_throw(std::string("Configuration not supported"));
  ASSERT_UNREACHABLE();
}

/**
 * Predict for a single example.
 */
flexible_type linear_svm::predict_single_example(
         const SparseVector& x,
         const prediction_type_enum& output_type){
  double margin = x.dot(coefs);
  switch (output_type) {
    // Margin
    case prediction_type_enum::MARGIN:
      return margin;

    // Class Index
      case prediction_type_enum::CLASS_INDEX:
      return (margin >= 0.0);
    // Class
    case prediction_type_enum::NA:
    case prediction_type_enum::CLASS:
    {
      size_t class_id = (margin >= 0.0);
      return  ml_mdata->target_indexer()->map_index_to_value(class_id);
    }

    // Not supported
    case prediction_type_enum::PROBABILITY:
    case prediction_type_enum::MAX_PROBABILITY:
    case prediction_type_enum::RANK:
    case prediction_type_enum::PROBABILITY_VECTOR:
      log_and_throw("Output type not supported.");
  }

  log_and_throw(std::string("Configuration not supported"));
  ASSERT_UNREACHABLE();
}


/**
 * Classify.
 */
sframe linear_svm::classify(const ml_data& test_data,
                            const std::string& output_type){
  sframe sf_class;
  sf_class = sf_class.add_column(predict(test_data, "class"), "class");
  return sf_class;
}

/**
 * Fast Classify.
 */
gl_sframe linear_svm::fast_classify(
    const std::vector<flexible_type>& rows,
    const std::string& missing_value_action) {
  // Class predictions
  gl_sframe sf_class;
  sf_class.add_column(fast_predict(rows, missing_value_action, "class"),
		      "class");
  return sf_class;
}

/**
 * Turi Serialization Save
 */
void linear_svm::save_impl(turi::oarchive& oarc) const {

  // State
  variant_deep_save(state, oarc);

  // Everything else
  oarc << ml_mdata
       << metrics
       << coefs
       << options;

}

/**
 * Setter for coefficients vector.
 */
void linear_svm::set_coefs(const DenseVector& _coefs) {
  coefs = _coefs;
}


/**
 * Turi Serialization Load
 */
void linear_svm::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_MSG(version <= SVM_MODEL_VERSION,
        "This model version cannot be loaded. Please re-save your model.");
  if (version < 5) {
    log_and_throw("Cannot load a model saved using a version prior to GLC-1.7.");
  }

  // State
  variant_deep_load(state, iarc);

  // Everything else
  // GLC 1.3-
  iarc >> ml_mdata
       >> metrics
       >> coefs
       >> options;

  // GLC 1.7
  if (version < 5) {
    tracking_metrics = metrics;
    this->set_default_evaluation_metric();
  }
}

/**
 * Gets the model version number
 */
size_t linear_svm::get_version() const{
  // Version translator
  // -----------------------
  //  0 - Version 1.0
  //  1 - Version 1.1
  //  2 - Version 1.3
  //  3 - Version 1.4
  //  4 - Version 1.5
  //  5 - Version 1.7
  return SVM_MODEL_VERSION;
}

std::shared_ptr<coreml::MLModelWrapper> linear_svm::export_to_coreml() {

  std::map<std::string, flexible_type> context = {
    {"model_type", "linear_svm"},
    {"version", std::to_string(get_version())},
    {"class", name()},
    {"short_description", "Linear SVM Model."}};

  return export_linear_svm_as_model_asset(ml_mdata, coefs, context);
}

} // supervised
} // turicreate
