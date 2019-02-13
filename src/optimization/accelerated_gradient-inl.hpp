/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ACCELERATED_GRADIENT_H_
#define TURI_ACCELERATED_GRADIENT_H_

// Types
#include <flexible_type/flexible_type.hpp>
#include <Eigen/Core>

// Optimization
#include <optimization/utils.hpp>
#include <optimization/optimization_interface.hpp>
#include <optimization/regularizer_interface.hpp>
#include <optimization/line_search-inl.hpp>
#include <table_printer/table_printer.hpp>


// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 1. FISTA's proximal abstraction for regularizers.
// 2. Perf improvement for sparse gradients.

namespace turi {
  
namespace optimization {

/**
 * \ingroup group_optimization
 * \addtogroup FISTA FISTA
 * \{
 */

/**
 *
 * Solve a first_order_optimization_iterface model with a dense accelerated 
 * gradient method.
 *
 * The algorithm is based on FISTA with backtracking (Beck and Teboulle 2009).
 * Details are in Page 194 of [1]. 
 * 
 * \param[in,out] model  Model with first order optimization interface.
 * \param[in] init_point Starting point for the solver.
 * \param[in,out] opts   Solver options.
 * \param[in] reg        Shared ptr to an interface to a regularizer.
 * \returns stats        Solver return stats.
 * \tparam Vector        Sparse or dense gradient representation.
 *
 * \note Fista is an accelerated gradient method. We can try Nesterov's
 * accelerated gradient method too.
 *
 * References:
 *
 * [1] Beck, Amir, and Marc Teboulle. "A fast iterative shrinkage-thresholding
 * algorithm for linear inverse problems." SIAM Journal on Imaging Sciences 2.1
 * (2009): 183-202.  http://mechroom.technion.ac.il/~becka/papers/71654.pdf
 *
 *
*/
template <typename Vector = DenseVector>
inline solver_return accelerated_gradient(first_order_opt_interface& model,
    const DenseVector& init_point, 
    std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<regularizer_interface> reg=NULL){ 

    // Benchmarking utils. 
    timer tmr;
    double start_time = tmr.current_time();
    logprogress_stream << "Starting Accelerated Gradient (FISTA)" << std::endl;
    logprogress_stream << "--------------------------------------------------------" << std::endl;
    std::stringstream ss;
    ss.str("");

    // First iteration will take longer. Warn the user.
    logprogress_stream <<"Tuning step size. First iteration could take longer"
                       <<" than subsequent iterations." << std::endl;

    // Print progress 
    table_printer printer(
        model.get_status_header({"Iteration", "Passes", "Step size", "Elapsed Time"}));
    printer.print_header();
   
    // Step 1: Algorithm option init
    // ------------------------------------------------------------------------

    size_t iter_limit = opts["max_iterations"];
    double convergence_threshold = opts["convergence_threshold"];
    size_t iters = 0;
    double step_size = opts["step_size"];
    solver_return stats;

    // Store previous point and gradient information
    DenseVector point = init_point;                   // Initial point
    DenseVector delta_point = point;                  // Step taken
    DenseVector y = point;                            // Momentum
    DenseVector xp = point;                           // Point in the previos iter
    DenseVector x = point;                            // Point in the current 
    delta_point.setZero();

    // First compute the residual. Sometimes, you already have the solution
    // during the starting point. In these settings, you don't want to waste
    // time performing a step of the algorithm. 
    Vector gradient(point.size());
    double fy;
    model.compute_first_order_statistics(y, gradient, fy);
    double residual = compute_residual(gradient);
    stats.num_passes++;
    
    std::vector<std::string> stat_info = {std::to_string(iters),
                                          std::to_string(stats.num_passes),
                                          std::to_string(step_size),
                                          std::to_string(tmr.current_time())};
    std::vector<std::string> row = model.get_status(point, stat_info);
    printer.print_progress_row_strs(iters, row);

    // Value of parameters t in itersation k-1 and k
    double t = 1;                           // t_k   
    double tp = 1;                          // t_{k-1}
    double fply, Qply;
    
    // Nan Checking!
    if (std::isnan(residual) || std::isinf(residual)){
      stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
    }
    
    // Step 2: Algorithm starts here
    // ------------------------------------------------------------------------
    // While not converged
    while((residual >= convergence_threshold) && (iters < iter_limit)){

    
      // Auto tuning the step_size
      while (step_size > LS_ZERO){

        // FISTA with backtracking
        // Equation 4: Page 194 of (1)
        
        // Test point 
        //      point = prox(y - \grad_f(y) * s) (where s is the step size)
        point = y - gradient * step_size;
        if(reg != NULL)
          reg->apply_proximal_operator(point, step_size);

        // Compute 
        // f(point)
        fply = model.compute_function_value(point);
        stats.func_evals++;
        
        // Compute 
        // f(y) + 0.5 * s * |delta_point|^2 + delta_point^T \grad_f(y)
        delta_point = (point - y);
        Qply = fy  + delta_point.dot(gradient) + 0.5
          * delta_point.squaredNorm() / step_size;
        
        if (fply < Qply){
          break;
        }

        // Reduce step size until a sufficient decrease is satisfied.
        step_size /= 1.5;

      }

      // FISTA Iteration
      // Equation 4: Page 193 of (1)
      x = point;
      t = (1 + sqrt(1 + 4*tp*tp))/2;
      y = x + (tp - 1)/t * (x - xp);
     
      delta_point =  x - xp;
      xp = x;
      tp = t;

      // Numerical error: Insufficient progress.
      if (delta_point.norm() <= OPTIMIZATION_ZERO){
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR;
        break;
      }
      // Numerical error: Numerical overflow. (Step size was too large)
      if (!delta_point.array().isFinite().all()) {
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
        break;
      }
      
      // Compute residual norm (to check for convergence)
      model.compute_first_order_statistics(y, gradient, fy);
      stats.num_passes++;
      // Changed the convergence criterion to stop when no progress is being
      // made.
      residual = compute_residual(delta_point);
      iters++;
      
      // Check for nan's in the function value.
      if(std::isinf(fy) || std::isnan(fy)) {
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR;
        break;
      }

      // Print progress
      stat_info = {std::to_string(iters),
                   std::to_string(stats.num_passes),
                   std::to_string(step_size),
                   std::to_string(tmr.current_time())};
      row = model.get_status(point, stat_info);
      printer.print_progress_row_strs(iters, row);
      
      // Log info for debugging. 
      logstream(LOG_INFO) << "Iters  (" << iters << ") " 
                          << "Passes (" << stats.num_passes << ") " 
                          << "Residual (" << residual << ") " 
                          << "Loss (" << fy << ") " 
                          << std::endl;
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
    stats.func_value = fy;
    stats.gradient = gradient;
    stats.solve_time = tmr.current_time() - start_time;
    stats.solution = point;
    stats.progress_table = printer.get_tracked_table();
    
    // Display solver stats
    log_solver_summary_stats(stats);
    
    return stats;
}

/// \}

} // optimizaiton

} // turicreate

#endif 

