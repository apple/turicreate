/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_lbfgs_H_
#define TURI_lbfgs_H_

#include <optimization/optimization_interface.hpp>
#include <flexible_type/flexible_type.hpp>
#include <numerics/armadillo.hpp>
#include <sframe/sframe.hpp>

#include <optimization/utils.hpp>
#include <optimization/optimization_interface.hpp>
#include <optimization/regularizer_interface.hpp>
#include <optimization/line_search-inl.hpp>
#include <table_printer/table_printer.hpp>
#include <numerics/armadillo.hpp>


// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 1. Performance improvement for Sparse gradients.

namespace turi {
  
namespace optimization {


/**
 * \ingroup group_optimization
 * \addtogroup LBFGS LBFGS
 * \{
 */

/**
 *
 * Solve a first_order_optimization_iterface model with an LBFGS
 * implementation.
 * 
 * The implementation is based on Algorithm 7.4 (pg 178) of [1].
 *
 *  This subroutine solves an unconstrained minimization problem
 *  using the limited memory BFGS method. The routine is especially
 *  effective on problems involving a large number of variables. In
 *  a typical iteration of this method an approximation Hk to the
 *  inverse of the Hessian is obtained by applying M BFGS updates to
 *  a diagonal matrix Hk0, using information from the previous M steps.
 *  The user specifies the number M, which determines the amount of
 *  storage required by the routine. The user may also provide the
 *  diagonal matrices Hk0 if not satisfied with the default choice.
 *  The algorithm is described in [2].
 *
 *  The user is required to calculate the function value and its
 *  gradient. 
 *
 *  The steplength is determined at each iteration by means of the
 *  line search routine MCVSRCH, which is a slight modification of
 *  the routine CSRCH written by More' and Thuente.
 *
 * 
 * References:
 *
 * (1) Wright S.J  and J. Nocedal. Numerical optimization. Vol. 2. 
 *                         New York: Springer, 1999.
 *  
 * (2) "On the limited memory BFGS method for large scale optimization", by D.
 * Liu and J. Nocedal, Mathematical Programming B 45 (1989) 503-528.
 *
 * \param[in,out] model  Model with first order optimization interface.
 * \param[in] init_point Starting point for the solver.
 * \param[in,out] opts   Solver options.
 * \param[in]      reg   Shared ptr to an interface to a smooth regularizer.
 * \returns stats        Solver return stats.
 * \tparam Vector        Sparse or dense gradient representation.
 *
*/
template <typename Vector = DenseVector>
inline solver_return lbfgs(first_order_opt_interface& model, 
    const DenseVector& init_point, 
    std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<smooth_regularizer_interface> reg=NULL){ 

    // Benchmarking utils. 
    timer t;
    double start_time = t.current_time();
    bool simple_mode = opts.count("simple_mode") && (opts.at("simple_mode"));

    if (!simple_mode) {
    logprogress_stream << "Starting L-BFGS " << std::endl;
    logprogress_stream 
         << "--------------------------------------------------------" 
         << std::endl;
    std::stringstream ss;
    ss.str("");
    } else { 
      logprogress_stream << "Calibrating solver; this may take some time." << std::endl;
    }
    
    // Step 1: Algorithm init
    // ------------------------------------------------------------------------
    // Check that all solver options are present.
    size_t iter_limit = opts["max_iterations"];
    double convergence_threshold = opts["convergence_threshold"];
    double step_size = 1.0 / model.num_examples(); // Esimate of Lipshipz
    int iters = 0;

    // Print status 
    auto header =
        (simple_mode ? model.get_status_header({"Iteration", "Elapsed Time"})
                     : model.get_status_header({"Iteration", "Passes",
                                                "Step size", "Elapsed Time"}));

    table_printer printer(header);
    printer.print_header();

    int m = opts["lbfgs_memory_level"];  // Memory level in LBFGS
    size_t n = model.num_variables();    // Dimension of point

    solver_return stats;

    // Keep track of previous point 
    DenseVector point = init_point;
    DenseVector delta_point = point;
    delta_point.zeros();
    
    // First compute the gradient. Sometimes, you already have the solution
    // during the starting point. In these settings, you don't want to waste
    // time performing the step. 
    Vector gradient(point.size());
    DenseVector reg_gradient(point.size());
    DenseVector delta_grad = gradient;
    DenseVector new_grad = gradient;
    delta_grad.zeros();

    // Add regularizer to gradients.
    double func_value;
    model.compute_first_order_statistics(point, gradient, func_value);
    stats.num_passes++;
    if (reg != NULL){
      reg->compute_gradient(point, reg_gradient);
      gradient += reg_gradient;
    }
    double residual = compute_residual(gradient);
    double fprevious = func_value;
    bool tune_step_size = true;

    std::vector<std::string> stat_info =
          (simple_mode
               ? std::vector<std::string>{std::to_string(iters),
              std::to_string(t.current_time())}
               : std::vector<std::string>{std::to_string(iters),
                     std::to_string(stats.num_passes),
                     "NaN",
                     std::to_string(t.current_time())});
    std::vector<std::string> row = model.get_status(point, stat_info);
    printer.print_progress_row_strs(iters, row);

    // LBFGS storage
    // The search steps and gradient differences are stored in a order 
    // controlled by the start point.
    DenseMatrix y(n,m);          // Step difference (prev m iters) 
    DenseMatrix s(n,m);          // Gradient difference (prev m iters) 
    DenseVector H0(n);           // Init Diagonal matrix (stores as a vector)
    DenseVector q(n);            // Storage required for the 2-loop recursion
    DenseVector r(n);            // Storage required for the 2-loop recursion
    double beta;                 // Storage required for the 2-loop recursion
    double gamma;                // Scaling factor of Initial hessian
    DenseVector rho(m);          // Scaling factors (prev m iters) 
    DenseVector alpha(m);        // Step sizes (prev m iters)

    int start_point = -1;
    H0.ones();
    s.zeros();
    y.zeros();
    r.zeros();
    q.zeros();
    rho.zeros();
    alpha.zeros();
    beta = 0;
    gamma = 0;
    

    // Nan Checking!
    if (!std::isfinite(residual)) {
      stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
    }

    // Step 2: Algorithm starts here
    // ------------------------------------------------------------------------
    // While not converged  
    ls_return ls_stats; 
    while((residual >= convergence_threshold) && (size_t(iters) < iter_limit)){

      // Perform gradient descent in the first iteration
      if (iters == 0){

        // Line search
        double reg_func = 0;
        if (reg != NULL)
          reg_func = reg->compute_function_value(point);

        ls_stats = more_thuente(model, 
                                step_size,
                                func_value + reg_func, 
                                point, 
                                gradient, 
                                -gradient,
                                reg);

        // Add info from line search 
        stats.func_evals += ls_stats.func_evals;
        stats.gradient_evals += ls_stats.gradient_evals;
        stats.num_passes += ls_stats.num_passes;

        // Line search failed
        if (ls_stats.status == false){
          stats.status = OPTIMIZATION_STATUS::OPT_LS_FAILURE;
          break;
        }

        delta_point = - ls_stats.step_size * gradient;
        point = point + delta_point;

      // Two loop recursion to compute the direction
      // Algorithm 7.4 of Reference [1]
      }else{
        
        q = gradient;
        
        /**
         *  Data is stored in a cyclic format using the following indexiing:  
         * 
         *   Iteration              Storage location
         *  *****************************************************
         *     iter-1               start_point
         *     iter-2               (start_point + 1) % m
         *      ...                  ...
         *     iter-m               (start_point + m - 1) % m
         * 
        **/
        for (int j = 0; j <= std::min(iters, m-1); j++){
          int i = (start_point + j) % m;
          alpha(i) = rho(i) * dot(s.col(i),q);
          q = q - alpha(i) * y.col(i);
        }
        
        // Scaling factor according to Pg 178 of [1]. This ensures that the 
        // problem is better scaled and that a step size of 1 is mostly accepted.
        gamma = 1 / (squared_norm(y.col(start_point)) * rho(start_point));
        r = gamma * (arma::diagmat(H0) * q);
        for (int j = std::min(iters, m-1); j >=0 ; j--){
          int i = (start_point + j) % m;
          beta = rho(i) * dot(y.col(i), r);
          r = r + s.col(i) * (alpha(i) - beta);
        }

        // Update the new point and gradient
        double reg_func = 0;
        if (reg != NULL)
          reg_func = reg->compute_function_value(point);

        if (tune_step_size == true){
          ls_stats = more_thuente(model, 
                                 1,
                                 func_value + reg_func, 
                                 point, 
                                 gradient, 
                                 -r,
                                 reg);
          // Add info from line search 
          stats.func_evals += ls_stats.func_evals;
          stats.gradient_evals += ls_stats.gradient_evals;
          stats.num_passes += ls_stats.num_passes;
          
          // Line search failed
          if (ls_stats.status == false){
            stats.status = OPTIMIZATION_STATUS::OPT_LS_FAILURE;
            break;
          }
          tune_step_size = false;
        }

        delta_point = - ls_stats.step_size * r;
        point = point + delta_point;

      }
      
      // Numerical error: Insufficient progress.
      if (arma::norm(delta_point, 2) <= OPTIMIZATION_ZERO){
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR;
        break;
      }
      // Numerical error: Numerical overflow. (Step size was too large)
      if (!delta_point.is_finite()) {
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW;
        break;
      }

      // Compute the new gradient and store the difference in gradients
      model.compute_first_order_statistics(point, new_grad, func_value);
      stats.num_passes++;
      if (reg != NULL){
        reg->compute_gradient(point, reg_gradient);
        new_grad += reg_gradient;
      }
      delta_grad = new_grad -  gradient;
      gradient = new_grad;
      residual = compute_residual(gradient);

      // Check for nan's in the function value.
      if(!std::isfinite(func_value)) {
        stats.status = OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR;
        break;
      }
      
          
      if(func_value > fprevious){
        tune_step_size = true;
      }
      fprevious = func_value;

      
      // Store the previous gradient difference, step difference and rho
      start_point = (start_point + 1) % m;
      s.col(start_point) = delta_point;
      y.col(start_point) = delta_grad;
      rho(start_point) = 1/ (dot(delta_point, delta_grad));
      iters++;
      
      // Log info for debugging. 
      logstream(LOG_INFO) << "Iters  (" << iters << ") " 
                          << "Passes (" << stats.num_passes << ") " 
                          << "Residual (" << residual << ") " 
                          << "Loss (" << func_value << ") " 
                          << std::endl;

      // Print progress
      stat_info =
          (simple_mode
               ? std::vector<std::string>{std::to_string(iters),
                                          std::to_string(t.current_time())}
               : std::vector<std::string>{std::to_string(iters),
                        std::to_string(stats.num_passes),
                        std::to_string(ls_stats.step_size),
                                          std::to_string(t.current_time())});

      row = model.get_status(point, stat_info);
      printer.print_progress_row_strs(iters, row);

    }
    printer.print_footer();


    // Step 3: Return optimization model status.
    // ------------------------------------------------------------------------
    if (stats.status == OPTIMIZATION_STATUS::OPT_UNSET) {
      if (size_t(iters) < iter_limit){
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
    stats.progress_table = printer.get_tracked_table();
    
    // Display solver stats
    log_solver_summary_stats(stats, simple_mode);

    return stats;
}


} // optimizaiton

/// \}
} // turicreate

#endif 

