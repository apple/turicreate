/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ACTIVITY_CLASSIFIER_H_
#define TURI_ACTIVITY_CLASSIFIER_H_

#include <table_printer/table_printer.hpp>
#include <unity/lib/extensions/ml_model.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/toolkits/activity_classification/ac_data_iterator.hpp>
#include <unity/toolkits/coreml_export/mlmodel_wrapper.hpp>
#include <unity/toolkits/neural_net/compute_context.hpp>
#include <unity/toolkits/neural_net/model_backend.hpp>
#include <unity/toolkits/neural_net/model_spec.hpp>

namespace turi {
namespace activity_classification {

class EXPORT activity_classifier: public ml_model_base {

 public:

  // ml_model_base interface

  void init_options(const std::map<std::string, flexible_type>& opts) override;

  // Interface exposed via Unity server

  void train(gl_sframe data, std::string target_column_name,
             std::string session_id_column_name,
             variant_type validation_data,
             std::map<std::string, flexible_type> opts);
  gl_sarray predict(gl_sframe data, std::string output_type);
  gl_sframe predict_per_window(gl_sframe data, std::string output_type);
  variant_map_type evaluate(gl_sframe data, std::string metric);
  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml(
      std::string filename);

  // TODO: Remainder of public interface: export, evaluate, predict, save/load

  BEGIN_CLASS_MEMBER_REGISTRATION("activity_classifier")

  IMPORT_BASE_CLASS_REGISTRATION(ml_model_base);

  REGISTER_CLASS_MEMBER_FUNCTION(activity_classifier::train, "data", "target",
                                 "session_id", "validation_data", "options");
  register_defaults("train",
                    {{"validation_data", to_variant(std::string("auto"))},
                     {"options",
                      to_variant(std::map<std::string, flexible_type>())}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      activity_classifier::train,
      "----------\n"
      "data : SFrame\n"
      "    Input data which consists of `sessions` of data where each session is\n"
      "    a sequence of data. The data must be in `stacked` format, grouped by\n"
      "    session. Within each session, the data is assumed to be sorted\n"
      "    temporally. Columns in `features` will be used to train a model that\n"
      "    will make a prediction using labels in the `target` column.\n"
      "target : string\n"
      "    Name of the column containing the target variable. The values in this\n"
      "    column must be of string or integer type.\n"
      "session_id : string\n"
      "    Name of the column that contains a unique ID for each session.\n"
      "validatation_data : SFrame or string\n"
      "    A dataset for monitoring the model's generalization performance to\n"
      "    prevent the model from overfitting to the training data.\n"
      "\n"
      "    For each row of the progress table, accuracy is measured over the\n"
      "    provided training dataset and the `validation_data`. The format of\n"
      "    this SFrame must be the same as the training set.\n"
      "\n"
      "    When set to 'auto', a validation set is automatically sampled from the\n"
      "    training data (if the training data has > 100 sessions).\n"
      "options : dict\n"
      "\n"
      "Options\n"
      "-------\n"
      "features : list[string]\n"
      "    Name of the columns containing the input features that will be used\n"
      "    for classification. If not set, all columns except `session_id` and\n"
      "    `target` will be used.\n"
      "prediction_window : int\n"
      "    Number of time units between predictions. For example, if your input\n"
      "    data is sampled at 100Hz, and the `prediction_window` is set to 100\n"
      "    (the default), then this model will make a prediction every 1 second.\n"
      "max_iterations : int\n"
      "    Maximum number of iterations/epochs made over the data during the\n"
      "    training phase. The default is 10 iterations.\n"
      "batch_size : int\n"
      "    Number of sequence chunks used per training step. Must be greater than\n"
      "    the number of GPUs in use. The default is 32.\n"
  );

  REGISTER_CLASS_MEMBER_FUNCTION(activity_classifier::predict, "data",
                                 "output_type");
  register_defaults("predict", {{"output_type", std::string("")}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      activity_classifier::predict,
      "----------\n"
      "data : SFrame\n"
      "    Dataset of new observations. Must include columns with the same\n"
      "    names as the features used for model training, but does not require\n"
      "    a target column. Additional columns are ignored.\n"
      "output_type : {'class', 'probability_vector'}, optional\n"
      "    Form of each prediction which is one of:\n"
      "    - 'probability_vector': Prediction probability associated with each\n"
      "      class as a vector. The probability of the first class (sorted\n"
      "      alphanumerically by name of the class in the training set) is in\n"
      "      position 0 of the vector, the second in position 1 and so on.\n"
      "    - 'class': Class prediction. This returns the class with maximum\n"
      "      probability.\n"
  );

  REGISTER_CLASS_MEMBER_FUNCTION(activity_classifier::predict_per_window,
                                 "data", "output_type");
  register_defaults("predict_per_window", {{"output_type", std::string("")}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      activity_classifier::predict_per_window,
      "----------\n"
      "data : SFrame\n"
      "    Dataset of new observations. Must include columns with the same\n"
      "    names as the features used for model training, but does not "
      "require\n"
      "    a target column. Additional columns are ignored.\n"

      "output_type : {'class', 'probability_vector'}, optional\n"
      "    Form of each prediction which is one of:\n"
      "    - 'probability_vector': Prediction probability associated with "
      "each\n"
      "      class as a vector. The probability of the first class (sorted\n"
      "      alphanumerically by name of the class in the training set) is in\n"
      "      position 0 of the vector, the second in position 1 and so on. \n"
      "      A probability_vector is given per prediction_window. \n"
      "    - 'class': Class prediction. This returns the class with maximum\n"
      "      probability per prediction_window.\n");

  REGISTER_CLASS_MEMBER_FUNCTION(activity_classifier::evaluate, "data",
                                 "metric");
  register_defaults("evaluate", {{"metric", std::string("auto")}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      activity_classifier::evaluate,
      "----------\n"
      "data : SFrame\n"
      "    Dataset of new observations. Must include columns with the same\n"
      "    names as the features used for model training, but does not require\n"
      "    a target column. Additional columns are ignored.\n"
      "metric : str, optional\n"
      "    Name of the evaluation metric.  Possible values are:\n"
      "    - 'auto'             : Returns all available metrics\n"
      "    - 'accuracy'         : Classification accuracy (micro average)\n"
      "    - 'auc'              : Area under the ROC curve (macro average)\n"
      "    - 'precision'        : Precision score (macro average)\n"
      "    - 'recall'           : Recall score (macro average)\n"
      "    - 'f1_score'         : F1 score (macro average)\n"
      "    - 'log_loss'         : Log loss\n"
      "    - 'confusion_matrix' : An SFrame with counts of possible\n"
      "                           prediction/true label combinations.\n"
      "    - 'roc_curve'        : An SFrame containing information needed for an\n"
      "                           ROC curve\n"
  );
  REGISTER_CLASS_MEMBER_FUNCTION(activity_classifier::export_to_coreml,
                                 "filename");

  END_CLASS_MEMBER_REGISTRATION

 protected:

  // Override points allowing subclasses to inject dependencies

  // Factory for data_iterator
  virtual std::unique_ptr<data_iterator> create_iterator(gl_sframe data, bool is_train) const;

  // Factory for compute_context
  virtual
  std::unique_ptr<neural_net::compute_context> create_compute_context() const;

  // Returns the initial neural network to train
  virtual std::unique_ptr<neural_net::model_spec> init_model() const;

  virtual std::tuple<gl_sframe, gl_sframe>
  init_data(gl_sframe data, variant_type validation_data,
            std::string session_id_column_name) const;

  // Support for iterative training.
  // TODO: Expose via forthcoming C-API checkpointing mechanism?
  virtual void init_train(gl_sframe data, std::string target_column_name,
                          std::string session_id_column_name,
                          gl_sframe validation_data,
                          std::map<std::string, flexible_type> opts);
  virtual void perform_training_iteration();

  virtual std::tuple<float, float>
  compute_validation_metrics(size_t prediction_window, size_t num_classes,
                             size_t batch_size);

  virtual void init_table_printer(bool has_validation);

  // Returns an SFrame where each row corresponds to one prediction, and
  // containing three columns: "session_id" indicating the session ID shared by
  // the samples in the prediction window, "preds" containing the class
  // probability vector for the prediction window, and "num_samples" indicating
  // the number of corresponding rows from the original SFrame (at most the
  // prediction window size).
  virtual gl_sframe perform_inference(data_iterator *data) const;

  // Utility code

  template <typename T>
  T read_state(const std::string& key) const {
    return variant_get_value<T>(get_state().at(key));
  }

 private:

  // Primary representation for the trained model.
  std::unique_ptr<neural_net::model_spec> nn_spec_;

  // Primary dependencies for training. These should be nonnull while training
  // is in progress.
  std::unique_ptr<data_iterator> training_data_iterator_;
  std::unique_ptr<data_iterator> validation_data_iterator_;
  std::unique_ptr<neural_net::compute_context> training_compute_context_;
  std::unique_ptr<neural_net::model_backend> training_model_;

  // Nonnull while training is in progress, if progress printing is enabled.
  std::unique_ptr<table_printer> training_table_printer_;
};

}  // namespace activity_classification
}  // namespace turi

#endif //TURI_ACTIVITY_CLASSIFIER_H_
