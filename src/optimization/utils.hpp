/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_OPTIMIZATION_UTILS_H_
#define TURI_OPTIMIZATION_UTILS_H_



#include <optimization/optimization_interface.hpp>
#include <flexible_type/flexible_type.hpp>
#include <Eigen/Core>

namespace turi {

namespace optimization { 



/**
 * \ingroup group_optimization
 * \addtogroup utils Utility Functions
 * \{
 */


/**
 *
 * Basic solver error checking and default option hanlding.
 *
 * This function takes in a dictionary of solver options as input. Keys in opts
 * that are required by the solver and NOT in opts are set to a default value.
 * 
 * \param[in] model       Any model with a first order optimization interface.
 * \param[in] init_point  Starting point for the solver.
 * \param[in,out] opts    Solver options.
 * \param[in] solver      Name of solver
 *
*/
void set_default_solver_options(const first_order_opt_interface& model, const
    DenseVector& point,  const std::string solver, std::map<std::string,
    flexible_type>& opts);

/**
 *
 * Compute residual gradient.
 * 
 * \param[in] gradient Dense Gradient
 * \returns Residual to check for termination.
 *
 *
*/
double compute_residual(const DenseVector& gradient); 

/**
 *
 * Compute residual gradient.
 * 
 * \param[in] gradient Dense Gradient
 * \returns Residual to check for termination.
 *
 *
*/
double compute_residual(const SparseVector& gradient); 

/**
 *
 * Check hessian of second_order_optimization_iterface models at a point. 
 *
 * The function lets you check that model.compute_hessian is accurately
 * implemented.
 *
 * Check uses central difference to hessian. The must be with 1e-3
 * relative tolerance. The notion of relative tolerance is tricky especially
 * when gradients are really large or really small.
 * 
 * \param[in] model   Any model with a first order optimization interface.
 * \param[in] point   Point at which to check the gradient.
 * \param[in] hessian Dense hessian matrix.
 * \param[in] mbStart Minibatch start index
 * \param[in] mbSize  Minibatch size
 * \returns bool      True if hessian is correct to 1e-3 tolerance.
 *
 * \note I can't make the model a const because model.compute_function_value()
 * need not be const.
 *
*/
bool check_hessian(second_order_opt_interface& model, const DenseVector& point,
    const DenseMatrix& hessian);

/**
 *
 * Check dense gradient of first_order_optimization_iterface models at a point. 
 *
 * The function lets you check that model.compute_gradient is accurately
 * implemented. 
 *
 * Check uses central difference to compute gradient. The must be with 1e-3
 * relative tolerance. The notion of relative tolerance is tricky especially
 * when gradients are really large or really small.
 * 
 * \param[in] model   Any model with a first order optimization interface.
 * \param[in] point   Point at which to check the gradient.
 * \param[in] grad    Sparse Gradient computed analytically at "point" 
 * \param[in] mbStart Minibatch start index
 * \param[in] mbSize  Minibatch size
 * \returns bool      True if gradient is correct to 1e-3 tolerance.
 *
 * \note I can't make the model a const because model.gradient() need not be
 * const.
 *
*/
bool check_gradient(first_order_opt_interface& model, const DenseVector& point,
    SparseVector& gradient, const size_t mbStart = 0, const size_t mbSize
    = (size_t)(-1));

/**
 *
 * Check sparse gradient of first_order_optimization_iterface models at
 * a point. 
 *
 * The function lets you check that model.compute_gradient is accurately
 * implemented. 
 *
 * Check uses central difference to compute gradient. The must be with 1e-3
 * relative tolerance. The notion of relative tolerance is tricky especially
 * when gradients are really large or really small.
 * 
 * \param[in] model   Any model with a first order optimization interface.
 * \param[in] point   Point at which to check the gradient.
 * \param[in] grad    Dense gradient computed analytically at "point" 
 * \param[in] mbStart Minibatch start index
 * \param[in] mbSize  Minibatch size
 * \returns bool      True if hessian is correct to 1e-3 tolerance.
 *
 *
*/
bool check_gradient(first_order_opt_interface& model, const DenseVector& point,
    const DenseVector& gradient, const size_t mbStart = 0, const size_t mbSize
    = (size_t)(-1));


/** 
 * Translate solver status to a string that a user can understand.
 * 
 * \param[in] status Status of the solver 
 * \returns   String with a meaningful interpretation of the solver status. 
*/
std::string translate_solver_status(const OPTIMIZATION_STATUS& status);

/** 
 * Log solver summary stats (useful for benchmarking 
 *
 * \param[in] status Status of the solver 
 * \returns Clean output of the optimization summary. 
*/
void log_solver_summary_stats(const solver_return& stats, bool simple_mode = false);

/**
 * Performs left = left + right across sparse and dense vectors.
 *
 * \note Although Eigen is an amazing library. This operation is horribly
 * inefficient when Left is a dense vector and Right is a sparse vector.
 *
 * \param[in,out] left  Vector
 * \param[in]     right Rector
*/
template <typename L, typename R>
void vector_add(L & left, const R & right);


/// \}
//
} // Optimization

} // turicreate

#endif 

