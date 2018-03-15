/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CLASS_LINEAR_SVM_OPT_INTERFACE_H_
#define TURI_CLASS_LINEAR_SVM_OPT_INTERFACE_H_

// ML-Data Utils
#include <ml_data/ml_data.hpp>

// Toolkits
#include <toolkits/supervised_learning/standardization-inl.hpp>
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>

// Optimization Interface
#include <optimization/optimization_interface.hpp>


namespace turi {
namespace supervised {


/*
 * SVM  Solver
 * *****************************************************************************
 *
 */

 /**
  * 
  * Scaled Logistic Loss function
  * --------------------------------
  *
  * SVM is trained using LBFGS on the Modifified logistic function described in
  * [1]. It is much simpler to optimize and very close to the hinge loss. 
  *
  * References:
  *
  * [1] Modified Logistic Regression: An Approximation to SVM and Its 
  * Applications in Large-Scale Text Categorization - Zhang et al ICML 2003
  *
  *
  */
class linear_svm_scaled_logistic_opt_interface: public
  optimization::first_order_opt_interface{

  protected:

  ml_data data;
  ml_data valid_data;
  linear_svm smodel;


  size_t features;                             /**< Num features */
  size_t examples;                             /**< Num examples */
  size_t primal_variables;                     /**< Primal variables */
  size_t classes = 2;                          /**< Number of classes */
  
  std::map<int, float> class_weights = {{0,1.0}, {1, 1.0}};

  size_t n_threads;
  std::shared_ptr<l2_rescaling> scaler;       /** <Scale features */
  bool feature_rescaling = false;             /** Feature rescaling */
  double gamma = 30;
  bool is_dense = false;                      /** Is the data dense? */

  public:

  /**
  * Default constructor
  *
  * \param[in] _data        ML Data containing everything
  *
  */
  linear_svm_scaled_logistic_opt_interface(const ml_data& _data, 
                                           const ml_data& _valid_data, 
                                           linear_svm& model);


  /**
  * Set the scale for the scaled logistic loss.
  * \param[in] _gamma   Set the Gamma
  *
  */
  void set_gamma(const double _gamma);

  /**
  * Default destructor
  */
  ~linear_svm_scaled_logistic_opt_interface();

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
  * Set the class weights (as a flex_dict which is already validated)
  *
  * \param[in] class_weights Validated flex_dict
  *            Key   : Index of the class in the target_metadata
  *            Value : Weights on the class
  */
  void set_class_weights(const flexible_type& class_weights);

  /**
  * Get the number of classes in the model
  *
  * \returns Number of classes
  */
  size_t num_classes() const;

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
  

};



} // supervised
} // turicreate

#endif

