/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CLASS_LINEAR_SVM_H_
#define TURI_CLASS_LINEAR_SVM_H_

// ML-Data Utils
#include <ml_data/ml_data.hpp>
#include <unity/lib/gl_sframe.hpp>

// Toolkits
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>
#include <unity/toolkits/coreml_export/mlmodel_wrapper.hpp>

// Optimization Interface
#include <optimization/optimization_interface.hpp>

#include <export.hpp>

namespace turi {
namespace supervised {

class linear_svm_scaled_logistic_opt_interface;

/*
 * SVM  Model
 * ****************************************************************************
 */

/**
 * SVM svm model class definition.
 *
 */
class EXPORT linear_svm: public supervised_learning_model_base {

  protected:

  Eigen::Matrix<double, Eigen::Dynamic,1>  coefs;    /**< Primal sol */
  std::shared_ptr<linear_svm_scaled_logistic_opt_interface>
                                          scaled_logistic_svm_interface;
  
  public:

  static constexpr size_t SVM_MODEL_VERSION = 5;
  /**
   * Destructor. Make sure bad things don't happen
   */
  virtual ~linear_svm();
  
  /**
   * Set the default evaluation metric during model evaluation..
   */
  void set_default_evaluation_metric() override {
    set_evaluation_metric({
        "accuracy", 
        "confusion_matrix",
        "f1_score", 
        "precision", 
        "recall",  
        }); 
  }
  
  /**
   * Set the default evaluation metric for progress tracking.
   */
  void set_default_tracking_metric() override {
    set_tracking_metric({
        "accuracy", 
       }); 
  }
  
  /**
   * Internal init after the ml_data is built.
   *
   * \param[in] data        Training data
   * \param[in] valid_data  Validation data
   *
   */
  void model_specific_init(const ml_data& data, 
                           const ml_data& valid_data) override;

  bool is_classifier() const override { return true; }

  /**
   * Train a svm model.
   */
  void train() override;

  /**
   * Init the options.
   *
   * \param[in] opts Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _opts) override;


  /**
   * Gets the model version number
   */
  size_t get_version() const override;

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
   * Make classification using a trained supervised_learning model.
   *
   * \param[in] X           Test data (only independent variables)
   * \param[in] output_type Type of classifcation (future proof).
   * \returns ret   SFrame with "class" and probability (if applicable)
   *
   * \note Already assumes that data is of the right shape.
   */
  sframe classify(const ml_data& test_data, 
                  const std::string& output_type="") override;

  /**
   * Fast path predictions given a row of flexible_types
   *
   * \param[in] rows List of rows (each row is a flex_dict)
   * \param[in] output_type Output type. 
   */
  gl_sframe fast_classify(
      const std::vector<flexible_type>& rows,
      const std::string& missing_value_action ="error") override;

  /**
  * Get coefficients for a trained model.
  */
  void get_coefficients(DenseVector& _coefs) const{
    _coefs.resize(coefs.size());
    _coefs = coefs;
  }
  
  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml() override;

  BEGIN_CLASS_MEMBER_REGISTRATION("classifier_svm");
  IMPORT_BASE_CLASS_REGISTRATION(supervised_learning_model_base);
  END_CLASS_MEMBER_REGISTRATION
 
};

} // supervised
} // turicreate

#endif

