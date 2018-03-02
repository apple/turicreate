/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_NEWTON_METHOD_H_
#define TURI_NEWTON_METHOD_H_

#include <optimization/optimization_interface.hpp>
#include <flexible_type/flexible_type.hpp>
#include <numerics/armadillo.hpp>

#include <optimization/utils.hpp>
#include <optimization/optimization_interface.hpp>
#include <optimization/regularizer_interface.hpp>
#include <optimization/line_search-inl.hpp>
#include <table_printer/table_printer.hpp>

// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 1. Sparse hessian newton method?

namespace turi {
  
namespace optimization {


/**
 * \ingroup group_optimization
 * \addtogroup Newton Newton Method
 * \{
 */

/**
 *
 * Solve a second_order_optimization_interface model with a (dense) hessian Newton
 * method.
 *
 * \param[in,out] model  Model with second order optimization interface.
 * \param[in] init_point Starting point for the solver.
 * \param[in,out] opts   Solver options.
 * \param[in]      reg   Shared ptr to an interface to a smooth regularizer.
 * \param[out] stats     Solver return stats.
 * \tparam Vector        Sparse or dense gradient representation.
 *
 * \note The hessian is always computed as a dense matrix. Only gradients are
 * allowed to be sparse. The implementation of Newton method must change when
 * the hessian is sparse. I.e we can no longer perform an LDLT decomposition to
 * invert the hessian matrix. We have to switch methods to Conjugate gradient
 * or Sparse LDLT decomposition.
 *
*/
template <typename Vector = DenseVector>
inline solver_return newton_method(second_order_opt_interface& model,
    const DenseVector& init_point, 
    std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<smooth_regularizer_interface> reg=NULL){ 

    // Benchmarking utils. 
    timer t;
    double start_time = t.current_time();
    solver_return stats;

    logprogress_stream << "Starting Newton Method " << std::endl;
    logprogress_stream << "--------------------------------------------------------" << std::endl;
    std::stringstream ss;
    ss.str("");
       
    // Step 1: Algorithm option init
    // ------------------------------------------------------------------------
    // Load options
    size_t iter_limit = opts["max_iterations"];
    double convergence_threshold = opts["convergence_threshold"];
    double step_size = 1;
    size_t iters = 0;

    // Log iteration and residual norms
    table_printer printer(model.get_status_header(
                        {"Iteration", "Passes", "Elapsed Time"}));
    printer.print_header();

    // First compute the gradient. Sometimes, you already have the solution
    // during the starting point. In these settings, you don't want to waste
    // time performing a newton the step. 
    DenseVector point = init_point;
    Vector gradient(point.size());
    DenseVector reg_gradient(point.size());
    DenseMatrix hessian(gradient.size(), gradient.size());
    DiagonalMatrix reg_hessian(gradient.size());
    double func_value;
    double relative_error;

    // Compute gradient (Add regularizer gradient)
    model.compute_second_order_statistics(point, hessian, gradient, func_value);
    stats.num_passes++;
    if (reg != NULL){
      reg->compute_gradient(point, reg_gradient);
      gradient += reg_gradient;
    }
    double residual = compute_residual(gradient);
    
    // Keep track of previous point 
    DenseVector delta_point = point;
    delta_point.zeros();


    // Nan Checking!
    if (std::isnan(residual) || std::isinf(residual)){
      stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
    }

    // Step 2: Algorithm starts here
    // ------------------------------------------------------------------------
    // While not converged
    while((residual >= convergence_threshold) && (iters < iter_limit)){

      // Add regularizer hessian
      if (reg != NULL){
        reg->compute_hessian(point, reg_hessian);
        hessian += diagmat(reg_hessian);
      }

      // OLD EIGEN CODE: delta_point = -step_size * hessian.ldlt().solve(gradient);
      //delta_point = (-step_size) * arma::solve(hessian, gradient);
      delta_point = (-step_size) * solve_ldlt(hessian, gradient);

      DenseVector pika = hessian*delta_point + gradient;
      relative_error = arma::norm(pika, 2)
        / std::max(arma::norm(gradient, 2), OPTIMIZATION_ZERO);


      // LDLT Decomposition failed. 
      if (relative_error > convergence_threshold){
        logprogress_stream << "WARNING: Matrix is close to being singular or"
          << " badly scaled. The solution is accurate only up to a tolerance of " 
          << relative_error << ". This typically happens when regularization"
          << " is not sufficient. Consider increasing regularization." 
          << std::endl;
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR;
        break;
      }

      // Update the new point and gradient
      point = point + delta_point;
      
      // Numerical overflow. (Step size was too large)
      if (!delta_point.is_finite()) {
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
        break;
      }

      model.compute_second_order_statistics(point, hessian, gradient, func_value);
      if (reg != NULL){
        reg->compute_gradient(point, reg_gradient);
        gradient += reg_gradient;
      }
      residual = compute_residual(gradient);
      stats.num_passes++;
      iters++;
    
      // Log info for debugging. 
      logstream(LOG_INFO) << "Iters  (" << iters << ") " 
                          << "Passes (" << stats.num_passes << ") " 
                          << "Residual (" << residual << ") " 
                          << "Loss (" << func_value << ") " 
                          << std::endl;

      // Check for nan's in the function value.
      if(std::isinf(func_value) || std::isnan(func_value)) {
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR;
        break;
      }

      // Print progress
      auto stat_info = {std::to_string(iters), 
                        std::to_string(stats.num_passes),
                        std::to_string(t.current_time())};
      auto row = model.get_status(point, stat_info);
      printer.print_progress_row_strs(iters, row);
    }
    printer.print_footer();

    // Step 3: Return optimization model status.
    // ------------------------------------------------------------------------
    if (stats.status == OPTIMIZATION_STATUS::OPT_UNSET) {
      if (iters < iter_limit){
        stats.status = OPTIMIZATION_STATUS::OPT_OPTIMAL;
      } else {
        stats.status = OPTIMIZATION_STATUS::OPT_ITERATION_LIMIT;
      }
    }
    stats.iters = iters;
    stats.residual = residual;
    stats.func_value = func_value;
    stats.solve_time = t.current_time() - start_time;
    stats.solution = point;
    stats.gradient = gradient;
    stats.hessian = hessian;
    stats.progress_table = printer.get_tracked_table();
    
    // Display solver stats
    log_solver_summary_stats(stats);

    return stats;

}


} // optimizaiton

} // turicreate

#endif 

