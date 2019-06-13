/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CONSTRAINTS_H_
#define TURI_CONSTRAINTS_H_

#include <string>
#include <core/data/flexible_type/flexible_type.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// Optimizaiton
#include <ml/optimization/utils.hpp>
#include <ml/optimization/optimization_interface.hpp>
#include <ml/optimization/constraint_interface.hpp>

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
 * Interface for non-negative constriants.
 *   x >= 0
 */
class non_negative_orthant : public constraint_interface {

  protected:

    size_t variables;                       /**< # Variables in the problem */

  public:


  /**
   * Default constructor.
   */
  non_negative_orthant(const size_t& _variables){
    variables = _variables;
  }

  /**
   * Default desctuctor. Do nothing.
   */
  ~non_negative_orthant(){
  }


  /**
   * Project a dense point into the constraint space.
   * \param[in,out]  point   Point (Dense Vector)
   *
   * Given a convex set X, the projection operator is given by
   *     P(y) = \std::max(x, o)
   *
   */
  inline void project(DenseVector &point) const {
    DASSERT_EQ(variables, point.size());
    point = point.cwiseMax(0);
  }

  /**
   * Project a block of a dense point into the constraint space.
   *
   * \param[in,out]  point        A block project the point.
   * \param[in]      block_start  Start index of the block
   * \param[in]      block_size   Size of the block
   *
   * Given a convex set X, the projection operator is given by
   *     P(y) = \std::max(x, o)
   *
   */
  inline void project_block(DenseVector &point, const size_t block_start, const
      size_t block_size) const {
    DASSERT_LE(variables, block_start + block_size);
    DASSERT_EQ(block_size, point.size());
    point = point.cwiseMax(0);
  }

  /**
   * Boolean function to deterstd::mine if a dense point is present in a constraint
   * space.
   * \param[in]  point   Point which we are querying.
   *
   */
  inline bool is_satisfied(const DenseVector &point) const {
    DASSERT_EQ(variables, point.size());
    for(size_t i=0; i < size_t(point.size()); i++){
      if( point(i) <= -OPTIMIZATION_ZERO)
        return false;
    }
    return true;
  }


  /**
   * A measure of the first order optimality conditions.
   *
   * \param[in]  point    Point which we are querying.
   * \param[in]  gradient Gradient at that point for a given function
   *
   * Use the Cauchy point as a measure of optimality. See Pg 486 of
   * Nocedal and Wright (Edition 2)
   *
   */
  inline double first_order_optimality_conditions(const DenseVector &point,
                                        const DenseVector& gradient) const{
    DASSERT_TRUE(is_satisfied(point));
    DenseVector proj_gradient = gradient;
    for(size_t i=0; i < size_t(point.size()); i++){
      if(point(i) <= OPTIMIZATION_ZERO){
        proj_gradient(i) = std::min(0.0, gradient(i));
      } else {
        proj_gradient(i) = gradient(i);
      }
    }
    return compute_residual(proj_gradient);
  }
};



/**
 * Interface for box-constraints on variables.
 *   lb <= x <= ub
 */
class box_constraints: public constraint_interface {

  protected:

    DenseVector lb;                  /**< # Upper bound */
    DenseVector ub;                  /**< # Lower bound */
    size_t variables;                /**< # Variables in the problem */

  public:


  /**
   * Default constructor.
   * \param[in]  _variables Number of variables
   * \param[in]  _lb Lower bound
   * \param[in]  _ub Upper bound
   */
  box_constraints(const double& _lb, const double& _ub, const size_t&
      _variables){
    variables = _variables;
    lb.resize(variables);
    lb.setConstant(_lb);
    ub.resize(variables);
    ub.setConstant(_ub);
  }

  /**
   * Default constructor.
   * \param[in]  _variables Number of variables
   * \param[in]  _lb Lower bound
   * \param[in]  _ub Upper bound
   */
  box_constraints(const DenseVector& _lb, const
      DenseVector& _ub){
    variables = _lb.size();
    DASSERT_EQ(variables, _ub.size());
    lb = _lb;
    ub = _ub;
  }

  /**
   * Default desctuctor. Do nothing.
   */
  ~box_constraints(){
  }


  /**
   * Project a dense point into the constraint space.
   * \param[in,out]  point   Point (Dense Vector)
   *
   * Given a convex set X, the projection operator is given by
   *     P(y) = \std::max(x, o)
   *
   */
  inline void project(DenseVector &point) const {
    DASSERT_EQ(variables, point.size());
    for(size_t i=0; i < size_t(point.size()); i++){
      point(i) = std::min(std::max(point(i), lb(i)), ub(i));
    }
  }

  /**
   * Project a block of a dense point into the constraint space.
   *
   * \param[in,out]  point        A block project the point.
   * \param[in]      block_start  Start index of the block
   * \param[in]      block_size   Size of the block
   *
   * Given a convex set X, the projection operator is given by
   *     P(y) = \std::max(x, o)
   *
   */
  inline void project_block(DenseVector &point, const size_t block_start, const
      size_t block_size) const {
    DASSERT_GE(variables, block_start + block_size);
    DASSERT_EQ(block_size, point.size());
    for(size_t i=0; i < block_size; i++){
      point(i) = std::min(std::max(point(i),
                                  lb(block_start + i)), ub(block_start + i));
    }
  }

  /**
   * Boolean function to determine if a dense point is present in a constraint
   * space.
   * \param[in]  point   Point which we are querying.
   *
   */
  inline bool is_satisfied(const DenseVector &point) const {
    DASSERT_EQ(variables, point.size());
    for(size_t i=0; i < size_t(point.size()); i++){
      if( point(i) <= lb(i) - OPTIMIZATION_ZERO ||
          point(i) >= ub(i) + OPTIMIZATION_ZERO)
        return false;
    }
    return true;
  }


  /**
   * A measure of the first order optimality conditions.
   *
   * \param[in]  point    Point which we are querying.
   * \param[in]  gradient Gradient at that point for a given function
   *
   * Use the Cauchy point as a measure of optimality. See Pg 486 of
   * Nocedal and Wright (Edition 2)
   *
   */
  inline double first_order_optimality_conditions(const DenseVector &point,
                                        const DenseVector& gradient) const{
    DASSERT_TRUE(is_satisfied(point));
    DenseVector proj_gradient = gradient;
    for(size_t i=0; i < size_t(point.size()); i++){
      if(point(i) <= OPTIMIZATION_ZERO + lb(i)){
        proj_gradient(i) = std::min(0.0, gradient(i));
      } else if (point(i) >= ub(i) - OPTIMIZATION_ZERO){
        proj_gradient(i) = std::max(0.0, gradient(i));
      } else {
        proj_gradient(i) = gradient(i);
      }
    }
    return compute_residual(proj_gradient);
  }


};


} // optimization
} // turicreate

#endif
