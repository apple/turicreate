/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_XGBOOST_H_
#define TURI_XGBOOST_H_
// SFrame
#include <sframe/sarray.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>

// ML-Data
#include <ml_data/ml_data.hpp>

// Utils
#include <timer/timer.hpp>
#include <table_printer/table_printer.hpp>
#include <export.hpp>

// Toolkits
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>
#include <unity/toolkits/coreml_export/mlmodel_wrapper.hpp>

// Forward delcare
namespace xgboost {
namespace learner {
class BoostLearner;
struct DMatrix;
}
}

namespace turi {
namespace supervised {
namespace xgboost {

// forward declare
class DMatrixMLData;

enum class storage_mode_enum : int { IN_MEMORY = 0, EXT_MEMORY = 1, AUTO = 2 };

/**
 * Regression model base class.
 */
class EXPORT xgboost_model : public supervised_learning_model_base {

  /** version number */
  static constexpr size_t XGBOOST_MODEL_VERSION = 9;

  public:

  xgboost_model();


  /**
   * Configure booster from options
   */
  virtual void configure(void) = 0;

  /**
   * Methods of base implementation.
   * -------------------------------------------------------------------------
   */
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
  virtual void init_options(const std::map<std::string,flexible_type>& _opts) override;

  /**
   * Methods already implemented.
   * -------------------------------------------------------------------------
   */
  bool support_missing_value() const override { return true; }

  /**
   * Train a regression model.
   */
  void train(void) override;

  /**
   * Make predictions using a trained regression model.
   *
   * \param[in] test_X Test data (only independent variables)
   * \param[in] output_type Type of prediction
   * only)
   * \returns ret   Shared pointer to an SArray containing predicions.
   *
   * \note Already assumes that data is of the right shape.
   */
  std::shared_ptr<sarray<flexible_type>> predict(
      const ml_data& test_data,
      const std::string& output_type="") override;

  /**
   * Get the predict from the base class and put it here :)
   */
  using supervised_learning_model_base::predict;

  /**
   * Fast path predictions given a row of flexible_types.
   *
   * \param[in] rows List of rows (each row is a flex_dict)
   * \param[in] missing_value_action Missing value action string
   * \param[in] output_type Output type.
   */
  gl_sarray fast_predict(
      const std::vector<flexible_type>& test_data,
      const std::string& missing_value_action = "error",
      const std::string& output_type="") override;

  std::shared_ptr<sarray<flexible_type>> predict_impl(
      const ::xgboost::learner::DMatrix& dmat,
      const std::string& output_type="");

  void xgboost_predict(const ::xgboost::learner::DMatrix& dmat,
                       bool output_margin,
                       std::vector<float>& out_preds,
                       double rf_running_rescale_constant=0.0);

  /**
   * Fast path predictions given a row of flexible_types.
   *
   * \param[in] rows List of rows (each row is a flex_dict)
   * \param[in] missing_value_action Missing value action string
   * \param[in] output_type Output type.
   * \param[in] topk Number of classes to return
   */
  gl_sframe fast_predict_topk(
      const std::vector<flexible_type>& rows,
      const std::string& missing_value_action ="error",
      const std::string& output_type="",
      const size_t topk = 5) override;

  sframe predict_topk_impl(
      const ::xgboost::learner::DMatrix& dmat,
      const std::string& output_type="",
      const size_t topk = 5);

  /**
   * Top-k predictions for multi-class classification.
   *
   * \param[in] test_X Test data (only independent variables)
   * \param[in] output_type Type of prediction
   * only)
   * \returns ret   Shared pointer to an SArray containing predicions.
   *
   * \note Already assumes that data is of the right shape.
   */
  sframe predict_topk(const ml_data& test_data,
                      const std::string& output_type="",
                      const size_t topk = 2) override;
  /**
   * First make predictions and then evaluate.
   *
   * \param[in] test_X Test data.
   * \param[in] evaluation_type Type of evaluation
   *
   * \note Already assumes that data is of the right shape. Test data
   * must contain target column also.
   *
   */
  std::map<std::string, variant_type> evaluate(
               const ml_data& test_data,
               const std::string& evaluation_type="",
               bool with_prediction=false) override;

  std::map<std::string, variant_type> evaluate_impl(
               const DMatrixMLData& dmat,
               const std::string& evaluation_type="");


  /**
   * Extract "tree features" for each test data instance.
   * The tree feature is a integer vector f of size
   * equal to number of trees, and f[i] is the leaf index of the tree.
   *
   * \param[test_data] test_X Test data.
   * \param[options] addtional options.
   */
  std::shared_ptr<sarray<flexible_type>> extract_features(
      const sframe& test_data,
      ml_missing_value_action missing_value_action) override;


  /**
   * Returns an SFrame with two columns: feature names, and feature
   * occurance in all trees.
   */
  gl_sframe get_feature_importance();

  /**
   * Get all the decision trees from XGboost.
   */
  flexible_type get_trees();

  /**
   * Get the decision tree associated with a particular tree_id.
   */
  flexible_type get_tree(size_t tree_id);


  /**
   * Returns a list of string representation of trees.
   */
  std::vector<std::string> dump(bool with_stats);
  std::vector<std::string> dump_json(bool with_stats);

  /**
   * Gets the model version number
   */
  virtual size_t get_version() const override {
    // Version translator
    // -----------------------
    //  0 -  Version 1.0
    //  1 -  Version 1.1
    //  2 -  Version 1.2
    //  3 -  Version 1.4
    //  4 -  Version 1.6
    //  5 -  Version 1.7
    //  6 -  Version 1.8
    //  7 -  Version 1.8.3
    //  8 -  Version 1.9
    return XGBOOST_MODEL_VERSION;
  }

  /**
   * Serialize the object.
   */
  void save_impl(turi::oarchive& oarc) const override;

  /**
   * Load the object
   */
  void load_version(turi::iarchive& iarc, size_t version) override;

  /**
   * Return true if the model is random forest classifier or regression model.
   */
  bool is_random_forest();

  /**
   * Return the number of classes of the model or 0 if the model is not a classifier.
   */
  size_t num_classes();

  /**
   * \internal
   * Set the model to use external memoty for training. Test only, do NOT
   * call directly.
   */
  void _set_storage_mode(storage_mode_enum mode);

  /**
   * \internal
   * Set the model to split the input data to batch_size. Test only, do NOT call directly.
   */
  void _set_num_batches(size_t num_batches);

  /**
   * \interal
   */
  std::pair<std::shared_ptr<DMatrixMLData>, std::shared_ptr<DMatrixMLData>> _init_data();

  void _init_learner(std::shared_ptr<DMatrixMLData> ptrain, std::shared_ptr<DMatrixMLData> pvalid,
      bool restore_from_checkpoint, std::string checkpoint_restore_path);

  table_printer _init_progress_printer(bool has_validation_data);

  size_t _get_early_stopping_rounds(bool has_validation_data);

  void _save_training_state(size_t iteration,
                            const std::vector<float>& training_metrics,
                            const std::vector<float>& validation_metrics,
                            std::shared_ptr<unity_sframe> progress_table,
                            double training_time);

  void _checkpoint(const std::string& path);

  void _restore_from_checkpoint(const std::string& path);

  void _save(oarchive& oarc, bool save_booster_prediction_buffer) const;

protected:

  /*! \brief internal ml data structure used for training*/
  ml_data ml_data_, validation_ml_data_;

  /*! \brief this is the xgboost object supporting things */
  std::shared_ptr<::xgboost::learner::BoostLearner> booster_;

  storage_mode_enum storage_mode_ = storage_mode_enum::AUTO;

  size_t num_batches_ = 0;
  
  std::shared_ptr<coreml::MLModelWrapper> _export_xgboost_model(bool is_classifier,
      bool is_random_forest,
      const std::map<std::string, flexible_type>& context);

};

}  // namespace xgboost
}  // namespace supervised
}  // namespace turi
#endif
