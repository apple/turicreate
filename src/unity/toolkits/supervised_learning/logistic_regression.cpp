/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// include these MLModel headers first so they don't get polluted by turi-create macros.
// otherwise at least CHECK() collides with incompatible implementation.

// ML Data
#include <sframe/sframe.hpp>
#include <sframe/algorithm.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <ml_data/ml_data.hpp>

// Toolkits
#include <toolkits/supervised_learning/logistic_regression_opt_interface.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>

// Core ML
#include <unity/toolkits/coreml_export/MLModel/src/transforms/LinearModel.hpp>
#include <unity/toolkits/coreml_export/MLModel/src/transforms/LogisticModel.hpp>
#include <unity/toolkits/coreml_export/mldata_exporter.hpp>
#include <unity/toolkits/coreml_export/coreml_export_utils.hpp>

// Solvers
#include <optimization/utils.hpp>
#include <optimization/newton_method-inl.hpp>
#include <optimization/lbfgs-inl.hpp>
#include <optimization/gradient_descent-inl.hpp>
#include <optimization/accelerated_gradient-inl.hpp>

// Regularizer
#include <optimization/regularizers-inl.hpp>

// Utilities
#include <numerics/armadillo.hpp>
#include <cmath>
#include <serialization/serialization_includes.hpp>

// Distributed
#ifdef HAS_DISTRIBUTED
#include <distributed/distributed_context.hpp>
#include <rpc/dc_global.hpp>
#include <rpc/dc.hpp>
#endif

#define LOGISTIC_REGRESSION_NEWTON_VARIABLES_HARD_LIMIT 10000
#define LOGISTIC_REGRESSION_NEWTON_VARIABLES_SOFT_LIMIT 500

namespace turi {
namespace supervised {

/*
 * Logistic Regression Model
 * ****************************************************************************
 */


/**
 * Destructor. Make sure bad things don't happen
 */
logistic_regression::~logistic_regression(){
  lr_interface.reset();
}


/**
 * Init function common to all regression inits.
 */
void logistic_regression::model_specific_init(const ml_data& data,
                                              const ml_data& valid_data){


  // Create an interface to the solver.
  this->num_classes = ml_mdata->target_index_size();
  size_t variables = get_number_of_coefficients(ml_mdata);
  this->num_coefficients = variables * (num_classes - 1);
  state["num_coefficients"] =  this->num_coefficients;

  // Examples per class
  state["num_classes"] = this->num_classes;
  state["num_examples_per_class"] =
                    to_variant(get_num_examples_per_class(ml_mdata));

  // Initialize the solver and set initial solution.
  lr_interface.reset(new logistic_regression_opt_interface(data, valid_data, *this));
  coefs = arma::zeros(variables);

}

/**
 * Setter for coefficients vector.
 */
void logistic_regression::set_coefs(const DenseVector& _coefs) {
  coefs = _coefs;
}

/**
 * Set options
 */
void logistic_regression::init_options(const std::map<std::string,
                                      flexible_type>&_opts) {

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
      {flexible_type("auto"), flexible_type("newton"),
       flexible_type("lbfgs"),
       flexible_type("gd"), flexible_type("fista")},
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
      "Penalty on the L1-penalty",
      0.01,
      0,
      optimization::OPTIMIZATION_INFTY,
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

  // Set options!
  options.set_options(_opts);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));

}


/**
 * Train a logistic regression model.
 */
void logistic_regression::train() {

  size_t variables_per_class = this->num_coefficients/ (this->num_classes - 1);
  if(get_option_value("feature_rescaling")){
    lr_interface->init_feature_rescaling();
  }

  // Set class weights
  flexible_type class_weights =
                      get_class_weights_from_options(options, ml_mdata);
  state["class_weights"] =  to_variant(class_weights);
  flex_dict _class_weights(this->num_classes);
  size_t i = 0;
  for(const auto& kvp: class_weights.get<flex_dict>()){
    _class_weights[i++] =
        {ml_mdata->target_indexer()->immutable_map_value_to_index(kvp.first),
         kvp.second.get<flex_float>()};
  }
  lr_interface->set_class_weights(_class_weights);

  // Set the initial point and write initial output to screen
  // ---------------------------------------------------------------------------
  DenseVector init_point(this->num_coefficients);
  init_point.zeros();
  display_classifier_training_summary("Logistic regression");
  logprogress_stream << "Number of coefficients      : " << this->num_coefficients
                     << std::endl;


  // Deal with regularizers
  // ---------------------------------------------------------------------------
  // Set regularize flag to all features except the intercept
  DenseVector is_regularized(this->num_coefficients);
  is_regularized.ones();
  for(size_t i = 1; i < this->num_classes; i++){
    is_regularized(i * variables_per_class - 1) = 0;
  }

  // Set the regularization penalty
  float l1_penalty = get_option_value("l1_penalty");
  float l2_penalty = get_option_value("l2_penalty");
  DenseVector alpha = is_regularized * l1_penalty;
  DenseVector beta = is_regularized * l2_penalty;


  // Construct the regularizer interface
  std::shared_ptr<optimization::regularizer_interface> reg;
  std::shared_ptr<optimization::smooth_regularizer_interface> smooth_reg;

  if (l1_penalty < optimization::OPTIMIZATION_ZERO) {
    smooth_reg.reset(new optimization::l2_norm(beta));
    reg = smooth_reg;
  } else {
    reg.reset(new optimization::elastic_net(alpha, beta));
  }


  // Sort out the appropriate solver for the regularization settings
  std::string solver =  get_option_value("solver");

  // Auto solver
  // \note Currently, we do not incorporate dataset sparsity while selecting
  // the datasets. We should store a "sparsity index" in metadata to give
  // us a sense of how sparse the dataset is. Ideally, all the rules are
  // going to be heavily dependant on sparsity. Right now, we will assume
  // that all "fat" datasets are always sparse.
  if(solver == "auto"){
    if(this->num_coefficients > LOGISTIC_REGRESSION_NEWTON_VARIABLES_SOFT_LIMIT){
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
  if(solver == "newton" && this->num_coefficients > LOGISTIC_REGRESSION_NEWTON_VARIABLES_HARD_LIMIT){
    ss << "Number of coefficients is too large for Newton method. "
       << "Try using the option solver='lbfgs'."
       << std::endl;
    log_and_throw(ss.str());
  }


  // Call the solvers
  // ---------------------------------------------------------------------------
  optimization::solver_return stats;
  std::map<std::string, flexible_type> solver_options
    = options.current_option_values();

  if (solver == "newton") {
    stats = turi::optimization::newton_method(*lr_interface, init_point,
        solver_options, smooth_reg);
  } else if (solver == "lbfgs") {
    stats = turi::optimization::lbfgs(*lr_interface, init_point,
        solver_options, smooth_reg);
  } else if (solver == "fista") {
    stats = turi::optimization::accelerated_gradient(*lr_interface,
        init_point, solver_options, reg);
  } else {
    std::ostringstream msg;
    msg << "Solver " << solver << " is not supported." << std::endl;
    msg << "Supported solvers are (auto, newton, lbfgs, fista)" << std::endl;
    log_and_throw(msg.str());
  }

  // Save final accuracies
  if(lr_interface->num_validation_examples() > 0) {
    state["validation_accuracy"] = lr_interface->get_validation_accuracy();
  }
  state["training_accuracy"] = lr_interface->get_training_accuracy();

  // Store the coefficients in the model
  // ---------------------------------------------------------------------------
  coefs = stats.solution;
  lr_interface->rescale_solution(coefs);
  bool has_stderr = (stats.hessian.n_rows * stats.hessian.n_cols > 0) &&
    (this->num_examples() > this->num_coefficients);
  if (has_stderr) {
    std_err = get_stderr_from_hessian(stats.hessian);
    DASSERT_EQ(std_err.size(), coefs.size()); 
    lr_interface->rescale_solution(std_err);
  }

  // Save coefs to SFrame.
  sframe sf_coef = get_coefficients_as_sframe(coefs, ml_mdata, std_err);
  if(!has_stderr) {
    sf_coef = add_na_std_err_to_coef(sf_coef);
  }
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

  // Compute validation-set stats.
  if (lr_interface->num_validation_examples() > 0) {
    // Recycle lvalues from stats to use as out parameters here, now that we're
    // otherwise done reading from stats.
    lr_interface->compute_validation_second_order_statistics(
        stats.solution, stats.hessian, stats.gradient, stats.func_value);
    state["validation_loss"] =  stats.func_value;
  }

  reg.reset();
  smooth_reg.reset();
}

/**
 * Predict for a single example.
 */
flexible_type logistic_regression::predict_single_example(
          const DenseVector& x,
          const prediction_type_enum& output_type) {

  // Binary classification
  if (this->num_classes == 2) {
    double margin = dot(x, coefs);
    double kernel = std::exp(-margin);
    double row_prob = std::exp(-std::log1p(kernel));

    // Probability or Rank. Note that we sort by rank for probability.
    switch(output_type) {

      case prediction_type_enum::MARGIN:
        return margin;

      case prediction_type_enum::PROBABILITY:
      case prediction_type_enum::RANK:
        return row_prob;

      // Probability vector prediction.
      case prediction_type_enum::PROBABILITY_VECTOR:
        return flex_vec{1.0 - row_prob, row_prob};

      // Class prediction
      case prediction_type_enum::CLASS_INDEX:
        return row_prob >= 0.5;

      case prediction_type_enum::CLASS:
        return  ml_mdata->target_indexer()->map_index_to_value(row_prob >= 0.5);

      case prediction_type_enum::MAX_PROBABILITY:
      case prediction_type_enum::NA:
        log_and_throw("Output type not supported");

    }

  // Multi-class classification
  } else {

    size_t variables_per_class = this->num_coefficients / (this->num_classes - 1);
    DenseMatrix coefsMat(coefs);
    coefsMat.reshape(variables_per_class, this->num_classes-1);
    DenseVector margin = coefsMat.t() * x;
    DenseVector kernel = arma::exp(margin);
    DenseVector prob = kernel / (1 + arma::sum(kernel));

    switch(output_type) {

      // Probability (doesn't get used from the python side)
      // Probability vector gets used (doesn't get used from the python side)
      case prediction_type_enum::PROBABILITY:
      case prediction_type_enum::PROBABILITY_VECTOR:
      {

        std::vector<double> prob_as_vector(prob.begin(),
                                           prob.begin() + this->num_classes -1);
        prob_as_vector.insert(prob_as_vector.begin(), 1 - arma::sum(prob));
        for(auto& d : prob_as_vector) {
          if (d < 0.0) d = 0.0;
          else if (d > 1.0) d = 1.0;
        }
        return prob_as_vector;
      }

      // Margin or Rank
      case prediction_type_enum::RANK:
      case prediction_type_enum::MARGIN:
      {

        std::vector<double> margin_as_vector(margin.begin(),
                                             margin.begin() + this->num_classes -1);
        margin_as_vector.insert(margin_as_vector.begin(), 0);
        return margin_as_vector;
      }

      // Best class.
      case prediction_type_enum::CLASS_INDEX:
      case prediction_type_enum::CLASS:
      {

        double max_margin = 0;
        size_t class_idx = 0;
        for(size_t i = 0; i < this->num_classes - 1; i++){
        if (max_margin < margin(i)){
            max_margin = margin(i);
            class_idx = i + 1;
          }
        }

        // Class index or class.
        if (output_type == prediction_type_enum::CLASS_INDEX) {
          return class_idx;
        } else {
          return ml_mdata->target_indexer()->map_index_to_value(class_idx);
        }

      }
      // Probability for best class
      case prediction_type_enum::MAX_PROBABILITY:
        return std::min<double>(std::max<double>(
                std::max<double>(1 - arma::sum(prob), prob.max()),
               0.0), 1.0);

      case prediction_type_enum::NA:
        log_and_throw("Output type not supported");
    }
  }
  DASSERT_TRUE(false);
}

/**
 * Predict for a single example.
 */
flexible_type logistic_regression::predict_single_example(
          const SparseVector& x,
          const prediction_type_enum& output_type) {

  // Binary classification
  if (this->num_classes == 2) {
    double margin = dot(x, coefs);
    double kernel = std::exp(-margin);
    double row_prob = std::exp(-std::log1p(kernel));

    switch (output_type) {

      // Margin.
      case prediction_type_enum::MARGIN:
        return margin;

      // Probability or Rank. Note that we sort by rank for probability.
      case prediction_type_enum::PROBABILITY:
      case prediction_type_enum::RANK:
        return row_prob;

      // Probability vector prediction.
      case prediction_type_enum::PROBABILITY_VECTOR:
        return flex_vec{1.0 - row_prob, row_prob};

      // Class prediction [index].
      case prediction_type_enum::CLASS_INDEX:
        return row_prob >= 0.5;

      // Class
      case prediction_type_enum::CLASS:
        return  ml_mdata->target_indexer()->map_index_to_value(row_prob >= 0.5);

      case prediction_type_enum::MAX_PROBABILITY:
      case prediction_type_enum::NA:
        log_and_throw("Output type not supported");
    }

  // Multi-class classification
  } else {
    size_t variables_per_class = this->num_coefficients / (this->num_classes - 1);

    DenseMatrix coefsMat(coefs);
    coefsMat.reshape(variables_per_class, this->num_classes-1);
    DenseVector margin = coefsMat.t() * x;
    DenseVector kernel = arma::exp(margin);
    DenseVector prob = kernel / (1 + arma::sum(kernel));

    switch (output_type) {

     // Probability
      case prediction_type_enum::PROBABILITY:
      case prediction_type_enum::PROBABILITY_VECTOR:
      {
        std::vector<double> prob_as_vector(this->num_classes);
        prob_as_vector[0] = 1 - arma::sum(prob);
        std::copy(prob.begin(), prob.begin() + this->num_classes-1,
                  prob_as_vector.begin() + 1);

        return prob_as_vector;
      }

      // Margin or Rank
      case prediction_type_enum::MARGIN:
      case prediction_type_enum::RANK:
      {
        std::vector<double> margin_as_vector(this->num_classes);
        margin_as_vector[0] = 0;
        std::copy(margin.begin(), margin.begin() + this->num_classes -1,
                  margin_as_vector.begin() + 1);
        return margin_as_vector;
      }

      // Best class.
      case prediction_type_enum::CLASS_INDEX:
      case prediction_type_enum::CLASS:
      {
        double max_margin = 0;
        size_t class_idx = 0;
        for(size_t i = 0; i < this->num_classes - 1; i++){
        if (max_margin < margin(i)){
            max_margin = margin(i);
            class_idx = i + 1;
          }
        }
        // Class index or class.
        if (output_type == prediction_type_enum::CLASS_INDEX) {
          return class_idx;
        } else {
          return ml_mdata->target_indexer()->map_index_to_value(class_idx);
        }
      }
      // Probability for best class
      case prediction_type_enum::MAX_PROBABILITY:
        return std::max<double>(1 - arma::sum(prob), prob.max());

      case prediction_type_enum::NA:
        log_and_throw("Output type not supported");

    }
  }
  log_and_throw("Internal error during prediction.");
  return flex_undefined();
}

/**
 * Make predictions using a trained model using the predict_single_example
 * interface.
 */
gl_sframe logistic_regression::fast_predict_topk(
          const std::vector<flexible_type>& rows,
          const std::string& missing_value_action,
          const std::string& output_type,
          const size_t topk){

  DASSERT_TRUE(name().find("classifier") != std::string::npos);
  DASSERT_TRUE(state.count("num_coefficients") > 0);

  // Get a copy of the variables in the state.
  size_t num_classes = variant_get_value<size_t>(state.at("num_classes"));
  DASSERT_TRUE(num_classes > 1);
  size_t variables = 0;
  auto pred_type_enum = prediction_type_enum_from_name(output_type);
  auto na_enum = get_missing_value_enum_from_string(missing_value_action);

  // Set the variables based on classes and coefficieents.
  variables = variant_get_value<size_t>(state.at("num_coefficients"));
  variables = variables / (num_classes - 1);

  // Error checking.
  if (topk > num_classes) {
    std::stringstream ss;
    ss << "The training data contained " << num_classes << " classes."
       << " The parameter 'k' must be less than or equal to the number of "
       << "classes in the training data." << std::endl;
    log_and_throw(ss.str());
  }

  // Setup the SFrame writer for output
  std::vector<std::string> col_names {"id", "class", output_type};
  std::vector<flex_type_enum> col_types {flex_type_enum::INTEGER,
                                    (this->ml_mdata)->target_column_type()};
  if (output_type == "rank"){
    col_types.push_back(flex_type_enum::INTEGER);
  } else {
    col_types.push_back(flex_type_enum::FLOAT);
  }
  gl_sframe_writer writer(col_names, col_types, 1); // 1 segment.

  // For each example: Do the following.
  size_t row_number = 0;
  for (const auto& row: rows) {
    // Local variables.
    std::vector<std::pair<size_t, double>> out;
    out.resize(num_classes);
    flexible_type preds;
    std::vector<flexible_type> write_x;
    write_x.resize(3);

    // Check type.
    if (row.get_type() != flex_type_enum::DICT) {
      log_and_throw(
          "TypeError: Expecting dictionary as input type for each example.");
    }

    // Dense predict.
    if (this->is_dense()) {
      DenseVector dense_vec(variables);
      fill_reference_encoding(ml_data_row_reference::from_row(
               this->ml_mdata, row.get<flex_dict>(), na_enum), dense_vec);
      dense_vec(variables - 1) = 1;
      preds = predict_single_example(dense_vec, pred_type_enum);

    // Sparse predict.
    } else {
      SparseVector sparse_vec(variables);
      fill_reference_encoding(ml_data_row_reference::from_row(
               this->ml_mdata, row.get<flex_dict>(), na_enum), sparse_vec);
      sparse_vec(variables - 1) = 1;
      preds = predict_single_example(sparse_vec, pred_type_enum);
    }

    // Multiclass
    if (preds.size() == num_classes) {
      for (size_t k = 0; k < num_classes; ++k) {
        out[k] = std::make_pair(k, preds[k]);
      }
    // Binary
    } else {
      double zero_pred = (pred_type_enum == prediction_type_enum::MARGIN) ? 0.0 : 1.0 - (double)preds;
      out[0] = std::make_pair(0, zero_pred);
      out[1] = std::make_pair(1, (double) preds);
    }

    // Sort
    std::nth_element(out.begin(), out.begin() + topk - 1, out.end(),
                  boost::bind(&std::pair<size_t, double>::second, _1) >
                  boost::bind(&std::pair<size_t, double>::second, _2));

    // Write the topk
    for (size_t k = 0; k < topk; ++k) {
      write_x[0] = row_number;
      write_x[1] = this->ml_mdata->target_indexer()
                        ->map_index_to_value(out[k].first);
      if (pred_type_enum == prediction_type_enum::RANK){
        write_x[2] = k;
      } else {
        write_x[2] = out[k].second;
      }
      writer.write(write_x, 0);
    }
    row_number++;
  }
  return writer.close();
}


/**
 * Turi Serialization Save
 */
void logistic_regression::save_impl(turi::oarchive& oarc) const {

  // State
  variant_deep_save(state, oarc);
  
  // Everything else
  oarc << ml_mdata
       << metrics
       << coefs
       << options;

}

/**
 * Turi Serialization Load
 */
void logistic_regression::load_version(turi::iarchive& iarc, size_t version) {
  ASSERT_MSG(version <= LOGISTIC_REGRESSION_MODEL_VERSION,
     "This model version cannot be loaded. Please re-save your state.");
  if (version < 6) {
    log_and_throw("Cannot load a model saved using a version prior to GLC-1.7.");
  }

  // State
  variant_deep_load(state, iarc);
  this->num_classes = variant_get_value<size_t>(state["num_classes"]);
  this->num_coefficients = variant_get_value<size_t>(state["num_coefficients"]);

  // GLC 1.3-
  iarc >> ml_mdata;
  iarc >> metrics;
  iarc >> coefs;
  iarc >> options;

  state["progress"] = to_variant(FLEX_UNDEFINED);

  // GLC 1.7
  if (version < 6) {
    tracking_metrics = metrics;
    this->set_default_evaluation_metric();
    
    // Add a column of Nones for stderrs.
    auto sf_coef = *(variant_get_value<std::shared_ptr<unity_sframe>>(
                state["coefficients"])->get_underlying_sframe());
    sf_coef = add_na_std_err_to_coef(sf_coef);
    std::shared_ptr<unity_sframe> unity_coef = std::make_shared<unity_sframe>();
    unity_coef->construct_from_sframe(sf_coef);
    state["coefficients"] = unity_coef; 
  }

}

/**
 * Gets the model version number
 */
size_t logistic_regression::get_version() const{
  // Version translator
  // -----------------------
  //  0 -  Version 1.0
  //  1 -  Version 1.1
  //  2 -  Version 1.2
  //  3 -  Version 1.3
  //  4 -  Version 1.4
  //  5 -  Version 1.5
  //  6 -  Version 1.7
  return LOGISTIC_REGRESSION_MODEL_VERSION;
}

void logistic_regression::export_to_coreml(const std::string& filename) {

  std::string prob_column_name = ml_mdata->target_column_name() + " Probability";
  CoreML::Pipeline  pipeline = CoreML::Pipeline::Classifier(ml_mdata->target_column_name(), prob_column_name, "");

  setup_pipeline_from_mldata(pipeline, ml_mdata);

  //////////////////////////////////////////////////////////////////////
  // Now set up the actual model.
  CoreML::LogisticModel model = CoreML::LogisticModel(ml_mdata->target_column_name(),
                                                                prob_column_name,
                                                                "Logistic Regression");

  std::vector<double> one_hot_coefs;
  supervised::get_one_hot_encoded_coefs(coefs, ml_mdata, one_hot_coefs);

  size_t num_classes = ml_mdata->target_index_size();
  double offset = one_hot_coefs.back();
  model.setOffsets({offset});
  one_hot_coefs.pop_back();
  model.setWeights({one_hot_coefs});

  auto target_output_data_type = CoreML::FeatureType::Double();
  auto target_additional_data_type = CoreML::FeatureType::Double();
  if(ml_mdata->target_column_type() == flex_type_enum::INTEGER) {
      std::vector<int64_t> classes(num_classes);
      for(size_t i = 0; i < num_classes; ++i) {
        classes[i] = ml_mdata->target_indexer()->map_index_to_value(i).get<flex_int>();
      }
      model.setClassNames(classes);
    target_output_data_type = CoreML::FeatureType::Int64();
    target_additional_data_type = \
              CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType);
  } else if(ml_mdata->target_column_type() == flex_type_enum::STRING) {
      std::vector<std::string> classes(num_classes);
      for(size_t i = 0; i < num_classes; i++) {
        classes[i] = ml_mdata->target_indexer()->map_index_to_value(i).get<std::string>();
      }
      model.setClassNames(classes);
      target_output_data_type = CoreML::FeatureType::String();
      target_additional_data_type = \
             CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_stringKeyType);

  } else {
    log_and_throw("Only exporting classifiers with an output class "
                  "of integer or string is supported.");
  }

  // Model inputs and output
  model.addInput("__vectorized_features__",
              CoreML::FeatureType::Array({ml_mdata->num_dimensions()}));
  model.addOutput(ml_mdata->target_column_name(), target_output_data_type);
  model.addOutput(prob_column_name, target_additional_data_type);

  // Pipeline outputs
  pipeline.add(model);
  pipeline.addOutput(ml_mdata->target_column_name(), target_output_data_type);
  pipeline.addOutput(prob_column_name, target_additional_data_type);
  
  // Add metadata
  std::map<std::string, flexible_type> context_metadata = {
    {"class", name()},
    {"version", std::to_string(get_version())},
    {"short_description", "Logisitic regression model."}};


  // Add metadata
  add_metadata(pipeline.m_spec, context_metadata);

  // Save pipeline
  CoreML::Result r = pipeline.save(filename);
  if(!r.good()) {
    log_and_throw("Could not export model: " + r.message());
  }
}

} // supervised
} // turicreate
