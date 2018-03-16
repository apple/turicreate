/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// ML Data
#include <sframe/sframe.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
;
#include <sframe/algorithm.hpp>
#include <ml_data/ml_data.hpp>

// Toolkits
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>

// Solvers
#include <optimization/utils.hpp>
#include <optimization/constraints-inl.hpp>
#include <limits>

// Regularizer
#include <optimization/regularizers-inl.hpp>
#include <optimization/lbfgs-inl.hpp>
#include <optimization/newton_method-inl.hpp>
#include <optimization/accelerated_gradient-inl.hpp>

#include <toolkits/supervised_learning/linear_svm.hpp>
#include <toolkits/supervised_learning/linear_svm_opt_interface.hpp>

// Distributed
#ifdef HAS_DISTRIBUTED
#include <distributed/distributed_context.hpp>
#include <rpc/dc_global.hpp>
#include <rpc/dc.hpp>
#endif

// Utilities
#include <numerics/armadillo.hpp>
#include <util/logit_math.hpp>
#include <cmath>
#include <serialization/serialization_includes.hpp>


namespace turi {
namespace supervised {


/*
* SVM supervised_learning Solver Interface
*
*******************************************************************************
*/


/**
* Constructor for linear svm solver object
*/
linear_svm_scaled_logistic_opt_interface::linear_svm_scaled_logistic_opt_interface(
    const ml_data& _data, 
    const ml_data& _valid_data, 
    linear_svm& _model) {


  // Initialize ml_data, readers, and other metadata.
  // -----------------------------------------------------------------------
  data = _data;
  if (_valid_data.num_rows() > 0) valid_data = _valid_data;
  smodel = _model;

  // Initialize reader and other data
  examples = data.num_rows();
#ifdef HAS_DISTRIBUTED 
  auto dc = distributed_control_global::get_instance();
  dc->all_reduce(examples);
#endif
  features = data.num_columns();
  n_threads = turi::thread_pool::get_instance().size();

  // Initialize the number of variables to 1 (bias term)
  primal_variables = get_number_of_coefficients(smodel.get_ml_metadata());
  is_dense = (primal_variables <= 3 * features) ? true : false;

}

/**
* Set gamma
*/
void linear_svm_scaled_logistic_opt_interface::set_gamma(const double _gamma){
  gamma = _gamma;
}

/**
* Destructor for the linear svm solver object
*/
linear_svm_scaled_logistic_opt_interface::~linear_svm_scaled_logistic_opt_interface() {
}

/**
* Set the class weights (as a flex_dict which is already validated)
*/
void linear_svm_scaled_logistic_opt_interface::set_class_weights(
                                        const flexible_type& _class_weights) {
  DASSERT_TRUE(_class_weights.size() == classes);
  for(const auto& kvp: _class_weights.get<flex_dict>()){
    class_weights[kvp.first.get<flex_int>()]= kvp.second.get<flex_float>();
  }
}

/**
* Set the number of threads
*/
void linear_svm_scaled_logistic_opt_interface::set_threads(size_t _n_threads) {
  n_threads = _n_threads;
}


/**
* Get the number of examples for the model
*/
size_t linear_svm_scaled_logistic_opt_interface::num_examples() const{
  return examples;
}

/**
* Get the number of validation-set examples for the model
*/
size_t linear_svm_scaled_logistic_opt_interface::num_validation_examples() const{
  return valid_data.num_rows();
}

/**
* Get the number of variables for the model
*/
size_t linear_svm_scaled_logistic_opt_interface::num_variables() const{
  return primal_variables;
}

/**
* Get the number of classes for the model
*/
size_t linear_svm_scaled_logistic_opt_interface::num_classes() const{
  return classes;
}

/**
 * Get strings needed to print the header for the progress table.
 */
std::vector<std::pair<std::string, size_t>> 
linear_svm_scaled_logistic_opt_interface::get_status_header(const std::vector<std::string>& stat_headers) {
  bool has_validation_data = (valid_data.num_rows() > 0);
  auto header = make_progress_header(smodel, stat_headers, has_validation_data); 
  return header;
}

/**
 * Get strings needed to print a row of the progress table.
 */
std::vector<std::string> linear_svm_scaled_logistic_opt_interface::get_status(
    const DenseVector& coefs, 
    const std::vector<std::string>& stats) {

  // Copy coefficients, rescale, and update the model.
  DenseVector coefs_tmp = coefs;
  rescale_solution(coefs_tmp);
  smodel.set_coefs(coefs_tmp); 

  auto ret = make_progress_row_string(smodel, data, valid_data, stats);
  return ret; 
}

/**
 * Set feature rescaling.
 */
void linear_svm_scaled_logistic_opt_interface::init_feature_rescaling() {
  feature_rescaling = true;
  scaler.reset(new l2_rescaling(data.metadata(), true));
}

/**
 * Transform final solution back to the original scale.
 */
void linear_svm_scaled_logistic_opt_interface::rescale_solution(DenseVector& coefs) {
  if(feature_rescaling){
    scaler->transform(coefs);
  }
}

double linear_svm_scaled_logistic_opt_interface::get_validation_accuracy() {
  DASSERT_TRUE(valid_data.num_rows() > 0);

  auto eval_results = smodel.evaluate(valid_data, "train");
  auto results = eval_results.find("accuracy");
  if(results == eval_results.end()) {
    log_and_throw("No Validation Accuracy.");
  }

  variant_type variant_accuracy = results->second;
  double accuracy = variant_get_value<flexible_type>(variant_accuracy).to<double>();
  return accuracy;
}

double linear_svm_scaled_logistic_opt_interface::get_training_accuracy() {
  auto eval_results = smodel.evaluate(data, "train");
  auto results = eval_results.find("accuracy");

  if(results == eval_results.end()) {
    log_and_throw("No Validation Accuracy.");
  }
  variant_type variant_accuracy = results->second;
  double accuracy = variant_get_value<flexible_type>(variant_accuracy).to<double>();

  return accuracy;
}


/**
 * Compute the first order statistics
*/
void linear_svm_scaled_logistic_opt_interface::compute_first_order_statistics(const
    DenseVector& point, DenseVector& gradient, double& function_value, const
    size_t mbStart, const size_t mbSize) {

  // Mini-batch gradient is meaningless in the dual space.
  DASSERT_EQ(mbStart, 0);
  DASSERT_EQ(mbSize, (size_t)(-1));
  DASSERT_EQ(point.size(), primal_variables);

  // Init
  std::vector<double> f(n_threads, 0.0);
  std::vector<DenseVector> G(n_threads, arma::zeros(primal_variables));

  // Dense data. 
  if (this->is_dense) {
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      DenseVector x(primal_variables);
      double y, row_prob, margin, row_func;
      size_t class_idx = 0;
      for(auto it = data.get_iterator(thread_idx, num_threads);!it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x(primal_variables - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        // Map 
        class_idx = it->target_index();
        y = class_idx * 2 - 1.0;
        margin = -gamma * (y * dot(x, point) - 1);

        row_prob = - sigmoid(margin);
        row_func = log1pe(margin);

        f[thread_idx] += class_weights[class_idx] * row_func / gamma;
        G[thread_idx] += class_weights[class_idx] * y * x * row_prob;
      }
    });

  // Sparse data
  } else {
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      SparseVector x(primal_variables);
      double y, row_prob, margin, row_func;
      size_t class_idx = 0;
      for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x(primal_variables - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        // Map 
        class_idx = it->target_index();
        y = class_idx * 2 - 1.0;
        margin = -gamma * (y * dot(x, point) - 1);

        row_prob = -sigmoid(margin);
        row_func = log1pe(margin);

        f[thread_idx] += class_weights[class_idx] * row_func / gamma;
        for(auto p : x) {
          G[thread_idx][p.first] += (class_weights[class_idx] * y * row_prob) * p.second;
        }
      }
    });
  }

  // Reduce
  function_value = f[0];
  gradient = G[0];
  for(size_t i=1; i < n_threads; i++){
    function_value += f[i];
    gradient += G[i];
  }

#ifdef HAS_DISTRIBUTED
  auto dc = distributed_control_global::get_instance();
  DASSERT_TRUE(dc != NULL);
  dc->all_reduce(gradient, true);
  dc->all_reduce(function_value, true);
#endif

}


} // supervised
} // turicreate
