/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_REGULARIZER_INTERFACE_H_
#define TURI_REGULARIZER_INTERFACE_H_

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
 * \addtogroup regularizers Regularizers
 * \{
 */



/**
 *
 * Interface for regularizers which are separable but not smooth.
 * \note Smooth refers to functions with continuous derivatives of all orders.
 *
*/
class regularizer_interface {

  public:

  /**
   * Default desctuctor.
   */
  virtual ~regularizer_interface() {};


  /**
   * Function to determine if the regularizer is smooth.
   * \returns false
   */
  bool is_smooth() {return false;};

  /**
   * Compute the function value of the regularizer at a given point.
   * \param[in]  point   Point at which we are computing the gradient.
   *
   */
  virtual double compute_function_value(const DenseVector &point) const = 0;


  /**
   * Compute the gradient (or subgradient) at the given point.
   *
   * \param[in]  point    Point at which we are computing the gradient.
   * \param[out] gradient Dense gradient
   *
   */
  virtual void compute_gradient(const DenseVector &point, DenseVector&
      gradient) const = 0;

  /**
   * Compute the proximal operator for the regularizer at a given point.
   *
   * \param[in,out]  point    Point at which we are computing the gradient.
   * \param[in]      penalty  Penalty parameters.
   *
   * \note The proximal operator for a convex function f(.) at the point x is
   * defined as
   *                prox_f(x) = argmin_v ( f(v) + 0.5 * penalty ||x-v||^2)
   *  The idea is an old concept in optimization but it well explained in (1).
   *
   * References:
   *
   * (1) Parikh, Neal, and Stephen Boyd. "Foundations and Trends in
   * Optimization." (2014).
   *
   */
  virtual void apply_proximal_operator(DenseVector &point, const double&
      _penalty=0) const = 0;


};


/**
 *
 * Interface for regularizers that separable and smooth.
 *
*/
class smooth_regularizer_interface: public regularizer_interface {

  public:

  /**
   * Function to determine if the regularizer is smooth.
   * \returns true
   */
  bool is_smooth() {return true;};

  /**
   * Default desctuctor.
   */
  virtual ~smooth_regularizer_interface() {};

  /**
   * Compute the hessian of the regularizer at a given point.
   * \param[in]      point   Point at which we are computing the gradient.
   * \param[in,out]  hessian Diagonal matrix as the hessian gradient.
   *
   */
  virtual void compute_hessian(const DenseVector &point, DiagonalMatrix
      &hessian) const = 0;

};


/// \}

} // optimization
} // turicreate

#endif
