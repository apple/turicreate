/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_REGULARIZER_H_
#define TURI_REGULARIZER_H_

#include <string>
#include <flexible_type/flexible_type.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// Optimizaiton
#include <optimization/optimization_interface.hpp>
#include <optimization/regularizer_interface.hpp>

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
 * Interface for the regularizer (Scaled L2-norm) 
 *
 *      f(x) = \sum_{i} lambda_i * x_i^2
 *
 */
class l2_norm : public smooth_regularizer_interface {

  protected:

    DenseVector lambda;                     /**< Penalty on the regularizer */
    size_t variables;                       /**< # Variables in the problem */

  public:
  
  
  /**
   * Default constructor. 
   */
  l2_norm(const DenseVector& _lambda){
    lambda= _lambda;
    variables = _lambda.size();
  }
  
  /**
   * Default desctuctor. Do nothing.
   */
  ~l2_norm(){
  }
  
  /**
   * Compute the hessian of the regularizer at a given point.
   * \param[in]      point   Point at which we are computing the gradient.
   * \param[in,out]  hessian Diagonal matrix as the hessian gradient.
   *
   */
  inline void compute_hessian(const DenseVector &point, DiagonalMatrix
      &hessian) const {
    hessian = 2 * lambda.asDiagonal();
  }
  
  /**
   * Compute the function value of the regularizer at a given point.
   * \param[in]  point   Point at which we are computing the gradient.
   *
   */
  inline double compute_function_value(const DenseVector &point) const{
    DASSERT_EQ(variables, point.size());
    return lambda.dot(point.cwiseAbs2());
  }

  
  /**
   * Compute the gradient (or subgradient) at the given point.
   *
   * \param[in]  point    Point at which we are computing the gradient.
   * \param[out] gradient Dense gradient
   * 
   */
  inline void compute_gradient(const DenseVector &point, DenseVector& gradient)
    const{
    DASSERT_EQ(variables, point.size());
    gradient = 2 * lambda.cwiseProduct(point);
  }
  
  /**
   * Compute the proximal operator for the l2-regularizer 
   *
   * \param[in,out]  point      Point at which we are computing the gradient.
   * \param[in]      penalty    Penalty
   *
   * \note The proximal operator for lambda * ||x||^2 at the point v is
   * given by 
   *                  v/(1 + 2*lambda*penalty)
   *  
   */
  inline void apply_proximal_operator(DenseVector &point, const double&
      _penalty=0)const{
    DASSERT_EQ(variables, point.size());
    for(size_t i = 0; i < variables; i++)
      point[i] = point[i] / (1 + 2*_penalty*lambda[i]);
  }
  

};


/**
 * Interface for the regularizer (Scaled L1-norm) 
 *
 *      f(x) = \sum_{i} lambda_i * |x_i|
 *
 */
class l1_norm : public regularizer_interface {

  protected:

    DenseVector lambda;                     /**< Penalty on the regularizer */
    size_t variables;                       /**< # Variables in the problem */

  public:
  
  /**
   * Default constructor. 
   */
  l1_norm(const DenseVector& _lambda){
    lambda= _lambda;
    variables = _lambda.size();
  }
  
  /**
   * Default desctuctor. Do nothing.
   */
  ~l1_norm(){
  }
  
  /**
   * Compute the function value of the regularizer at a given point.
   * \param[in]  point   Point at which we are computing the gradient.
   *
   */
  inline double compute_function_value(const DenseVector &point) const{
    DASSERT_EQ(variables, point.size());
    return lambda.dot(point.cwiseAbs());
  }

  
  /**
   * Compute the subgradient at the given point.
   *
   * \param[in]  point    Point at which we are computing the gradient.
   * \param[out] gradient Dense sub-gradient
   * 
   */
  inline void compute_gradient(const DenseVector &point, DenseVector& gradient) const{
    DASSERT_EQ(variables, point.size());
    gradient.setZero();
    for(size_t i = 0; i < variables; i++)
      if (gradient[i] > OPTIMIZATION_ZERO)
        gradient[i] = lambda[i];
      else if (gradient[i] < - OPTIMIZATION_ZERO)
        gradient[i] = - lambda[i];
  }
  
  /**
   * Compute the proximal operator for the l2-regularizer 
   *
   * \param[in,out]  point      Point at which we are computing the gradient.
   * \param[in]      penalty    Penalty
   *
   * \note The proximal operator for lambda * ||x||_1 at the point v is
   * given by 
   *        soft(x, lambda) = (x - lambda)_+ - (-x - lambda)_+ 
   *  
   */
  inline void apply_proximal_operator(DenseVector &point, const double&
      _penalty=0)const{
    DASSERT_EQ(variables, point.size());
    for(size_t i = 0; i < variables; i++)
      point[i] = std::max(point[i] - _penalty*lambda[i], 0.0) - 
                            std::max(-point[i] - _penalty*lambda[i], 0.0);
  }
  

};


/**
 * Interface for the elastic net regularizer (Scaled L1-norm) 
 *
 *      f(x) = \sum_{i} alpha_i * |x_i| + \sum_{i} beta_i * x_i^2
 *
 */
class elastic_net : public regularizer_interface {

  protected:

    DenseVector alpha;                     /**< Penalty on the l1-regularizer */
    DenseVector beta;                      /**< Penalty on the l2-regularizer */
    size_t variables;                      /**< # Variables in the problem */

  public:
  
  
  /**
   * Default constructor. 
   */
  elastic_net(const DenseVector& _alpha, const DenseVector& _beta){
    DASSERT_EQ(_alpha.size(), _beta.size());
    alpha = _alpha;
    beta = _beta;
    variables = _alpha.size();
  }
  
  /**
   * Default desctuctor. Do nothing.
   */
  ~elastic_net(){
  }
  
  /**
   * Compute the function value of the regularizer at a given point.
   * \param[in]  point   Point at which we are computing the gradient.
   *
   */
  inline double compute_function_value(const DenseVector &point) const{
    DASSERT_EQ(variables, point.size());
    return alpha.dot(point.cwiseAbs()) + beta.dot(point.cwiseAbs2());
  }

  
  /**
   * Compute the subgradient at the given point.
   *
   * \param[in]  point    Point at which we are computing the gradient.
   * \param[out] gradient Dense sub-gradient
   * 
   */
  inline void compute_gradient(const DenseVector &point, DenseVector& gradient) const{
    DASSERT_EQ(variables, point.size());
    gradient = 2 * beta.cwiseProduct(point);
    for(size_t i = 0; i < variables; i++)
      if (gradient[i] > OPTIMIZATION_ZERO)
        gradient[i] += alpha[i];
      else if (gradient[i] < - OPTIMIZATION_ZERO)
        gradient[i] += -alpha[i];
  }
  
  /**
   * Compute the proximal operator for the elastic-regularizer 
   *
   * \param[in,out]  point      Point at which we are computing the gradient.
   * \param[in]      penalty    Penalty
   *
   * \note The proximal operator for alpha||x||_1 + beta||x||_2^2 at 
   *        y = soft(x, alpha) = (x - alpha)_+ - (-x - alpha)_+ 
   *        x = y / (1 + 2 beta) 
   *
   * \note Do not swap the order.
   *  
   */
  inline void apply_proximal_operator(DenseVector &point, const double&
      _penalty=0)const{
    DASSERT_EQ(variables, point.size());
    for(size_t i = 0; i < variables; i++){
      point[i] = std::max(point[i] - _penalty*alpha[i], 0.0) - 
                            std::max(-point[i] - _penalty*alpha[i], 0.0);
      point[i] = point[i] / (1 + 2*_penalty*beta[i]);
    }
  }
  

};

/// \}

} // optimization
} // turicreate

#endif 

