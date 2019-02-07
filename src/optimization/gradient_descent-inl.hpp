/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GRADIENT_DESCENT_H_
#define TURI_GRADIENT_DESCENT_H_

#include <flexible_type/flexible_type.hpp>
#include <Eigen/Core>

#include <optimization/utils.hpp>
#include <optimization/optimization_interface.hpp>
#include <optimization/regularizer_interface.hpp>
#include <optimization/line_search-inl.hpp>
#include <table_printer/table_printer.hpp>


// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 1. Constant line seach tuning?

namespace turi {
  
namespace optimization {


/**
 * \ingroup group_optimization
 * \addtogroup gradient_descent Gradient Descent
 * \{
 */

/**
 *
 * Solve a first_order_optimization_iterface model with a gradient descent
 * method.
 * 
 * \param[in,out] model  Model with first order optimization interface.
 * \param[in] init_point Starting point for the solver.
 * \param[in,out] opts   Solver options.
 * \returns stats        Solver return stats.
 * \param[in] reg        Shared ptr to an interface to a regularizer.
 * \tparam Vector        Sparse or dense gradient representation.
 *
 *
*/
template <typename Vector = DenseVector>
inline solver_return gradient_descent(first_order_opt_interface& model,
    const DenseVector& init_point, 
    std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<regularizer_interface> reg=NULL){ 

    // Benchmarking utils. 
    timer t;
    double start_time = t.current_time();

    logprogress_stream << "Starting Gradient Descent " << std::endl;
    logprogress_stream << "--------------------------------------------------------" << std::endl;
    std::stringstream ss;
    ss.str("");

    // Step 1: Algorithm option init
    // ------------------------------------------------------------------------
    // Check that all solver options are present.
    // Load options
    size_t iter_limit = opts["max_iterations"];
    double convergence_threshold = opts["convergence_threshold"];
    double step_size = opts["step_size"];
    size_t iters = 1;
    solver_return stats;

    // Print progress
    table_printer printer(
        model.get_status_header({"Iteration", "Passes", "Step size", "Elapsed Time"}));
    printer.print_header();


    // First compute the residual. Sometimes, you already have the solution
    // during the starting point. In these settings, you don't want to waste
    // time performing a step of the algorithm.
    DenseVector point = init_point; 
    Vector gradient(point.size());
    double func_value;
    model.compute_first_order_statistics(point, gradient, func_value);
    double residual = compute_residual(gradient);

    stats.func_evals++;
    stats.gradient_evals++;

    // Needs to store previous point and gradient information
    DenseVector delta_point = point;
    delta_point.setZero();
    
    // First iteration will take longer. Warn the user.
    logprogress_stream <<"Tuning step size. First iteration could take longer"
                       <<" than subsequent iterations." << std::endl;
    

    // Nan Checking!
    if (!std::isfinite(residual)) {
      stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
    }
    
    // Step 2: Algorithm starts here
    // ------------------------------------------------------------------------
    // While not converged
    while((residual >= convergence_threshold) && (iters <= iter_limit)){


      // Line search for step size. 
      ls_return ls_stats;
     
      // Pick line search based on regularizers.
      if (reg != NULL){
        step_size  *= 2;
        ls_stats =  backtracking(model, 
                                 step_size,
                                 func_value, 
                                 point, 
                                 gradient, 
                                 -gradient,
                                 reg);
      } else {
          ls_stats =  more_thuente(model, 
                                   step_size,
                                   func_value, 
                                   point, 
                                   gradient, 
                                   -gradient);
      }
      

      // Add info from line search 
      stats.func_evals += ls_stats.func_evals;
      stats.gradient_evals += ls_stats.gradient_evals;
      step_size = ls_stats.step_size;

      // Line search failed
      if (ls_stats.status == false){
        stats.status = OPTIMIZATION_STATUS::OPT_LS_FAILURE;
        break;
      }

      // \delta x_k = x_{k} - x_{k-1}
      delta_point =  point;
      point = point -step_size * gradient;
      if (reg != NULL)
        reg->apply_proximal_operator(point, step_size);
      delta_point = point - delta_point;

      // Numerical error: Insufficient progress.
      if (delta_point.norm() <= OPTIMIZATION_ZERO){
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR;
        break;
      }
      // Numerical error: Numerical overflow. (Step size was too large)
      if (!delta_point.array().array().isFinite().all()) {
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
        break;
      }
     
      // Compute residual norm (to check for convergence)
      model.compute_first_order_statistics(point, gradient, func_value);
      stats.num_passes++;
      residual = compute_residual(gradient);
      iters++;

      // Print progress
      auto stat_info = {std::to_string(iters), 
                        std::to_string(stats.num_passes),
                        std::to_string(step_size), 
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
    stats.gradient = gradient;
    stats.func_value = func_value;
    stats.solve_time = t.current_time() - start_time;
    stats.solution = point;
    stats.progress_table = printer.get_tracked_table();
    
    // Display solver stats
    log_solver_summary_stats(stats);
    return stats;
}


} // optimizaiton

/// \}
} // turicreate

#endif 

