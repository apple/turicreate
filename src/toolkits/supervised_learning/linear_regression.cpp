/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_interface/unity_sframe.hpp>

// ML Data
#include <ml/ml_data/ml_data.hpp>

// Core ML
#include <toolkits/coreml_export/linear_models_exporter.hpp>

// Toolkits
#include <toolkits/supervised_learning/linear_regression_opt_interface.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>

// Solvers
#include <ml/optimization/utils.hpp>
#include <ml/optimization/newton_method-inl.hpp>
#include <ml/optimization/gradient_descent-inl.hpp>
#include <ml/optimization/accelerated_gradient-inl.hpp>
#include <ml/optimization/lbfgs.hpp>

// Regularizer
#include <ml/optimization/regularizers-inl.hpp>

#include <Eigen/SparseCore>

constexpr size_t LINEAR_REGRESSION_NEWTON_VARIABLES_HARD_LIMIT = 10000;
constexpr size_t LINEAR_REGRESSION_NEWTON_VARIABLES_SOFT_LIMIT = 500;

// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 1. Parallel Predict function.
// 2. Supporting stats (p-values, std errors etc.)
// 3. Add a warning for Hessian computation if it can't fit in memory.


namespace turi {
namespace supervised {



/*
 * Linear Regression Model
 * ****************************************************************************
 */


/**
 * Destructor. Make sure bad things don't happen
 */
linear_regression::~linear_regression(){
  lr_interface.reset();
}

/**
 * Init function common to all regression inits.
 */
void linear_regression::model_specific_init(const ml_data& data,
                                            const ml_data& valid_data){

  // Create an interface to the solver.
  size_t variables = get_number_of_coefficients(this->ml_mdata);

  // Update the model
  state["num_coefficients"] =  variables;

  // Initialize the solver
  lr_interface.reset(new linear_regression_opt_interface(data, valid_data, *this));
}


/**
 * Set options
 */
void linear_regression::init_options(const std::map<std::string,
    flexible_type>&_opts){

  options.create_real_option(
      "convergence_threshold",
      "Convergence threshold for training",
      0.01,
      optimization::OPTIMIZATION_ZERO,
      optimization::OPTIMIZATION_INFTY,
      false);

  options.create_real_option(
      "step_size",
      "Guess for the initial step size for the solver",
      1.0,
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

  options.create_boolean_option(
      "feature_rescaling",
      "Rescale features to have unit L2-Norm",
      true,
      false);

  options.create_integer_option(
      "lbfgs_memory_level",
      "Number of previous iterations to cache for LBFGS",
      11,
      1,
      std::numeric_limits<int>::max(),
      false);

  options.create_categorical_option(
      "solver",
      "Solver used for training",
      "auto",
      {flexible_type("auto"), flexible_type("newton"), flexible_type("lbfgs"),
      flexible_type("fista")},
      false);

  options.create_real_option(
      "l1_penalty",
      "Penalty on the L1-penalty",
      0,
      0,
      optimization::OPTIMIZATION_INFTY,
      false);

  options.create_real_option(
      "l2_penalty",
      "Penalty on the L2-penalty",
      0.01,
      0,
      optimization::OPTIMIZATION_INFTY,
      false);

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
 * Train a linear regression model.
 */
void linear_regression::train(){

  // Set the rescaling
  if(get_option_value("feature_rescaling")){
    lr_interface->init_feature_rescaling();
  }

  // Step 1: Set the starting point
  // -------------------------------------------------------------------------
  size_t examples = num_examples();
  size_t variables =  variant_get_value<size_t>(state.at("num_coefficients"));
  DenseVector init_point(variables);
  init_point.setZero();

  display_regression_training_summary("Linear regression");
  logprogress_stream << "Number of coefficients    : " << variables << std::endl;

  // Step 2: Deal with regularizers
  // -------------------------------------------------------------------------
  // Regularize all variables except the bias term.
  DenseVector is_regularized(variables);
  is_regularized.setOnes();
  is_regularized(variables-1) = 0;

  // Set the penalties for the regularizer.
  // L1 or L2 or both?
  float l1_penalty = get_option_value("l1_penalty");
  float l2_penalty = get_option_value("l2_penalty");


  DenseVector alpha = is_regularized * l1_penalty;
  DenseVector beta = is_regularized * l2_penalty;

  // L2 is continuous
  std::shared_ptr<optimization::regularizer_interface> reg;
  std::shared_ptr<optimization::smooth_regularizer_interface> smooth_reg;

  if (l1_penalty < optimization::OPTIMIZATION_ZERO){
    smooth_reg.reset(new optimization::l2_norm(beta));
    reg = smooth_reg;
  } else {
    reg.reset(new optimization::elastic_net(alpha, beta));
  }

  // Step 3: Call the solvers.
  // -------------------------------------------------------------------------
  std::string solver =  get_option_value("solver");

  // Auto solver
  // \note Currently, we do not incorporate dataset sparsity while selecting
  // the datasets. We should store a "sparsity index" in ml_metadata to give
  // us a sense of how sparse the dataset is. Ideally, all the rules are
  // going to be heavily dependant on sparsity. Right now, we will assume
  // that all "fat" datasets are always sparse.
  if(solver == "auto"){
    if(variables > LINEAR_REGRESSION_NEWTON_VARIABLES_SOFT_LIMIT){
      if(l1_penalty > optimization::OPTIMIZATION_ZERO){
        solver = "fista";
      } else {
        solver = "lbfgs";
      }
    } else {
      if(l1_penalty > optimization::OPTIMIZATION_ZERO){
        solver = "fista";
      } else {
        solver = "newton";
      }
    }
  }
  this->set_options({{"solver", solver}});

  std::stringstream ss;
  if (l1_penalty > optimization::OPTIMIZATION_ZERO &&
      (solver ==  "newton" || solver == "lbfgs")){
    ss << "Solver '" << solver
       << "' not compatible with L1-regularization. "
       << "Try using the option solver='fista'."
       << std::endl;
    log_and_throw(ss.str());
  }


  // To prevent Newton method from crashing
  if(solver == "newton" &&
                variables > LINEAR_REGRESSION_NEWTON_VARIABLES_HARD_LIMIT){
    ss << "Number of coefficients is too large for Newton method. "
       << "Try using the option solver='lbfgs'."
       << std::endl;
    log_and_throw(ss.str());
  }

  optimization::solver_return stats;
  std::map<std::string, flexible_type> solver_options
    = options.current_option_values();
  if (solver == "newton"){
    stats = turi::optimization::newton_method(*lr_interface, init_point,
        solver_options, smooth_reg);
  } else if (solver == "fista"){
    stats = turi::optimization::accelerated_gradient(*lr_interface,
        init_point, solver_options, reg);
  } else if (solver == "lbfgs"){
      stats = turi::optimization::lbfgs_compat(lr_interface, init_point,
          solver_options, smooth_reg);
  } else {
      std::ostringstream msg;
      msg << "Solver " << solver << " is not supported." << std::endl;
      msg << "Supported solvers are (auto, newton, lbfgs, fista)" << std::endl;
      log_and_throw(msg.str());
  }

  // Step 4: Store the coefficients
  // -------------------------------------------------------------------------
  coefs = stats.solution;
  lr_interface->rescale_solution(coefs);
  bool has_stderr = (stats.hessian.rows() * stats.hessian.cols() > 0) &&
    (examples > variables);
  if (has_stderr) {
    double scale = 2 * stats.func_value / (examples - variables);
    std_err = get_stderr_from_hessian(stats.hessian) * sqrt(scale);
    DASSERT_EQ(std_err.size(), coefs.size());
    lr_interface->rescale_solution(std_err);
  }

  // Save coefs to SFrame.
  sframe sf_coef = get_coefficients_as_sframe(this->coefs, this->ml_mdata,
      this->std_err);
  if(!has_stderr) {
    sf_coef = add_na_std_err_to_coef(sf_coef);
  }
  std::shared_ptr<unity_sframe> unity_coef = std::make_shared<unity_sframe>();
  unity_coef->construct_from_sframe(sf_coef);
  state["coefficients"] = to_variant(unity_coef);

  // Step 5: Store the training stats.
  // -------------------------------------------------------------------------
  state["training_iterations"] =  stats.iters;
  state["training_time"] =  stats.solve_time;
  state["training_loss"] =  stats.func_value;
  state["training_rmse"] =  sqrt((stats.func_value)/examples);
  state["training_solver_status"] = (std::string)
                                      translate_solver_status(stats.status);

  // Store progress table.
  std::shared_ptr<unity_sframe> unity_progress = std::make_shared<unity_sframe>();
  unity_progress->construct_from_sframe(stats.progress_table);
  state["progress"] = to_variant(unity_progress);

  // Compute validation-set stats.
  if (lr_interface->num_validation_examples() > 0) {
    // Recycle lvalues from stats to use as out parameters here, now that we're
    // otherwise done reading from stats.
    lr_interface->compute_validation_first_order_statistics(
        stats.solution, stats.gradient, stats.func_value);
    state["validation_loss"] =  stats.func_value;
    state["validation_rmse"] =  sqrt((stats.func_value)/examples);
  }

  reg.reset();
  smooth_reg.reset();

}



/**
 * Predict for a single example.
 */
flexible_type linear_regression::predict_single_example(
         const DenseVector& x,
         const prediction_type_enum& output_type){
  return x.dot(coefs);
}

/**
 * Predict for a single example.
 */
flexible_type linear_regression::predict_single_example(
         const SparseVector& x,
         const prediction_type_enum& output_type){
  return x.dot(coefs);
}

/**
 * Setter for coefficients vector.
 */
void linear_regression::set_coefs(const DenseVector& _coefs) {
  coefs = _coefs;
}

/**
 * Turi Serialization Save
 */
void linear_regression::save_impl(turi::oarchive& oarc) const {

  // State
  variant_deep_save(state, oarc);

  // Everything else
  oarc << this->ml_mdata
       << this->metrics
       << this->coefs
       << this->options;

}

/**
 * Turi Serialization Load
 */
void linear_regression::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_MSG(version <= LINEAR_REGRESSION_MODEL_VERSION,
          "This model version cannot be loaded. Please re-save your state.");

  if (version < 4) {
    log_and_throw("Cannot load a model saved using a version prior to GLC-1.7.");
  }

  // State
  variant_deep_load(state, iarc);

  iarc >> ml_mdata
       >> metrics
       >> coefs
       >> options;

  if (version < 1){
    state["progress"] = to_variant(FLEX_UNDEFINED);
  }

  // Erase options that are no longer valid.
  if (version < 2){
    state.erase("auto_tuning");
    state.erase("mini_batch_size");
    options.delete_option("auto_tuning");
    options.delete_option("mini_batch_size");
  }

  // GLC 1.7
  if (version < 4) {
    tracking_metrics = metrics;
    this->set_default_evaluation_metric();

    // Add a column of Nones for stderrs.
    auto sf_coef = *(variant_get_value<std::shared_ptr<unity_sframe>>(
                state["coefficients"])->get_underlying_sframe());
    sf_coef = add_na_std_err_to_coef(sf_coef);
    std::shared_ptr<unity_sframe> unity_coef = std::make_shared<unity_sframe>();
    unity_coef->construct_from_sframe(sf_coef);
    state["coefficients"] = to_variant(unity_coef);
  }

}

/**
 * Gets the model version number
 */
size_t linear_regression::get_version() const{
  // Version translator
  // -----------------------
  //  0 -  Version 1.0
  //  1 -  Version 1.3
  //  2 -  Version 1.4
  //  3 -  Version 1.5
  //  4 -  Version 1.7
  return LINEAR_REGRESSION_MODEL_VERSION;
}

std::shared_ptr<coreml::MLModelWrapper> linear_regression::export_to_coreml() {

  std::map<std::string, flexible_type> context_metadata = {
    {"class", name()},
    {"version", std::to_string(get_version())},
    {"short_description", "Linear regression model."}};

  return export_linear_regression_as_model_asset(ml_mdata, coefs,
                                                 context_metadata);
}



} // supervised
} // turicreate
