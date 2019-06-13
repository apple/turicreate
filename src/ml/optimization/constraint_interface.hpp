/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CONSTRAINT_INTERFACE_H_
#define TURI_CONSTRAINT_INTERFACE_H_

#include <string>
#include <core/data/flexible_type/flexible_type.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// Optimizaiton
#include <ml/optimization/optimization_interface.hpp>

// TODO: List of todo's for this file
//------------------------------------------------------------------------------
//

namespace turi {

namespace optimization {


/**
 * \ingroup group_optimization
 * \addtogroup optimization_constraints Optimization Constraints
 * \{
 */



/**
 *
 * Interface for constraints for Gradient Projection solvers. See chapter 12
 * of (1) for an intro to constrained optimization.
 *
 * Some implementations are based on Section 16.7 of (1).
 *
 *
 * Background: Gradient Projection Methods
 * --------------------------------------------------------------------------
 *
 * Gradient project methods are methods for solving bound constrained
 * optimization problems.
 *
 * Traditionally, in unconstrained optimization, we solve the problem
 *  \min_x f(x) using a gradient descent method as follows:
 *
 *              x_{k+1} = x_{k} - \alpha_k \grad f(x_k)             (G)
 *
 *   where \alpha_k is a step size.
 *
 *  The gradient-projection framework performs solves the problem:
 *  \min_{x \in C} f(x) using a slight modification to the gradient step in (G)
 *
 *          x_{k+1} =  P_C(x_{k} - \alpha_k \grad f(x_k))       (PG)
 *
 *   where P_C is the projection of a point on to the convex set.
 *
 *       P_C(z) = \min_{x \in z} ||x-z||^2
 *
 *   which works out to be the closest point to the set in C.
 *
 * Comparison of gradient projection with other methods
 * -----------------------------------------------------------
 *
 * In solving bound constrained optimization problems, active set methods face
 * criticism because the working set changes slowly; at each iteration, at most
 * one constraint is added to or dropped from the working set. If there are k0
 * constraints active at the initial W0, but kθ constraints active at the
 * solution, then at least |kθ−k0| iterations are required for convergence.
 * This property can be a serious disadvantage in large problems if the working
 * set at the starting point is vastly different from the active set at the
 * solution.
 *
 *
 * The gradient-projection method is guaranteed to identify the active set at
 * a solution in a finite number of iterations.  After it has identified the
 * correct active set, the gradient-projection algorithm reduces to the
 * steepest-descent algorithm on the subspace of free variables.
 *
 * References:
 *
 * (1) Wright S.J  and J. Nocedal. Numerical optimization. Vol. 2.
 *                         New York: Springer, 1999. (Chapter 12)
 *
 *
*/
class constraint_interface {

  public:

  /**
   * Default desctuctor.
   */
  virtual ~constraint_interface() {};


  /**
   * Project a dense point into the constraint space.
   * \param[in,out]  point   Point which we are using to project the point.
   *
   * Given a convex set X, the projection operator is given by
   *
   *     P(y) = \min_{x \in X} || x - y ||^2
   *
   */
  virtual void project(DenseVector &point) const = 0;

  /**
   * Project a dense a block of co-ordinates point into the constraint space.
   *
   * \param[in,out]  point        A block project the point.
   * \param[in]      block_start  Start index of the block
   * \param[in]      block_size   Size of the block
   *
   * Given a convex set X, the projection operator is given by
   *
   *     P(y) = \min_{x \in X} || x - y ||^2
   *
   */
  virtual void project_block(DenseVector &point, const size_t block_start,
      const size_t block_size) const = 0;

  /**
   * Boolean function to determine if a dense point is present in a constraint
   * space.
   * \param[in]  point   Point which we are querying.
   *
   */
  virtual bool is_satisfied(const DenseVector &point) const = 0;

  /**
   * A measure of the first order optimality conditions.
   *
   * \param[in]  point    Point which we are querying.
   * \param[in]  gradient Gradient at that point for a given function
   *
   *  If you don't know what to do, then use
   *                ||P_C(x - grad) - x||
   * where x is the point, grad is the gradient and P_C is the projection to
   * the set in consideration.
   *
   */
  virtual double first_order_optimality_conditions(const DenseVector &point,
                                        const DenseVector& gradient) const = 0;


};

/// \}
} // Optimization
} // Turi

#endif
