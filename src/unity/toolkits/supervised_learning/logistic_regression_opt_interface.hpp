/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_REGR_LOGISTIC_REGRESSION_OPT_INTERFACE_H_
#define TURI_REGR_LOGISTIC_REGRESSION_OPT_INTERFACE_H_

// ML-Data Utils
#include <ml_data/ml_data.hpp>

// Toolkits
#include <toolkits/supervised_learning/standardization-inl.hpp>
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/logistic_regression.hpp>

// Optimization Interface
#include <optimization/optimization_interface.hpp>


namespace turi {
namespace supervised {

/*
 * Logistic Regression Solver
 * *****************************************************************************
 *
 */


 /**
 * Solver interface for logistic regression.
 *
 * Let J denote the number of classes, K the number of features, and N the 
 * number of examples.
 *
 * coefs = [coef_1 ... coef_{J-1}] := (K * (J-1)) x 1 column vector
 * where each 
 * coef_j for j = 1 .. J-1 is a K x 1 column vector representing coefficients
 * for the class j.
 *
 */
class logistic_regression_opt_interface: public
  optimization::second_order_opt_interface {

  protected:

  ml_data data;
  ml_data valid_data;
  logistic_regression smodel;

  // number of examples, features, and total variables
  size_t examples = 0;
  size_t classes = 2;
  size_t features = 0;
  size_t variables = 0;
  size_t n_threads;

  std::map<int, float> class_weights = {{0,1.0}, {1, 1.0}};

  std::shared_ptr<l2_rescaling> scaler;        /** <Scale features */
  bool feature_rescaling = false;              /** Feature rescaling */
  bool is_dense = false;                       /** Is the data dense? */

  public:

  /**
  * Default constructor
  *
  * \param[in] _data        ML Data containing everything
  *
  * \note Default options are used when the interface is called from the
  * logistic regression class.
  */
  logistic_regression_opt_interface(const ml_data& _data, 
                                    const ml_data& _valid_data, 
                                    logistic_regression& _model);

  /**
  * Default destructor
  */
  ~logistic_regression_opt_interface();

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
  * Set the number of threads
  *
  * \param[in] _n_threads Number of threads
  */
  void set_threads(size_t _n_threads);
  
  /**
  * Set the class weights (as a flex_dict which is already validated)
  *
  * \param[in] class_weights Validated flex_dict
  *            Key   : Index of the class in the target_metadata
  *            Value : Weights on the class
  */
  void set_class_weights(const flexible_type& class_weights);

  /**
  * Get the number of examples for the model
  *
  * \returns Number of examples
  */
  size_t num_examples() const;

  /**
  * Get the number of validation-set examples for the model
  *
  * \returns Number of examples
  */
  size_t num_validation_examples() const;

  /**
  * Get the number of variables in the model
  *
  * \returns Number of variables
  */
  size_t num_variables() const;
  
  /**
  * Get the number of classes in the model
  *
  * \returns Number of classes
  */
  size_t num_classes() const;


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

  double get_validation_accuracy();
  double get_training_accuracy();

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
   * Compute second order statistics at the given point with respect to the
   * validation data. (Gradient & Function value)
   *
   * \param[in]  point           Point at which we are computing the stats.
   * \param[out] hessian         Hessian (Dense)
   * \param[out] gradient        Dense gradient
   * \param[out] function_value  Function value
   *
   */
  void compute_validation_second_order_statistics(
      const DenseVector& point, DenseMatrix& hessian, DenseVector& gradient,
      double &function_value);

  private:

  void compute_second_order_statistics(
      const ml_data& data,const DenseVector& point, DenseMatrix& hessian,
      DenseVector& gradient, double &function_value);
};


} // supervised
} // turicreate

#endif

