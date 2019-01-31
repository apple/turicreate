/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LBFGS_2_H_
#define TURI_LBFGS_2_H_

#include <optimization/optimization_interface.hpp>
#include <flexible_type/flexible_type.hpp>
#include <sframe/sframe.hpp>

#include <optimization/utils.hpp>
#include <optimization/optimization_interface.hpp>
#include <optimization/regularizer_interface.hpp>
#include <optimization/line_search-inl.hpp>
#include <Eigen/Core>

typedef Eigen::VectorXd DenseVector;
typedef Eigen::MatrixXd DenseMatrix;

namespace turi {
  
namespace optimization {

/**
 * Solver status.
 */
struct solver_status {
  size_t iteration = 0;                         /*!< Iterations taken */
  double solver_time = 0;                       /*!< Wall clock time (s) */
  DenseVector solution;                         /*!< Current Solution */
  DenseVector gradient;                         /*!< Current gradient */
  DenseMatrix hessian;                          /*!< Current hessian */
  double residual = NAN;                        /*!< Residual norm */
  double function_value = NAN;                  /*!< Function value */
  size_t num_function_evaluations = 0;          /*!< Function evals */
  size_t num_gradient_evaluations = 0;          /*!< Gradient evals */
  double step_size = 0;                         /*!< Current step size */

  OPTIMIZATION_STATUS status = OPTIMIZATION_STATUS::OPT_UNSET;  /*!< Status */
};


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
 * \param[in]     model  Model with first order optimization interface.
 * \param[in] init_point Starting point for the solver.
 * \param[in]     opts   Solver options.
 * \param[in]      reg   Shared ptr to an interface to a smooth regularizer.
 * \returns stats        Solver return stats.
 * \tparam Vector        Sparse or dense gradient representation.
 *
 */
class lbfgs_solver {
 public:

  /** Construct the solver around a specific model interface.
   *
   * \param[in]     model  Model with first order optimization interface.
   */
  lbfgs_solver(std::shared_ptr<first_order_opt_interface> _model)
      : model(_model) {}

  /** Sets up (or resets) the solver.
   *
   * \param[in] init_point Starting point for the solver.
   * \param[in]     opts   Solver options.  Options are "lbfgs_memory_level" and
   *                       "convergence_threshold".  If not given, defaults are
   *                       taken from the table in optimization_interface.hpp.
   *
   * \param[in]      reg   Shared ptr to an interface to a smooth regularizer.
   */
  void setup(const DenseVector& init_point,
             const std::map<std::string, flexible_type>& opts,
             const std::shared_ptr<smooth_regularizer_interface>& reg = nullptr);

  /** Perform the next update of the solution. 
   *
   *  Call this method repeatedly to perform the optimization.  
   *  Each iteration updates the solution point with one step.
   */
  bool next_iteration();

  /** The status after a given iteration.
   *
   *
   *  The best solution so far is given by status().solution.
   *
   */
  const solver_status& status() const { return m_status; }

 private:
  
  timer compute_timer;

  // The model used in the optimization.
  std::shared_ptr<first_order_opt_interface> model;

  std::shared_ptr<smooth_regularizer_interface> reg;
  
  size_t num_variables = 0;
  size_t lbfgs_memory_level = 0;
  double function_value = NAN, function_scaling_factor = 1.0;

  solver_status m_status; 
  
  // LBFGS storage
  // The search steps and gradient differences are stored in a order
  // controlled by the start point.

  // Step difference (prev m iters)
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> y;
  
  // Gradient difference (prev m iters)
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> s;

  DenseVector q;         // Storage required for the 2-loop recursion
  DenseVector rho;       // Scaling factors (prev m iters)
  DenseVector alpha;     // Step sizes (prev m iters)

  // Buffers used internally.  The function value and gradient here is scaled my
  // m_status.function_scaling_value for numerical stability.
  DenseVector delta_point, gradient, delta_grad, previous_gradient;

  double convergence_threshold = 0;
};

// Old version for backwards compatibility with the previous interface.
// Includes printing.
solver_return lbfgs_compat(
    std::shared_ptr<first_order_opt_interface> model,
    const DenseVector& init_point,
    const std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<smooth_regularizer_interface>& reg = nullptr);

/** Solves lbgfgs problem end-to-end.
 *
 *  This class wraps the above iterative solver in a convenience function,
 * iterating the solution until completion.
 *
 *  \param model The implementation of first_order_opt_interface used in the
 * optimization.
 *
 *  \param init_point The initial point at which the optimization starts.
 *
 *  \param opts The options.  Uses all the options given to setup() in the
 * lbfgs_solver class, plus "max_iterations" to terminate the optimization after
 * a given number of iterations.
 *
 * \param reg Optional regularization interface.
 *
 */
solver_status lbfgs(
    std::shared_ptr<first_order_opt_interface> model,
    const DenseVector& init_point,
    const std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<smooth_regularizer_interface>& reg = nullptr);



}  // namespace optimization

/// \}
}  // namespace turi

#endif
 
