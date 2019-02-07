/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_BOOSTED_TREES_H_
#define TURI_BOOSTED_TREES_H_
// unity xgboost
#include <unity/toolkits/supervised_learning/xgboost.hpp>
#include <unity/toolkits/coreml_export/mlmodel_wrapper.hpp>

#include <export.hpp>

namespace turi {
namespace supervised {
namespace xgboost {

/**
 * Boosted trees regression.
 *
 */
class EXPORT boosted_trees_regression: public xgboost_model {  
  
  public:
  
  /**
   * Set one of the options in the algorithm.
   *
   * This values is checked	against the requirements given by the option
   * instance. Options that are not present use default options.
   *
   * \param[in] opts Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _opts) override; 

  bool is_classifier() const override { return false; }

  /** 
   * Configure booster from options 
   */
  void configure(void) override;

  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml() override;


  BEGIN_CLASS_MEMBER_REGISTRATION("boosted_trees_regression");
  IMPORT_BASE_CLASS_REGISTRATION(supervised_learning_model_base);
  END_CLASS_MEMBER_REGISTRATION

};

/**It can also be used to predict the class of
 * Boosted trees classifier.
 *
 */
class EXPORT boosted_trees_classifier : public xgboost_model {  
  
  public:
  
  /**
   * Initialize things that are specific to your model.
   *
   * \param[in] data ML-Data object created by the init function.
   *
   */
  void model_specific_init(const ml_data& data, 
                           const ml_data& valid_data) override;

  /**
   * Set one of the options in the algorithm.
   *
   * This values is checked	against the requirements given by the option
   * instance. Options that are not present use default options.
   *
   * \param[in] opts Options to set
   */
  void init_options(const std::map<std::string, flexible_type>& _opts) override;
 
  bool is_classifier() const override { return true; }

  /** 
   * Configure booster from options 
   */
  void configure(void) override;

  /**
   * Set the default evaluation metric during model evaluation..
   */
  void set_default_evaluation_metric() override {
    set_evaluation_metric({
        "accuracy", 
        "auc", 
        "confusion_matrix",
        "f1_score", 
        "log_loss",
        "precision", 
        "recall",  
        "roc_curve",
        }); 
  }
  
  /**
   * Set the default evaluation metric for progress tracking.
   */
  void set_default_tracking_metric() override {
    set_tracking_metric({
        "accuracy", "log_loss"
       });
  }
 
  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml() override;

  BEGIN_CLASS_MEMBER_REGISTRATION("boosted_trees_classifier");
  IMPORT_BASE_CLASS_REGISTRATION(supervised_learning_model_base);
  END_CLASS_MEMBER_REGISTRATION

};

}  // namespace xgboost
}  // namespace supervised
}  // namespace turi
#endif
