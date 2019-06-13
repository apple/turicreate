/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/coreml_export/coreml_export_utils.hpp>
#include <unity/toolkits/coreml_export/linear_models_exporter.hpp>
#include <unity/toolkits/coreml_export/mldata_exporter.hpp>
#include <unity/toolkits/supervised_learning/supervised_learning_utils-inl.hpp>
#include <mlmodel/src/transforms/LinearModel.hpp>
#include <mlmodel/src/transforms/LogisticModel.hpp>

using turi::coreml::MLModelWrapper;

namespace turi {

/**
 * Export as model asset
 */
std::shared_ptr<MLModelWrapper> export_linear_regression_as_model_asset(
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context) {

  auto pipeline = std::make_shared<CoreML::Pipeline>(
      CoreML::Pipeline::Regressor(metadata->target_column_name(), ""));

  setup_pipeline_from_mldata(*pipeline, metadata);

  // Build the actual model
  CoreML::LinearModel lr = CoreML::LinearModel(metadata->target_column_name(), "");

  std::vector<double> one_hot_coefs;
  supervised::get_one_hot_encoded_coefs(coefs, metadata, one_hot_coefs);

  // Push the offset
  double offset = one_hot_coefs.back();
  lr.setOffsets({offset});

  // Push the coefs
  one_hot_coefs.pop_back();
  lr.setWeights({one_hot_coefs});

  lr.addInput("__vectorized_features__",
              CoreML::FeatureType::Array({static_cast<int64_t>(metadata->num_dimensions())}));
  lr.addOutput(metadata->target_column_name(), CoreML::FeatureType::Double());

  pipeline->add(lr);
  pipeline->addOutput(metadata->target_column_name(),
                      CoreML::FeatureType::Double());

  // Add metadata
  add_metadata(pipeline->getProto(), context);

  return std::make_shared<MLModelWrapper>(std::move(pipeline));
}

void export_linear_regression_as_model_asset(
    const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context) {

  std::shared_ptr<MLModelWrapper> coreml_model =
      export_linear_regression_as_model_asset(metadata, coefs, context);
  coreml_model->save(filename);
}


/**
 * Export linear SVM as model asset.
 */
std::shared_ptr<MLModelWrapper> export_linear_svm_as_model_asset(
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context) {

  std::string prob_column_name = metadata->target_column_name() + "Probability";
  auto pipeline = std::make_shared<CoreML::Pipeline>(
      CoreML::Pipeline::Classifier(metadata->target_column_name(),
                                   prob_column_name, ""));

  setup_pipeline_from_mldata(*pipeline, metadata);

  //////////////////////////////////////////////////////////////////////
  // Now set up the actual model.
  CoreML::LogisticModel model = CoreML::LogisticModel(metadata->target_column_name(),
                                                                        prob_column_name,
                                                                        "Linear SVM");

  std::vector<double> one_hot_coefs;
  supervised::get_one_hot_encoded_coefs(coefs, metadata, one_hot_coefs);

  size_t num_classes = metadata->target_index_size();
  double offset = one_hot_coefs.back();
  model.setOffsets({offset});
  one_hot_coefs.pop_back();
  model.setWeights({one_hot_coefs});

  auto target_output_data_type = CoreML::FeatureType::Double();
  auto target_additional_data_type = CoreML::FeatureType::Double();
  if(metadata->target_column_type() == flex_type_enum::INTEGER) {
      std::vector<int64_t> classes(num_classes);
      for(size_t i = 0; i < num_classes; ++i) {
        classes[i] = metadata->target_indexer()->map_index_to_value(i).get<flex_int>();
      }
      model.setClassNames(classes);
    target_output_data_type = CoreML::FeatureType::Int64();
    target_additional_data_type = \
              CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType);
  } else if(metadata->target_column_type() == flex_type_enum::STRING) {
      std::vector<std::string> classes(num_classes);
      for(size_t i = 0; i < num_classes; i++) {
        classes[i] = metadata->target_indexer()->map_index_to_value(i).get<std::string>();
      }
      model.setClassNames(classes);
      target_output_data_type = CoreML::FeatureType::String();
      target_additional_data_type = \
             CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_stringKeyType);

  } else {
    log_and_throw("Only exporting classifiers with an output class "
                  "of integer or string is supported.");
  }

  // Model inputs and output
  model.addInput("__vectorized_features__",
              CoreML::FeatureType::Array({static_cast<int64_t>(metadata->num_dimensions())}));
  model.addOutput(metadata->target_column_name(), target_output_data_type);
  model.addOutput(prob_column_name, target_additional_data_type);

  // Pipeline outputs
  pipeline->add(model);
  pipeline->addOutput(metadata->target_column_name(), target_output_data_type);
  pipeline->addOutput(prob_column_name, target_additional_data_type);

  // Add metadata
  add_metadata(pipeline->getProto(), context);

  return std::make_shared<MLModelWrapper>(std::move(pipeline));
}

void export_linear_svm_as_model_asset(
    const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context) {

  std::shared_ptr<MLModelWrapper> coreml_model =
      export_linear_svm_as_model_asset(metadata, coefs, context);
  coreml_model->save(filename);
}


/**
 * Export logistic regression as model asset.
 */
std::shared_ptr<MLModelWrapper> export_logistic_model_as_model_asset(
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context) {


  std::string prob_column_name = metadata->target_column_name() + "Probability";

  auto pipeline = std::make_shared<CoreML::Pipeline>(
      CoreML::Pipeline::Classifier(metadata->target_column_name(),
                                   prob_column_name, ""));

  setup_pipeline_from_mldata(*pipeline, metadata);

  //////////////////////////////////////////////////////////////////////
  // Now set up the actual model.
  //


  CoreML::LogisticModel model = CoreML::LogisticModel(metadata->target_column_name(),
                                                                        prob_column_name,
                                                                        "Logistic Regression");

  std::vector<double> one_hot_coefs;
  supervised::get_one_hot_encoded_coefs(coefs, metadata, one_hot_coefs);

  // Set weights and intercepts
  std::vector<double> offsets;
  std::vector<std::vector<double>> weights;
  size_t num_classes = metadata->target_index_size();
  size_t variables_per_class = one_hot_coefs.size() / (num_classes - 1);
  for(size_t i = 0; i < num_classes - 1; i++) {
    size_t starting_index = i * variables_per_class;
    weights.push_back(std::vector<double>());
    for(size_t j = 0; j < variables_per_class - 1; j++) {
      weights[i].push_back(one_hot_coefs[starting_index + j]);
    }
    double cur_offset = one_hot_coefs[starting_index + variables_per_class - 1];
    offsets.push_back(cur_offset);
  }
  model.setWeights(weights);
  model.setOffsets(offsets);

  auto target_output_data_type = CoreML::FeatureType::Double();
  auto target_additional_data_type = CoreML::FeatureType::Double();
  if(metadata->target_column_type() == flex_type_enum::INTEGER) {
      std::vector<int64_t> classes(num_classes);
      for(size_t i = 0; i < num_classes; ++i) {
        classes[i] = metadata->target_indexer()->map_index_to_value(i).get<flex_int>();
      }
      model.setClassNames(classes);
    target_output_data_type = CoreML::FeatureType::Int64();
    target_additional_data_type = \
              CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_int64KeyType);
  } else if(metadata->target_column_type() == flex_type_enum::STRING) {
      std::vector<std::string> classes(num_classes);
      for(size_t i = 0; i < num_classes; i++) {
        classes[i] = metadata->target_indexer()->map_index_to_value(i).get<std::string>();
      }
      model.setClassNames(classes);
      target_output_data_type = CoreML::FeatureType::String();
      target_additional_data_type = \
             CoreML::FeatureType::Dictionary(MLDictionaryFeatureTypeKeyType_stringKeyType);

  } else {
    log_and_throw("Only exporting classifiers with an output class "
                  "of integer or string is supported.");
  }

  // Model inputs and output
  model.addInput("__vectorized_features__",
              CoreML::FeatureType::Array({static_cast<int64_t>(metadata->num_dimensions())}));
  model.addOutput(metadata->target_column_name(), target_output_data_type);
  model.addOutput(prob_column_name, target_additional_data_type);

  // Pipeline outputs
  pipeline->add(model);
  pipeline->addOutput(metadata->target_column_name(), target_output_data_type);
  pipeline->addOutput(prob_column_name, target_additional_data_type);

  // Add metadata
  add_metadata(pipeline->getProto(), context);

  return std::make_shared<MLModelWrapper>(std::move(pipeline));
}

void export_logistic_model_as_model_asset(
    const std::string& filename,
    const std::shared_ptr<ml_metadata>& metadata,
    const Eigen::Matrix<double, Eigen::Dynamic,1>& coefs,
    const std::map<std::string, flexible_type>& context) {

  std::shared_ptr<MLModelWrapper> coreml_model =
      export_logistic_model_as_model_asset(metadata, coefs, context);
  coreml_model->save(filename);
}


}  // namespace turi
