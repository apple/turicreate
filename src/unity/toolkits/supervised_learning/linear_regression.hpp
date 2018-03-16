/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_REGR_LINEAR_REGRESSION_H_
#define TURI_REGR_LINEAR_REGRESSION_H_

// ML-Data Utils
#include <ml_data/metadata.hpp>


// Toolkits
#include <toolkits/supervised_learning/supervised_learning.hpp>

// Optimization Interface
#include <optimization/optimization_interface.hpp>

#include <export.hpp>

namespace turi {
namespace supervised {

class linear_regression_opt_interface;

/*
 * Linear Regression Model
 * ****************************************************************************
 */


/**
 * Linear regression model class definition.
 *
 */
class EXPORT linear_regression: public supervised_learning_model_base {


  protected:

  std::shared_ptr<linear_regression_opt_interface> lr_interface;

  public:
  
  static constexpr size_t LINEAR_REGRESSION_MODEL_VERSION = 4;
  arma::vec  coefs;                 /**< Coefs */
  arma::vec  std_err;

  /**
   * Destructor. Make sure bad things don't happen
   */
  ~linear_regression();


  /**
   * Initialize things that are specific to your model.
   *
   * \param[in] data ML-Data object created by the init function.
   *
   */
  void model_specific_init(const ml_data& data, const ml_data& valid_data);
  
  /**
   * Initialize the options.
   *
   * \param[in] _options Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _options);
  
  /**
   * Gets the model version number
   */
  size_t get_version() const;


  /**
   * Train a regression model.
   */
  void train();
  
  /**
   * Setter for model coefficieints.
   */
  void set_coefs(const DenseVector& _coefs);

  /**
   * Serialize the object.
   */
  void save_impl(turi::oarchive& oarc) const;

  /**
   * Load the object
   */
  void load_version(turi::iarchive& iarc, size_t version);

  /**
   * Predict for a single example. 
   *
   * \param[in] x  Single example.
   * \param[in] output_type Type of prediction.
   *
   * \returns Prediction for a single example.
   *
   */
  flexible_type predict_single_example(const DenseVector& x, 
          const prediction_type_enum& output_type=prediction_type_enum::NA);

  /**
   * Predict for a single example. 
   *
   * \param[in] x  Single example.
   * \param[in] output_type Type of prediction.
   *
   * \returns Prediction for a single example.
   *
   */
  flexible_type predict_single_example(const SparseVector& x, 
          const prediction_type_enum& output_type=prediction_type_enum::NA);

  /**
  * Get coefficients for a trained model.
  */
  void get_coefficients(DenseVector& _coefs) const{
    _coefs.resize(coefs.size());
    _coefs = coefs;
  }

  void export_to_coreml(const std::string& filename);

  SUPERVISED_LEARNING_METHODS_REGISTRATION(
      "regression_linear_regression", linear_regression); 
      
};

} // supervised
} // turicreate

#endif

