/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// ML Data
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>

// Toolkits
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <toolkits/supervised_learning/logistic_regression_opt_interface.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>

// Solvers
#include <ml/optimization/utils.hpp>
#include <ml/optimization/newton_method-inl.hpp>
#include <ml/optimization/lbfgs.hpp>
#include <ml/optimization/gradient_descent-inl.hpp>
#include <ml/optimization/accelerated_gradient-inl.hpp>

// Regularizer
#include <ml/optimization/regularizers-inl.hpp>

// Utilities
#include <Eigen/SparseCore>
#include <cmath>
#include <core/storage/serialization/serialization_includes.hpp>


namespace turi {
namespace supervised {


/*
* Logistic Regression Solver Interface
*
*******************************************************************************
*/


/**
* Perform a specialized operation of a outer product between a sparse
* vector and a dense vector and flatten the result.
*
* out = a * b.transpose()
* out.resize(9,1);
*
*/
void flattened_sparse_vector_outer_prod(const SparseVector& a,
                                        const DenseVector& b,
                                        SparseVector& out) {
  DASSERT_TRUE(out.size() == a.size() * b.size());
  out.reserve(a.nonZeros() * b.size());
  size_t a_size = a.size();
  for(size_t j = 0; j < size_t(b.size()); j++){
    for (SparseVector::InnerIterator i(a); i; ++i){
      out.coeffRef(i.index() + a_size * j)  = b(j) * i.value();
    }
  }
}

/**
* Constructor for logistic regression solver object
*/
logistic_regression_opt_interface::logistic_regression_opt_interface(
    const ml_data& _data,
    const ml_data& _valid_data,
    logistic_regression& _sp_model)
: smodel(_sp_model) {

  data = _data;
  if (_valid_data.num_rows() > 0) valid_data = _valid_data;

  // Initialize reader and other data
  examples = data.num_rows();
  features = data.num_columns();
  n_threads = turi::thread_pool::get_instance().size();

  // Initialize the number of variables to 1 (bias term)
  auto ml_metadata = smodel.get_ml_metadata();
  classes = ml_metadata->target_index_size();
  variables = get_number_of_coefficients(ml_metadata);
  is_dense = (variables <= 3 * data.max_row_size()) ? true : false;
  variables *= (classes - 1);


}


/**
* Destructor for the logistic regression solver object
*/
logistic_regression_opt_interface::~logistic_regression_opt_interface() {
}


/**
* Set the number of threads
*/
void logistic_regression_opt_interface::set_threads(size_t _n_threads) {
  n_threads = _n_threads;
}


/**
* Set the class weights (as a flex_dict which is already validated)
*/
void logistic_regression_opt_interface::set_class_weights(
                                        const flexible_type& _class_weights) {
  DASSERT_TRUE(_class_weights.size() == classes);
  for(const auto& kvp: _class_weights.get<flex_dict>()){
    class_weights[kvp.first.get<flex_int>()]= kvp.second.get<flex_float>();
  }
}


/**
 * Set feature rescaling.
 */
void logistic_regression_opt_interface::init_feature_rescaling() {
  feature_rescaling = true;
  scaler.reset(new l2_rescaling(smodel.get_ml_metadata(), true));
}

/**
 * Transform final solution back to the original scale.
 */
void logistic_regression_opt_interface::rescale_solution(DenseVector& coefs) {
  if(feature_rescaling){
    size_t variables_per_class = variables / (classes-1);
    DenseVector coefs_per_class(variables_per_class);
    for(size_t i = 0; i < classes - 1; i++){
      coefs_per_class = coefs.segment(i * variables_per_class,
                                      variables_per_class);
      scaler->transform(coefs_per_class);
      coefs.segment(i * variables_per_class, variables_per_class) =
                                                        coefs_per_class;
    }
  }
}

/**
* Get the number of examples for the model
*/
size_t logistic_regression_opt_interface::num_examples() const{
  return examples;
}


/**
* Get the number of validation-set examples for the model
*/
size_t logistic_regression_opt_interface::num_validation_examples() const{
  return valid_data.num_rows();
}


/**
* Get the number of variables for the model
*/
size_t logistic_regression_opt_interface::num_variables() const{
  return variables;
}


/**
* Get the number of classes for the model
*/
size_t logistic_regression_opt_interface::num_classes() const{
  return classes;
}


/**
 * Get strings needed to print the header for the progress table.
 */
std::vector<std::pair<std::string, size_t>>
logistic_regression_opt_interface::get_status_header(const std::vector<std::string>& stat_headers) {
  bool has_validation_data = (valid_data.num_rows() > 0);
  auto header = make_progress_header(smodel, stat_headers, has_validation_data);
  return header;
}

double logistic_regression_opt_interface::get_validation_accuracy() {
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

double logistic_regression_opt_interface::get_training_accuracy() {
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
 * Get strings needed to print a row of the progress table.
 */
std::vector<std::string> logistic_regression_opt_interface::get_status(
    const DenseVector& coefs,
    const std::vector<std::string>& stats) {

  DenseVector coefs_tmp = coefs;
  rescale_solution(coefs_tmp);
  smodel.set_coefs(coefs_tmp);

  auto ret = make_progress_row_string(smodel, data, valid_data, stats);
  return ret;
}

/**
 * Compute the first order statistics
*/
void logistic_regression_opt_interface::compute_first_order_statistics(
    const ml_data& data, const DenseVector& point, DenseVector& gradient,
    double& function_value, const size_t mbStart, const size_t mbSize) {
  DASSERT_TRUE(mbStart == 0);
  DASSERT_TRUE(mbSize == (size_t)(-1));

  // Init
  std::vector<DenseVector> G(n_threads, Eigen::MatrixXd::Zero(variables, 1));
  std::vector<double> f(n_threads, 0.0);
  size_t variables_per_class = variables / (classes-1);
  timer t;
  double start_time = t.current_time();

  logstream(LOG_INFO) << "Starting first order stats computation" << std::endl;

  // Dense data.
  if (this->is_dense) {
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      DenseVector x(variables_per_class);
      double row_func = 0, margin_dot_class = 0;
      DenseVector margin(classes - 1), kernel(classes - 1), row_prob(classes -1);
      DenseMatrix pointMat(point);
      pointMat.resize(variables_per_class, classes-1);
      size_t class_idx = 0;
      double kernel_sum = 0;
      for (auto it = data.get_iterator(thread_idx, num_threads); !it.done();
           ++it) {
        class_idx = it->target_index();

        if(class_idx >= classes) {
           continue;
        }

        fill_reference_encoding(*it, x);
        x.coeffRef(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMat.transpose() * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;

        kernel =  (margin).array().exp();
        kernel_sum = kernel.sum();
        row_func = log1p(kernel_sum) - margin_dot_class;
        row_prob = kernel / (1 + kernel_sum);
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;

        DenseMatrix G_tmp = class_weights[class_idx] * (x * row_prob.transpose());
        G[thread_idx] += Eigen::Map<Eigen::VectorXd>(G_tmp.data(), variables);
        f[thread_idx] += class_weights[class_idx] * row_func;
      }
    });

  // Sparse data
  } else {
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      SparseVector x(variables_per_class);
      double row_func = 0, margin_dot_class = 0;
      DenseVector margin(classes - 1), kernel(classes - 1), row_prob(classes -1);
      DenseMatrix pointMat(point);
      pointMat.resize(variables_per_class, classes-1);
      size_t class_idx = 0;
      double kernel_sum = 0;
      for(auto it = data.get_iterator(thread_idx, num_threads);
                                                              !it.done(); ++it) {
        class_idx = it->target_index();

        if(class_idx >= classes) {
           continue;
        }

        fill_reference_encoding(*it, x);
        x.coeffRef(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMat.transpose() * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;

        kernel =  (margin).array().exp();
        kernel_sum = kernel.sum();
        row_func = log1p(kernel_sum) - margin_dot_class;
        row_prob = kernel / (1 + kernel_sum);

        SparseVector G_tmp(variables);
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;
        flattened_sparse_vector_outer_prod(x, row_prob, G_tmp);
        G_tmp *= class_weights[class_idx];

        optimization::vector_add<DenseVector, SparseVector>(G[thread_idx], G_tmp);
        f[thread_idx] += class_weights[class_idx] * row_func;
      }
    });
  }

  // Reduce
  function_value = f[0];
  gradient = G[0];
  for(size_t i=1; i < n_threads; i++){
    gradient += G[i];
    function_value += f[i];
  }

  logstream(LOG_INFO) << "Computation done at "
                      << (t.current_time() - start_time) << "s" << std::endl;
}

/**
 * Compute the second order statistics
*/
void logistic_regression_opt_interface::compute_second_order_statistics(
    const DenseVector& point, DenseMatrix& hessian, DenseVector& gradient,
    double& function_value) {

  timer t;
  double start_time = t.current_time();
  logstream(LOG_INFO) << "Starting second order stats computation" << std::endl;

  // Init
  std::vector<DenseMatrix> H(n_threads, Eigen::MatrixXd::Zero(variables, variables));
  std::vector<DenseVector> G(n_threads, Eigen::MatrixXd::Zero(variables, 1));
  std::vector<double> f(n_threads, 0.0);
  size_t variables_per_class = variables / (classes-1);

  // Dense data
  if (this->is_dense) {

    in_parallel([&](size_t thread_idx, size_t num_threads) {
      DenseVector x(variables_per_class);
      double row_func = 0, margin_dot_class = 0, kernel_sum = 0;
      size_t class_idx = 0;
      DenseVector margin(classes - 1), kernel(classes - 1), row_prob(classes -1);
      DenseMatrix pointMat(point);
      pointMat.resize(variables_per_class, classes-1);
      DenseMatrix A(classes-1, classes-1);
      for(auto it = data.get_iterator(thread_idx, num_threads);
                                                              !it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x.coeffRef(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMat.transpose() * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;

        kernel =  (margin).array().exp();
        kernel_sum = kernel.sum();
        row_prob = kernel / (1 + kernel_sum);

        // Can't be done in one line thanks to Eigen.
        A = -row_prob * row_prob.transpose();
        A +=  row_prob.asDiagonal();

        row_func = log1p(kernel_sum) - margin_dot_class;
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;

        f[thread_idx] += class_weights[class_idx] * row_func;
        DenseMatrix G_tmp = class_weights[class_idx] * (x * row_prob.transpose());
        G[thread_idx] += Eigen::Map<Eigen::VectorXd>(G_tmp.data(), variables);
        DenseMatrix XXT = x * x.transpose();

        for(size_t a = 0; a < classes - 1; a++){
          for(size_t b = 0; b < classes - 1; b++){
            H[thread_idx].block(a * variables_per_class,
                 b * variables_per_class,
                 variables_per_class,
                 variables_per_class) += class_weights[class_idx] * A(a,b) * XXT;
          }
        }
      }
    });

  // Sparse data.
  } else {
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      SparseVector x(variables_per_class);
      double row_func = 0, margin_dot_class = 0, kernel_sum = 0;
      size_t class_idx = 0;
      DenseVector margin(classes - 1), kernel(classes - 1), row_prob(classes -1);
      DenseMatrix pointMat(point);
      pointMat.resize(variables_per_class, classes-1);
      DenseMatrix A(classes-1, classes-1);
      for(auto it = data.get_iterator(thread_idx, num_threads);
                                                              !it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x.coeffRef(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMat.transpose() * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;

        kernel =  (margin).array().exp();
        kernel_sum = kernel.sum();
        row_prob = kernel / (1 + kernel_sum);

        // Can't be done in one line thanks to Eigen.
        A = -row_prob * row_prob.transpose();
        A +=  row_prob.asDiagonal();

        row_func = log1p(kernel_sum) - margin_dot_class;
        SparseVector G_tmp(variables);
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;
        flattened_sparse_vector_outer_prod(x, row_prob, G_tmp);
        G_tmp = G_tmp * class_weights[class_idx];
        optimization::vector_add<DenseVector, SparseVector>(G[thread_idx], G_tmp);
        f[thread_idx] += class_weights[class_idx] * row_func;

        // Sadly, this is the fastest way to do this in Eigen. It can be done
        // in block mode if x is dense, but when x is sparse, this seems to be
        // faster (much faster).
        for(size_t a = 0; a < classes - 1; a++){
          for(size_t b = 0; b < classes - 1; b++){
            size_t a_index_offset = a * variables_per_class;
            size_t b_index_offset = b * variables_per_class;
            for (SparseVector::InnerIterator i(x); i; ++i){
              for (SparseVector::InnerIterator j(x); j; ++j){
                H[thread_idx](a_index_offset + i.index(),
                              b_index_offset + j.index()) +=
                  class_weights[class_idx] * i.value() * j.value() * A(a,b);
              }
            }
          }
        }
      }
    });
  }

  // Reduce
  function_value = f[0];
  hessian = H[0];
  gradient = G[0];
  for(size_t i=1; i < n_threads; i++){
    hessian += H[i];
    gradient += G[i];
    function_value += f[i];
  }

  logstream(LOG_INFO) << "Computation done at "
                      << (t.current_time() - start_time) << "s" << std::endl;
}

void logistic_regression_opt_interface::compute_first_order_statistics(const
    DenseVector& point, DenseVector& gradient, double& function_value, const
    size_t mbStart, const size_t mbSize) {
  compute_first_order_statistics(
      data, point, gradient, function_value, mbStart, mbSize);
}

void
logistic_regression_opt_interface::compute_validation_first_order_statistics(
    const DenseVector& point, DenseVector& gradient, double& function_value) {
  compute_first_order_statistics(
      valid_data, point, gradient, function_value);
}


} // supervised
} // turicreate
