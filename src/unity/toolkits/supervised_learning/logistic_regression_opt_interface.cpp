/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// ML Data
#include <sframe/sframe.hpp>
#include <sframe/algorithm.hpp>
#include <unity/lib/unity_sframe.hpp>

// Toolkits
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <toolkits/supervised_learning/logistic_regression_opt_interface.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>

// Solvers
#include <optimization/utils.hpp>
#include <optimization/newton_method-inl.hpp>
#include <optimization/lbfgs-inl.hpp>
#include <optimization/gradient_descent-inl.hpp>
#include <optimization/accelerated_gradient-inl.hpp>

// Regularizer
#include <optimization/regularizers-inl.hpp>

// Distributed
#ifdef HAS_DISTRIBUTED
#include <distributed/distributed_context.hpp>
#include <rpc/dc_global.hpp>
#include <rpc/dc.hpp>
#endif

// Utilities
#include <numerics/armadillo.hpp>
#include <cmath>
#include <serialization/serialization_includes.hpp>


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
* out = a * b.t()
* out.resize(9,1);
*
*/
void flattened_sparse_vector_outer_prod(const SparseVector& a, 
                                        const DenseVector& b,
                                        SparseVector& out) {
  DASSERT_TRUE(out.size() == a.size() * b.size());
  out.clear();
  out.reserve(a.num_nonzeros() * b.size());
  size_t a_size = a.size();
  for(size_t j = 0; j < b.size(); j++){
    for(auto p : a) {
      out.insert(p.first + a_size * j, b(j) * p.second);
    }
  }
}

/**
* Constructor for logistic regression solver object
*/
logistic_regression_opt_interface::logistic_regression_opt_interface(
    const ml_data& _data, 
    const ml_data& _valid_data, 
    logistic_regression& _sp_model) {  

  data = _data;
  if (_valid_data.num_rows() > 0) valid_data = _valid_data;
  smodel = _sp_model;

  // Initialize reader and other data
  examples = data.num_rows();
#ifdef HAS_DISTRIBUTED 
  auto dc = distributed_control_global::get_instance();
  dc->all_reduce(examples);
#endif
  features = data.num_columns();
  n_threads = turi::thread_pool::get_instance().size();

  // Initialize the number of variables to 1 (bias term)
  auto ml_metadata = smodel.get_ml_metadata();
  classes = ml_metadata->target_index_size();
  variables = get_number_of_coefficients(ml_metadata);
  is_dense = (variables <= 3 * features) ? true : false;
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
      size_t m = variables_per_class;
      coefs_per_class = coefs.subvec(i * m, (i + 1)*m - 1 /*end inclusive*/);
      scaler->transform(coefs_per_class);
      coefs.subvec(i * m, (i + 1) * m - 1) = coefs_per_class;
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
void logistic_regression_opt_interface::compute_first_order_statistics(const
    DenseVector& point, DenseVector& gradient, double& function_value, const
    size_t mbStart, const size_t mbSize) {
  DASSERT_TRUE(mbStart == 0);
  DASSERT_TRUE(mbSize == (size_t)(-1));

  // Init
  std::vector<DenseVector> G(n_threads, arma::zeros(variables));
  std::vector<double> f(n_threads, 0.0);
  size_t variables_per_class = variables / (classes-1);
  timer t;
  double start_time = t.current_time();

#ifdef HAS_DISTRIBUTED
  auto dc = distributed_control_global::get_instance();
  DASSERT_TRUE(dc != NULL);
  logstream(LOG_INFO) << "Worker (" << dc->procid() << ") ";
#endif
  logstream(LOG_INFO) << "Starting first order stats computation" << std::endl; 

  // Dense data. 
  if (this->is_dense) {
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      DenseVector x(variables_per_class);
      double row_func = 0, margin_dot_class = 0;
      DenseVector margin(classes - 1), kernel(classes - 1), row_prob(classes -1);
      DenseMatrix pointMat(point);
      pointMat.reshape(variables_per_class, classes-1);
      size_t class_idx = 0;
      double kernel_sum = 0;
      for(auto it = data.get_iterator(thread_idx, num_threads); 
                                                              !it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMat.t() * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;
   
        kernel =  arma::exp(margin);
        kernel_sum = arma::sum(kernel);
        row_func = log1p(kernel_sum) - margin_dot_class;
        row_prob = kernel / (1 + kernel_sum);
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;

        G[thread_idx] += arma::vectorise(class_weights[class_idx] * (x * row_prob.t()));
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
      pointMat.reshape(variables_per_class, classes-1);
      arma::mat pointMatT = pointMat.t();
      size_t class_idx = 0;
      double kernel_sum = 0;
      for(auto it = data.get_iterator(thread_idx, num_threads); 
                                                              !it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMatT * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;
   
        kernel =  exp(margin);
        kernel_sum = arma::sum(kernel);
        row_func = log1p(kernel_sum) - margin_dot_class;
        row_prob = kernel / (1 + kernel_sum);

        SparseVector G_tmp(variables);
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;
        flattened_sparse_vector_outer_prod(x, row_prob, G_tmp);
        G_tmp *= class_weights[class_idx];

        G[thread_idx] += G_tmp;
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

#ifdef HAS_DISTRIBUTED
  logstream(LOG_INFO) << "Worker (" << dc->procid() << ") Computation done at " 
                      << (t.current_time() - start_time) << "s" << std::endl; 

  dc->all_reduce(gradient, true);
  dc->all_reduce(function_value, true);

  logstream(LOG_INFO) << "Worker (" << dc->procid() << ") All-reduce done at " 
                      << (t.current_time() - start_time) << "s" << std::endl; 
#else
  logstream(LOG_INFO) << "Computation done at " 
                      << (t.current_time() - start_time) << "s" << std::endl; 
#endif

}

/**
 * Compute the second order statistics
*/
void logistic_regression_opt_interface::compute_second_order_statistics(
    const ml_data& data, const DenseVector& point, DenseMatrix& hessian,
    DenseVector& gradient, double& function_value) {
    
  timer t;
  double start_time = t.current_time();
#ifdef HAS_DISTRIBUTED
  auto dc = distributed_control_global::get_instance();
  DASSERT_TRUE(dc != NULL);
  logstream(LOG_INFO) << "Worker (" << dc->procid() << ") ";
#endif
  logstream(LOG_INFO) << "Starting second order stats computation" << std::endl; 

  // Init  
  std::vector<DenseMatrix> H(n_threads, arma::zeros(variables, variables));
  std::vector<DenseVector> G(n_threads, arma::zeros(variables));
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
      pointMat.reshape(variables_per_class, classes-1);

      DenseMatrix A(classes-1, classes-1);

      for(auto it = data.get_iterator(thread_idx, num_threads); 
                                                              !it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMat.t() * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;

        kernel = exp(margin);
        kernel_sum = arma::sum(kernel);
        row_prob = kernel / (1 + kernel_sum);

        A = diagmat(row_prob) - row_prob * row_prob.t();

        row_func = log1p(kernel_sum) - margin_dot_class;
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;
        
        f[thread_idx] += class_weights[class_idx] * row_func;
        DenseMatrix G_tmp = class_weights[class_idx] * (x * row_prob.t());
        G[thread_idx] += arma::vectorise(G_tmp);
        DenseMatrix XXT = x * x.t();

        for(size_t a = 0; a < classes - 1; a++){
          for(size_t b = 0; b < classes - 1; b++){
            size_t m = variables_per_class;
            H[thread_idx].submat(a * m, b *m,
                                 (a + 1) * m - 1, (b + 1) * m - 1)
                      += class_weights[class_idx] * A(a,b) * XXT;
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
      pointMat.reshape(variables_per_class, classes-1);
      arma::mat pointMatT = pointMat.t();
      DenseMatrix A(classes-1, classes-1);
      for(auto it = data.get_iterator(thread_idx, num_threads); 
                                                              !it.done(); ++it) {
        fill_reference_encoding(*it, x);
        x(variables_per_class - 1) = 1;
        if(feature_rescaling){
          scaler->transform(x);
        }

        class_idx = it->target_index();
        margin = pointMatT * x;
        margin_dot_class = (class_idx > 0) ? margin(class_idx - 1) : 0;

        kernel =  arma::exp(margin);
        kernel_sum = arma::sum(kernel);
        row_prob = kernel / (1 + kernel_sum);

        A = diagmat(row_prob) - row_prob * row_prob.t();

        row_func = log1p(kernel_sum) - margin_dot_class;
        SparseVector G_tmp(variables);
        if (class_idx > 0) row_prob(class_idx - 1) -= 1;
        flattened_sparse_vector_outer_prod(x, row_prob, G_tmp);
        G_tmp *= class_weights[class_idx];
        G[thread_idx] += G_tmp;
        f[thread_idx] += class_weights[class_idx] * row_func;

        // Sadly, this is the fastest way to do this in Eigen. It can be done 
        // in block mode if x is dense, but when x is sparse, this seems to be 
        // faster (much faster).
        for(size_t a = 0; a < classes - 1; a++){
          for(size_t b = 0; b < classes - 1; b++){
            size_t a_index_offset = a * variables_per_class;
            size_t b_index_offset = b * variables_per_class;
            for (auto pi : x) {
              for (auto pj : x) {
                H[thread_idx](a_index_offset + pi.first,
                              b_index_offset + pj.first) +=
                  class_weights[class_idx] * pi.second * pj.second * A(a,b);
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

#ifdef HAS_DISTRIBUTED
  logstream(LOG_INFO) << "Worker (" << dc->procid() << ") Computation done at " 
                      << (t.current_time() - start_time) << "s" << std::endl; 

  dc->all_reduce(hessian, true);
  dc->all_reduce(gradient, true);
  dc->all_reduce(function_value, true);

  logstream(LOG_INFO) << "Worker (" << dc->procid() << ") All-reduce done at " 
                      << (t.current_time() - start_time) << "s" << std::endl; 
#else
  logstream(LOG_INFO) << "Computation done at " 
                      << (t.current_time() - start_time) << "s" << std::endl; 
#endif

}

void logistic_regression_opt_interface::compute_second_order_statistics(
    const DenseVector& point, DenseMatrix& hessian, DenseVector& gradient,
    double& function_value) {
  compute_second_order_statistics(
      data, point, hessian, gradient, function_value);
}

void
logistic_regression_opt_interface::compute_validation_second_order_statistics(
    const DenseVector& point, DenseMatrix& hessian, DenseVector& gradient,
    double& function_value) {
  compute_second_order_statistics(
      valid_data, point, hessian, gradient, function_value);
}


} // supervised
} // turicreate
