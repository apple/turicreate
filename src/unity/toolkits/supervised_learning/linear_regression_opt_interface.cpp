/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/lib/unity_sframe.hpp>

// Toolkits
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/linear_regression.hpp>
#include <toolkits/supervised_learning/linear_regression_opt_interface.hpp>
#include <toolkits/supervised_learning/supervised_learning_utils-inl.hpp>

// Solvers
#include <optimization/utils.hpp>
#include <optimization/newton_method-inl.hpp>
#include <optimization/gradient_descent-inl.hpp>
#include <optimization/accelerated_gradient-inl.hpp>
#include <optimization/lbfgs-inl.hpp>

// Regularizer
#include <optimization/regularizers-inl.hpp>
#include <numerics/armadillo.hpp>
#include <serialization/serialization_includes.hpp>

// Distributed
#ifdef HAS_DISTRIBUTED
#include <distributed/distributed_context.hpp>
#include <rpc/dc_global.hpp>
#include <rpc/dc.hpp>
#endif

constexpr size_t LINEAR_REGRESSION_BATCH_SIZE = 1000;

// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// Supporting stats (p-values, std errors etc.)

namespace turi {
namespace supervised {

/*
 * Linear Regression Solver Interface
 * ****************************************************************************
 */

/**
 * Constructor for Linear regression solver object.dual_linear_svm_dual_opt_interface
 */
linear_regression_opt_interface::linear_regression_opt_interface(
    const ml_data& _ml_data, const ml_data& _valid_data, 
    linear_regression& _model, bool _feature_rescaling) {

  data = _ml_data;
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
  variables = get_number_of_coefficients(smodel.get_ml_metadata());
  is_dense = (variables <= 3 * features);
}

/**
 * Desctuctor for Linear regression solver object.
 */
linear_regression_opt_interface::~linear_regression_opt_interface(){
}

/**
 * Set the number of threads.
 */
void linear_regression_opt_interface::set_threads(size_t _n_threads) {
  n_threads = _n_threads;
}

/**
 * Set feature rescaling.
 */
void linear_regression_opt_interface::init_feature_rescaling() {
  feature_rescaling = true;
  scaler.reset(new l2_rescaling(smodel.get_ml_metadata(), true));
}

/**
 * Transform final solution back to the original scale.
 */
void linear_regression_opt_interface::rescale_solution(DenseVector& coefs) {
  if(feature_rescaling){
    scaler->transform(coefs);
  }
}

/**
 * Get the number of variables in the model.
 */
size_t linear_regression_opt_interface::num_variables() const{
  return variables;
}

/**
 * Get the number of examples in the model
 */
size_t linear_regression_opt_interface::num_examples() const{
  return examples;
}

/**
 * Get the number of validation-set examples in the model
 */
size_t linear_regression_opt_interface::num_validation_examples() const{
  return valid_data.num_rows();
}


/**
 * Get strings needed to print the header for the progress table.
 */
std::vector<std::pair<std::string, size_t>> 
linear_regression_opt_interface::get_status_header(const std::vector<std::string>& stat_headers) {
  bool has_validation_data = (valid_data.num_rows() > 0);
  auto header = make_progress_header(smodel, stat_headers, has_validation_data); 
  return header;
}

/**
 * Get strings needed to print a row of the progress table.
 */
std::vector<std::string> linear_regression_opt_interface::get_status(
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
 * Compute the first order statistics
*/
void linear_regression_opt_interface::compute_first_order_statistics(const
    DenseVector& point, DenseVector& gradient, double& function_value, const
    size_t mbStart, const size_t mbSize) {
  DASSERT_TRUE(mbStart == 0);
  DASSERT_TRUE(mbSize == (size_t)(-1));

  // Init
  std::vector<DenseVector> G(n_threads, arma::zeros(variables));
  std::vector<double> f(n_threads, 0.0);

  // Dense data. 
  if (this->is_dense) {
      in_parallel([&](size_t thread_idx, size_t num_threads) {
        DenseMatrix x(LINEAR_REGRESSION_BATCH_SIZE, variables);
        DenseVector y(LINEAR_REGRESSION_BATCH_SIZE);
        size_t row_id = 0;
        for(auto it = data.get_iterator(thread_idx, num_threads);
                                                                   !it.done();) {

          // Fill.
          row_id = 0;
          while( (row_id < LINEAR_REGRESSION_BATCH_SIZE) && !(it.done())) {
            fill_reference_encoding(*it, x.row(row_id));
            x(row_id, variables - 1) = 1;
            y(row_id) = it->target_value();
            ++it;
            ++row_id;
          }

          // Resize will happen only once (last few rows).
          x.resize(row_id, variables);
          y.resize(row_id);
          if(feature_rescaling){
            scaler->transform(x);
          }
          
          // Compute
          DenseVector r = x * point - y;
          G[thread_idx] += 2 * x.t() * r;
          f[thread_idx] += dot(r, r);
        }
      });

  // Sparse data.
  } else {
      in_parallel([&](size_t thread_idx, size_t num_threads) {
        SparseVector x(variables);
        double r = 0;
        for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
          // Fill.
          fill_reference_encoding(*it, x);
          x(variables - 1) = 1;
          if(feature_rescaling){
            scaler->transform(x);
          }

          // Compute.
          r = dot(x, point) - it->target_value();
          for(auto p : x) {
            G[thread_idx][p.first]  += 2 * r * p.second;
          }
          f[thread_idx] += r * r;
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
  auto dc = distributed_control_global::get_instance();
  DASSERT_TRUE(dc != NULL);
  dc->all_reduce(gradient, true);
  dc->all_reduce(function_value, true);
#endif
}

/**
 * Compute the second order statistics
*/
void linear_regression_opt_interface::compute_second_order_statistics(
    const ml_data& data, const DenseVector& point, DenseMatrix& hessian,
    DenseVector& gradient, double& function_value) {

  std::vector<DenseMatrix> H(n_threads, 
                        arma::zeros(variables,variables));
  std::vector<DenseVector> G(n_threads, 
                        arma::zeros(variables));
  std::vector<double> f(n_threads, 0.0);
  
  // Dense data. 
  if (this->is_dense) {
    in_parallel([&](size_t thread_idx, size_t num_threads) {
      DenseMatrix x(LINEAR_REGRESSION_BATCH_SIZE, variables);
      DenseVector y(LINEAR_REGRESSION_BATCH_SIZE);
      size_t row_id = 0;
      for(auto it = data.get_iterator(thread_idx, num_threads); 
                                                                !it.done();) {
        row_id = 0;
        while( (row_id < LINEAR_REGRESSION_BATCH_SIZE) && !(it.done())) {
          fill_reference_encoding(*it, x.row(row_id));
          x(row_id, variables - 1) = 1;
          y(row_id) = it->target_value();
          ++it;
          ++row_id;
        }

        // Resize will happen only once (last few rows).
        x.resize(row_id, variables);
        y.resize(row_id);
        if(feature_rescaling){
          scaler->transform(x);
        }

        // Compute
        DenseVector r = x * point - y;
        G[thread_idx] += 2 * x.t() * r;
        f[thread_idx] += dot(r, r);
        H[thread_idx] += 2 * x.t() * x;
      }
    });

  // Sparse data
  } else {
      in_parallel([&](size_t thread_idx, size_t num_threads) {
        SparseVector x(variables);
        double y = 0, r = 0;
        for(auto it = data.get_iterator(thread_idx, num_threads); 
                               !it.done(); ++it) {
          // Fill.
          fill_reference_encoding(*it, x);
          y = it->target_value();
          x(variables - 1) = 1;
          if(feature_rescaling){
            scaler->transform(x);
          }
                            
          // Compute.       
          r = dot(x, point) - y;
          for(auto p : x) {
            G[thread_idx][p.first]  += 2 * r * p.second;
          }

          f[thread_idx] += r * r;

          for (auto pi : x) {
            for (auto pj : x) {
              H[thread_idx](pi.first, pj.first) += 2 * pi.second * pj.second;
            }
          }
        }
      });
  }

  // Reduce on threads.
  function_value = f[0];
  hessian = H[0];
  gradient = G[0];
  for(size_t i=1; i < n_threads; i++){
    hessian += H[i];
    gradient += G[i];
    function_value += f[i];
  }
 
#ifdef HAS_DISTRIBUTED
  auto dc = distributed_control_global::get_instance();
  DASSERT_TRUE(dc != NULL);
  dc->all_reduce(hessian, true);
  dc->all_reduce(gradient, true);
  dc->all_reduce(function_value, true);
#endif

}

void linear_regression_opt_interface::compute_second_order_statistics(
    const DenseVector& point, DenseMatrix& hessian, DenseVector& gradient,
    double& function_value) {
  compute_second_order_statistics(
      data, point, hessian, gradient, function_value);
}

void
linear_regression_opt_interface::compute_validation_second_order_statistics(
    const DenseVector& point, DenseMatrix& hessian, DenseVector& gradient,
    double& function_value) {
  compute_second_order_statistics(
      valid_data, point, hessian, gradient, function_value);
}

} // supervised
} // turicreate
