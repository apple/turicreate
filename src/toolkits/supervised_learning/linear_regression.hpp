/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_REGR_LINEAR_REGRESSION_H_
#define TURI_REGR_LINEAR_REGRESSION_H_

// ML-Data Utils
#include <ml/ml_data/metadata.hpp>


// Toolkits
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>

// Optimization Interface
#include <ml/optimization/optimization_interface.hpp>

#include <core/export.hpp>

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
  Eigen::Matrix<double, Eigen::Dynamic,1>  coefs;                 /**< Coefs */
  Eigen::Matrix<double, Eigen::Dynamic,1>  std_err;

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
  void model_specific_init(const ml_data& data, const ml_data& valid_data) override;

  /**
   * Initialize the options.
   *
   * \param[in] _options Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _options) override;

  /**
   * Gets the model version number
   */
  size_t get_version() const override;

  bool is_classifier() const override { return false; }

  /**
   * Train a regression model.
   */
  void train() override;

  /**
   * Setter for model coefficieints.
   */
  void set_coefs(const DenseVector& _coefs) override;

  /**
   * Serialize the object.
   */
  void save_impl(turi::oarchive& oarc) const override;

  /**
   * Load the object
   */
  void load_version(turi::iarchive& iarc, size_t version) override;

  /**
   * Predict for a single example.
   *
   * \param[in] x  Single example.
   * \param[in] output_type Type of prediction.
   *
   * \returns Prediction for a single example.
   *
   */
  flexible_type predict_single_example(
    const DenseVector& x,
    const prediction_type_enum& output_type=prediction_type_enum::NA) override;

  /**
   * Predict for a single example.
   *
   * \param[in] x  Single example.
   * \param[in] output_type Type of prediction.
   *
   * \returns Prediction for a single example.
   *
   */
  flexible_type predict_single_example(
    const SparseVector& x,
    const prediction_type_enum& output_type=prediction_type_enum::NA) override;

  /**
  * Get coefficients for a trained model.
  */
  void get_coefficients(DenseVector& _coefs) const{
    _coefs.resize(coefs.size());
    _coefs = coefs;
  }

  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml() override;

  BEGIN_CLASS_MEMBER_REGISTRATION("regression_linear_regression");
  IMPORT_BASE_CLASS_REGISTRATION(supervised_learning_model_base);
  END_CLASS_MEMBER_REGISTRATION
};

} // supervised
} // turicreate

#endif
