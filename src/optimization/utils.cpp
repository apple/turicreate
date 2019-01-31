/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <flexible_type/flexible_type.hpp>
#include <optimization/optimization_interface.hpp>
#include <optimization/utils.hpp>
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <string>

// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 1. Implement numerical hessian checks.
// 2. Implement better sparse and dense isNan & isInf functions.
// 3. Model computes residual?

namespace turi {
  
namespace optimization {

/**
 * Make sure the options dictionary always has everything any solver needs to
 * function.
 *
*/
void set_default_solver_options(const first_order_opt_interface& model, const
    DenseVector& point, const std::string solver, std::map<std::string,
    flexible_type>& opts){
    
  std::stringstream msg;

  if (size_t(model.num_variables()) != size_t(point.size())){
      msg << "Dimension mismatch: Initial point has " << point.size() <<
        " dimensions but the model has " << model.num_variables() <<
        " variables." << std::endl;
      log_and_throw(msg.str());
  }
  
  // Check that all solver options are present and make sure types are right too.
  for (const auto& kvp:  default_solver_options) {
    if (opts.count(kvp.first) == 0){
      opts[kvp.first] = kvp.second;
    }
    flex_type_enum ctype = kvp.second.get_type();
    if(not(opts[kvp.first].get_type() == ctype)){
      msg << "Optimization Error: Option "
          << kvp.first << " must be of type " << flex_type_enum_to_name(ctype)
          << std::endl; 
      log_and_throw(msg.str());
    } 
  }
  
  // Check that the options make sense.
  // --------------------------------------------------------------------------
  if (opts["max_iterations"] <= 0){
    msg << "Optimization Error: Called " << solver 
        << " with <= 0 iterations." << std::endl;
    log_and_throw(msg.str());
  }
  if (model.num_examples() == 0){
    msg << "Optimization Error: Called " << solver 
        << " on a model with no data" << std::endl;
    log_and_throw(msg.str());
  }
  if (model.num_variables()== 0){
    msg << "Optimization Error: Called " << solver 
        << " on a model with no variables." << std::endl;
    log_and_throw(msg.str());
  }
  if (opts["convergence_threshold"] < optimization::OPTIMIZATION_ZERO){
    msg << "Option convergence threshold cannot be lower than " 
       << std::scientific << std::setprecision(5) 
       << optimization::OPTIMIZATION_ZERO << "." << std::endl;
    log_and_throw(msg.str());
  }
  if (opts["step_size"] < optimization::LS_ZERO || opts["step_size"]
      > optimization::LS_MAX_STEP_SIZE){
    msg << "Step size (a.k.a learning rate) must be in [" 
        << std::scientific << std::setprecision(5) 
        << optimization::LS_ZERO << "," 
        << optimization::LS_MAX_STEP_SIZE<< "]." << std::endl;
    log_and_throw(msg.str());
  }
  if (opts["max_iterations"] < 1){
    msg << "Max iterations must be more than 1." << std::endl;
    log_and_throw(msg.str());
  }

  // Solver specific options
  // --------------------------------------------------------------------------
  
  // SGD 
  if (solver == "sgd"){
    if (opts["mini_batch_size"] <= 0){
      msg << "Optimization Error: Called " << solver 
          << "with minibatch size of 0." << std::endl;
      log_and_throw(msg.str());
    }
  }

  // L-BFGS 
  if (solver == "lbfgs" || solver == "l-bfgs"){
    if (opts["lbfgs_memory_level"] <= 0){
      msg << "L-BFGS memory level must be more than 1." 
          << std::endl;
      log_and_throw(msg.str());
    }
  }

}

/**
 * Compute residual gradient.
*/
double compute_residual(const SparseVector& gradient){

  // Make this more efficient
  DenseVector dense_grad = gradient;
  return compute_residual(dense_grad);
}

/**
 * Compute residual gradient.
*/
double compute_residual(const DenseVector& gradient){
  double infNorm = gradient.lpNorm<Eigen::Infinity>();
  return infNorm;
}

/**
 * Check gradient of first_order_optimization_iterface models at a point. 
*/
bool check_gradient(first_order_opt_interface& model, const DenseVector&
    point, const DenseVector& gradient, const size_t mbStart, const size_t
    mbSize){

  size_t mbEnd = std::min(model.num_examples(), mbStart + mbSize);
  // Check that the dimensions match
  if (gradient.cols() != point.cols()){
    logprogress_stream << "Gradient is (" << gradient.rows() << "x" << gradient.cols()
                  <<") which is mismatched with dimension of point (" 
                  << point.cols() << ")" <<  std::endl;
    return false;
  }

  // Minibatch error checking
  if (mbStart > model.num_examples()){
    logprogress_stream << "Minibatch start is " << mbStart << " but the model has "
                  << model.num_examples() << " examples." << std::endl;
    return false;
  }
  
  if (mbEnd > model.num_examples()){
    logprogress_stream << "Trying to index example" << mbEnd
                  << " but the model has " << model.num_examples() 
                  << " examples." << std::endl;
    return false;
  }


  size_t n = point.cols();                 // Dimension
  DenseVector new_point = point;           // New point
  double f_l, f_r, grad_i;
  double rel_toler;                     

  // Check gradient using central difference. 
  // Required (n+1) function computations.
  for(size_t i=0; i < n; i++){

    // Compute function values at both ends. 
    // (More accurate than forward difference)
    new_point(i) = point(i) - FINITE_DIFFERENCE_EPSILON;
    f_l = model.compute_function_value(new_point, mbStart, mbSize);
    new_point(i) = point(i);

    new_point(i) = point(i) + FINITE_DIFFERENCE_EPSILON;
    f_r = model.compute_function_value(new_point, mbStart, mbSize);
    new_point(i) = point(i);
    
    grad_i = (f_r - f_l) / (2 * FINITE_DIFFERENCE_EPSILON);
    
    // Check for relative gradiends (Safeguard against poor scaling)
    rel_toler = std::abs(gradient(i) - grad_i) / std::max(std::abs(gradient(i)), 1.0);

    // Expect the gradient to be atleast as accurate as 1e-3
    if(rel_toler >= 1e-3){
      
      logprogress_stream << "Gradient mismatch " << std::endl;
      logprogress_stream << "Index           : " << i << std::endl;
      logprogress_stream << "Minibatch start : " << mbStart << std::endl;
      logprogress_stream << "Minibatch size  : " << mbSize << std::endl;
      logprogress_stream << "Should be around " << grad_i << " but is " <<
        gradient(i) << std::endl;
      return false;
    }

  }
  return true;

} 



/**
 * Check gradient of first_order_optimization_iterface models at a point. 
 *
*/
bool check_gradient(first_order_opt_interface& model, const DenseVector&
    point, const SparseVector& gradient, const size_t mbStart, const size_t
    mbSize){

    DenseVector dense_gradient = gradient;
    return check_gradient(model, point, dense_gradient, mbStart, mbSize);
}



/**
 * Check hessian of second_order_optimization_iterface models at a point. 
 *
*/
bool check_hessian(second_order_opt_interface& model, const DenseVector& point,
    const DenseMatrix& hessian){


  if (hessian.cols() != hessian.rows()){
    logprogress_stream << "Hessian (" << hessian.rows() << "x"
                       << hessian.cols() <<") not square." << std::endl;
    return false;
  }
  if (hessian.cols() != point.size()){
    logprogress_stream << "Hessian size (" << hessian.rows() << "x"
                       << hessian.cols() <<") mismatched with variables ("
                       << point.cols() << ")" <<  std::endl;
    return false;
  }

  size_t n = point.size();                 // Dimension
  DenseVector new_point = point;           // New point
  double f_ip_jp, f_ip_jn, f_in_jp, f_in_jn, hessian_ij;
  double rel_toler;                     
  

  // Required 4n^2 function computations but is more accurate than forward
  // differences.
  for(size_t i=0; i < n; i++){
  
    for(size_t j=0; j < n; j++){

      // Compute function values at 4 points
      // More accurate than forward difference.

      // Point 1:
      if (i != j){
        new_point(i) = point(i) + FINITE_DIFFERENCE_EPSILON;
        new_point(j) = point(j) + FINITE_DIFFERENCE_EPSILON;
      } else {
        new_point(i) = point(i) + 2 * FINITE_DIFFERENCE_EPSILON;
      }
      f_ip_jp = model.compute_function_value(new_point);
      new_point(i) = point(i);
      new_point(j) = point(j);
      
      // Point 2:
      if (i != j){
        new_point(i) = point(i) + FINITE_DIFFERENCE_EPSILON;
        new_point(j) = point(j) - FINITE_DIFFERENCE_EPSILON;
      } else {
        new_point(i) = point(i);
      }
      f_ip_jn = model.compute_function_value(new_point);
      new_point(i) = point(i);
      new_point(j) = point(j);
      
      
      // Point 3:
      if (i != j){
        new_point(i) = point(i) - FINITE_DIFFERENCE_EPSILON;
        new_point(j) = point(j) + FINITE_DIFFERENCE_EPSILON;
      } else {
        new_point(i) = point(i);
      }
      f_in_jp = model.compute_function_value(new_point);
      new_point(i) = point(i);
      new_point(j) = point(j);
      
      // Point 4:
      if (i != j){
        new_point(i) = point(i) - FINITE_DIFFERENCE_EPSILON;
        new_point(j) = point(j) - FINITE_DIFFERENCE_EPSILON;
      } else {
        new_point(i) = point(i) - 2 * FINITE_DIFFERENCE_EPSILON;
      }
      f_in_jn = model.compute_function_value(new_point);
      new_point(i) = point(i);
      new_point(j) = point(j);
      
      hessian_ij = (f_ip_jp + f_in_jn - f_ip_jn - f_in_jp) / (4
          * pow(FINITE_DIFFERENCE_EPSILON,2));
      
      // Check for relative gradiends (Safeguard against poor scaling)
      rel_toler = std::abs(hessian(i,j) - hessian_ij) / std::max(std::abs(hessian(i,j)),
          1.0);

      // Expect the gradient to be atleast as accurate as 1e-3
      if(rel_toler >= 1e-3){
        
        logprogress_stream << "Hessian mismatch " << std::endl;
        logprogress_stream << "Index           : " << i << "," << j << std::endl;
        logprogress_stream << "Should be around " << hessian_ij << " but is " 
                           << hessian(i,j) << std::endl;
        return false;
      }
    }
  }

  return true;
} 



/**
 * Translate solver status to a string that the user can understand.
*/
std::string translate_solver_status(const OPTIMIZATION_STATUS& status){

  std::string ret;

  switch (status){
     case OPTIMIZATION_STATUS::OPT_UNSET:
        ret = "FAILURE: Optimizer wasn't called";
        break;
     case OPTIMIZATION_STATUS::OPT_LOADED:
        ret = "FAILURE: Model was loaded but the solution was not found.";
        break;
     case OPTIMIZATION_STATUS::OPT_OPTIMAL:
        ret = "SUCCESS: Optimal solution found.";
        break;
     case OPTIMIZATION_STATUS::OPT_ITERATION_LIMIT:
        ret = "Completed (Iteration limit reached).";
        break;
     case OPTIMIZATION_STATUS::OPT_TIME_LIMIT:
	      ret = "Completed (Time limit reached).";
        break;
     case OPTIMIZATION_STATUS::OPT_INTERRUPTED:
 	      ret = "TERMINATED: Terminated by user.";
        break;
     case OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR:
 	      ret = "TERMINATED: Terminated due to numerical difficulties.";
        break;
     case OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW:
        ret = "TERMINATED: Terminated due to numerical overflow error. ";
        break;
     case OPTIMIZATION_STATUS::OPT_LS_FAILURE:
        ret = "TERMINATED: Terminated due to numerical difficulties in line search.";
        break;
     case OPTIMIZATION_STATUS::OPT_IN_PROGRESS:
        ret = "Optimization still in progress.";
        break;
  }

  return ret;

}

/**
 * Translate solver status to a string that the user can understand.
*/
std::string get_recourse_actions(const OPTIMIZATION_STATUS& status){

  std::string ret;

  switch (status){
     case OPTIMIZATION_STATUS::OPT_UNSET:
     case OPTIMIZATION_STATUS::OPT_OPTIMAL:
     case OPTIMIZATION_STATUS::OPT_INTERRUPTED:
        ret = "";
        break;
     case OPTIMIZATION_STATUS::OPT_LOADED:
     case OPTIMIZATION_STATUS::OPT_TIME_LIMIT:
        ret = "Internal error.";
     case OPTIMIZATION_STATUS::OPT_IN_PROGRESS:
     case OPTIMIZATION_STATUS::OPT_ITERATION_LIMIT:
        ret += "This model may not be optimal. To improve it, consider ";
        ret += "increasing `max_iterations`.\n";
        break;
     case OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR:
     case OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW:
     case OPTIMIZATION_STATUS::OPT_LS_FAILURE:
        ret += "This model may not be ideal. To improve it, consider doing one of the following:\n";
        ret += "(a) Increasing the regularization.\n";
        ret += "(b) Standardizing the input data.\n";
        ret += "(c) Removing highly correlated features.\n";
        ret += "(d) Removing `inf` and `NaN` values in the training data.\n";
        break;
  }
  return ret;

}

/**
 * Pretty print solver traiing stats.
*/
void log_solver_summary_stats(const solver_return& stats, bool simple_mode){

    // Don't you love flexible type :)
    flexible_type residual;
    if (stats.residual == OPTIMIZATION_INFTY){
      residual = "Not computed.";
    } else {
      residual = stats.residual;
    }

    std::stringstream ss;
    ss << "Solution time     = " << stats.solve_time << " sec" << std::endl;
    ss << "Loss              = " << stats.func_value << std::endl;
    ss << "Iterations        = " << stats.iters  << std::endl;
    ss << "Solution Accuracy = " << residual << std::endl;
    ss << "Number of Passes  = " << stats.num_passes << std::endl;
    ss << "Function evals    = " << stats.func_evals << std::endl;
    ss << "Gradient evals    = " << stats.gradient_evals  << std::endl;
    ss << "Solver Status     = " << translate_solver_status(stats.status) 
                              << std::endl;
    logstream(LOG_INFO) << ss.str() << std::endl;
    ss.str("");

    // Print the status to the user.
    logprogress_stream <<  translate_solver_status(stats.status) << std::endl;
    if(!simple_mode) {
    logprogress_stream <<  get_recourse_actions(stats.status) << std::endl;
    }

}

/**
 * Performs left = left + right
 * \note Doing this natively in Eigen is super slow!
 * \note Speciaized implementation for the one corner case of the template function
 * vector_add
*/
template<>
void vector_add<DenseVector, SparseVector>(DenseVector& left,
                                           const SparseVector& right){
  DASSERT_EQ(left.size(), right.size());
 for (SparseVector::InnerIterator i(right); i; ++i){
   left[i.index()] += i.value();
 }
}

/**
 * Performs left = left + right
*/
template<>
void vector_add<DenseVector, DenseVector>(DenseVector& left,
                                           const DenseVector& right){
  left += right;
}

template<>
void vector_add<SparseVector, SparseVector>(SparseVector& left,
                                           const SparseVector& right){
  left += right;
}

} // optimizaiton
} // turicreate
