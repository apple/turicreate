/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_REGR_LINEAR_REGRESSION_OPT_INTERFACE_H_
#define TURI_REGR_LINEAR_REGRESSION_OPT_INTERFACE_H_

// ML-Data Utils
#include <ml_data/ml_data.hpp>


// Toolkits
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>
#include <unity/toolkits/supervised_learning/standardization-inl.hpp>
#include <unity/toolkits/supervised_learning/linear_regression.hpp>

// Optimization Interface
#include <optimization/optimization_interface.hpp>


// TODO: List of todo's for this file
//------------------------------------------------------------------------------

namespace turi {
namespace supervised {

/*
 * Linear Regression Solver
 * ****************************************************************************
 */

/**
 * Solver interface for the linear regression problem.
 *
 */
class linear_regression_opt_interface: public
                                       optimization::second_order_opt_interface
{
  protected:

  ml_data data;
  ml_data valid_data;
  linear_regression smodel;

  size_t examples = 0;          /**< Number of examples */
  size_t features = 0;          /**< Number of features */
  size_t variables = 0;         /**< Number of variables */
  size_t n_threads;             /** < Num threads */

  std::shared_ptr<l2_rescaling> scaler;        /** <Scale features */
  bool feature_rescaling = false;              /** Feature rescaling */
  bool is_dense = false;                       /** Is the data sparse */

  public:


  /**
   * Default constructor.
   *
   * \param[in] _ml_data           ML Data containing everything!
   * \param[in] feature_rescaling  Feature Rescaling
   *
   * \note Default options are to be used when the interface is called from the
   * linear_regression class.
   */
  linear_regression_opt_interface(const ml_data& _data, 
                                  const ml_data& _valid_data, 
                                  linear_regression& _model, 
                                  bool _feature_rescaling=true);

  /**
   * Default destructor.
   */
  ~linear_regression_opt_interface();

  /**
   * Set the number of threads.
   *
   * \param[in] _n_threads Set the number of threads.
   */
  void set_threads(size_t _n_threads);
  
  /**
   * Set feature scaling
   */
  void init_feature_rescaling();
  
  /**
   * Transform the final solution back to the original scale.
   *
   * \param[in,out] coefs Solution vector
   */
  void rescale_solution(DenseVector& coefs);

  /**
   * Get the number of examples in the model
   *
   * \returns Number of examples
   */
  size_t num_examples() const;

  /**
   * Get the number of variables in the model
   *
   * \returns Number of variables
   */
  size_t num_variables() const;

  /**
   * Get the number of validation-set examples in the model
   *
   * \returns Number of examples
   */
  size_t num_validation_examples() const;

  /**
   * Get strings needed to print the header for the progress table.
   *
   * \param[in] a vector of strings to print at the beginning of the header.
   */
  std::vector<std::pair<std::string, size_t>> 
      get_status_header(const std::vector<std::string>& stat_names);

  /**
   * Get strings needed to print a row of the progress table.
   *
   * \param[in] a vector of model coefficients.
   * \param[in] a vector of stats to print at the beginning of each row
   */
  std::vector<std::string> get_status(const DenseVector& coefs, 
                                      const std::vector<std::string>& stats);

  /**
   * Compute first order statistics at the given point. (Gradient & Function value)
   *
   * \param[in]  point           Point at which we are computing the stats.
   * \param[out] gradient        Dense gradient
   * \param[out] function_value  Function value
   * \param[in]  mbStart         Minibatch start index
   * \param[in]  mbSize          Minibatch size (-1 implies all)
   *
   */
  void compute_first_order_statistics(const DenseVector &point, DenseVector&
      gradient, double & function_value, const size_t mbStart = 0, const size_t
      mbSize = -1);

  /**
   * Compute second order statistics at the given point. (Gradient & Function value)
   *
   * \param[in]  point           Point at which we are computing the stats.
   * \param[out] hessian         Hessian (Dense)
   * \param[out] gradient        Dense gradient
   * \param[out] function_value  Function value
   *
   */
  void compute_second_order_statistics(const DenseVector &point, DenseMatrix&
      hessian, DenseVector& gradient, double & function_value);

  /**
   * Compute first order statistics at the given point with respect to the
   * validation data. (Gradient & Function value)
   *
   * \param[in]  point           Point at which we are computing the stats.
   * \param[out] gradient        Dense gradient
   * \param[out] function_value  Function value
   *
   */
  void compute_validation_first_order_statistics(
      const DenseVector& point, DenseVector& gradient, double &function_value);

  private:

  void compute_first_order_statistics(const ml_data& data, const DenseVector
      &point, DenseVector& gradient, double & function_value, const size_t
      mbStart = 0, const size_t mbSize = -1);
};



} // supervised
} // turicreate

#endif

