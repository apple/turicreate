/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DRAWING_CLASSIFIER_H_
#define TURI_DRAWING_CLASSIFIER_H_

#include <core/logging/table_printer/table_printer.hpp>
#include <model_server/lib/extensions/ml_model.hpp>
#include <model_server/lib/variant.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <toolkits/coreml_export/mlmodel_wrapper.hpp>
#include <ml/neural_net/compute_context.hpp>
#include <ml/neural_net/model_backend.hpp>
#include <ml/neural_net/model_spec.hpp>

namespace turi {
namespace drawing_classifier {

class EXPORT drawing_classifier: public ml_model_base {

 public:
  
  // ml_model_base interface

  /* Commented out for the purpose of a skeleton. */
  // void init_options(const std::map<std::string, flexible_type>& opts) override;
  // size_t get_version() const override;
  // void save_impl(oarchive& oarc) const override;
  // void load_version(iarchive& iarc, size_t version) override;

  // Interface exposed via Unity server

  void train(gl_sframe data,
             std::string target_column_name, std::string feature_column_name,
             variant_type validation_data,
             std::map<std::string, flexible_type> opts);
  gl_sarray predict(gl_sframe data, std::string output_type);
  gl_sframe predict_topk(gl_sframe data, std::string output_type, size_t k);
  variant_map_type evaluate(gl_sframe data, std::string metric);
  std::shared_ptr<coreml::MLModelWrapper> export_to_coreml(
      std::string filename);
  
  BEGIN_CLASS_MEMBER_REGISTRATION("drawing_classifier")

  IMPORT_BASE_CLASS_REGISTRATION(ml_model_base);

  REGISTER_CLASS_MEMBER_FUNCTION(drawing_classifier::train, "data",
                                 "target_column_name", "feature_column_name",
                                 "validation_data", "options");
  register_defaults("train",
                    {{"validation_data", to_variant(std::string("auto"))},
                     {"options",
                      to_variant(std::map<std::string, flexible_type>())}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      drawing_classifier::train,
      "----------\n"
      "data : SFrame\n"
      "    Input data, which consists of columns named by the\n"
      "    feature_column_name and target_column_name parameters, used for\n"
      "    training the Drawing Classifier."
      "target_column_name : string\n"
      "    Name of the column containing the target variable. The values in "
      "    this column must be of string type.\n"
      "feature_column_name : string\n"
      "    Name of the column containing the input drawings.\n"
      "    The feature column can contain either bitmap-based drawings or\n"
      "    stroke-based drawings. Bitmap-based drawing input can be a\n"
      "    grayscale tc.Image of any size.\n"
      "\n"
      "    Stroke-based drawing input must be in the following format:\n"
      "    Every drawing must be represented by a list of strokes, where each\n"
      "    stroke must be a list of points in the order in which they were\n"
      "    drawn on the canvas.\n"
      "\n"
      "    Each point must be a dictionary with two keys,\n"
      "    \"x\" and \"y\", and their\n"
      "    respective values must be numerical, i.e. either integer or float.\n"
      "validatation_data : SFrame or string\n"
      "    A dataset for monitoring the model's generalization performance to\n"
      "    prevent the model from overfitting to the training data.\n"
      "\n"
      "    For each row of the progress table, accuracy is measured over the\n"
      "    provided training dataset and the `validation_data`. The format of\n"
      "    this SFrame must be the same as the training set.\n"
      "\n"
      "    When set to 'auto', a validation set is automatically sampled from "
      "the\n"
      "    training data (if the training data has > 100 sessions).\n"
      "options : dict\n"
      "\n"
      "Options\n"
      "-------\n"
      "max_iterations : int\n"
      "    Maximum number of iterations/epochs made over the data during the\n"
      "    training phase. The default is 10 iterations.\n"
      "batch_size : int\n"
      "    Number of sequence chunks used per training step. Must be greater "
      "than\n"
      "    the number of GPUs in use. The default is 32.\n");

  REGISTER_CLASS_MEMBER_FUNCTION(drawing_classifier::predict, "data",
                                 "output_type");
  register_defaults("predict", {{"output_type", std::string("class")}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      drawing_classifier::predict,
      "----------\n"
      "data : SFrame\n"
      "    The drawing(s) on which to perform drawing classification.\n"
      "    If dataset is an SFrame, it must have a column with the same name\n"
      "    as the feature column during training. Additional columns are\n"
      "    ignored.\n"
      "    If the data is a single drawing, it can be either of type tc.Image,\n"
      "    in which case it is a bitmap-based drawing input,\n"
      "    or of type list, in which case it is a stroke-based drawing input.\n"
      "output_type : {\"class\", \"probability_vector\"}, optional\n"
      "    Form of each prediction which is one of:\n"
      "    - \"probability_vector\": Prediction probability associated with \n"
      "      each class as a vector. The probability of first class (sorted\n"
      "      alphanumerically by name of the class in the training set) is in\n"
      "      position 0 of the vector, the second in position 1 and so on.\n"
      "    - \"class\": Class prediction. This returns the class with maximum\n"
      "      probability.\n"
  );

  REGISTER_CLASS_MEMBER_FUNCTION(drawing_classifier::predict_topk,
                                 "data", "output_type", "k");
  register_defaults("predict_topk", {{"output_type", std::string("probability")}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      drawing_classifier::predict_topk,
      "----------\n"
      "data : SFrame\n"
      "    Dataset of new observations.\n"
      "    SFrame must include columns with the same\n"
      "    names as the features used for model training, but does not\n"
      "    require a target column. Additional columns are ignored."
      "output_type : {\"probability\", \"rank\"}, optional\n"
      "    Form of each prediction which is one of:\n"
      "    - \"probability\": Probability associated with each label in the\n"
      "      prediction\n"
      "    - \"rank\": Rank associated with each label in the prediction.\n"
      "k : int\n"
      "    Number of classes to return for each input example.\n");

  REGISTER_CLASS_MEMBER_FUNCTION(drawing_classifier::evaluate, "data",
                                 "metric");
  register_defaults("evaluate", {{"metric", std::string("auto")}});
  REGISTER_CLASS_MEMBER_DOCSTRING(
      drawing_classifier::evaluate,
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
  REGISTER_CLASS_MEMBER_FUNCTION(drawing_classifier::export_to_coreml,
                                 "filename");

  END_CLASS_MEMBER_REGISTRATION

 protected:

  // Returns the initial neural network to train
  virtual std::unique_ptr<neural_net::model_spec> init_model() const;


 private:
  // Primary representation for the trained model.
  std::unique_ptr<neural_net::model_spec> nn_spec_;
  std::unique_ptr<neural_net::model_backend> training_model_;
  std::unique_ptr<neural_net::compute_context> training_compute_context_;

};

}  // namespace drawing_classifier
}  // namespace turi

#endif //TURI_DRAWING_CLASSIFIER_H_
