project(unity_toolkits)

add_library(unity_coreml_model_export OBJECT
  mldata_exporter.cpp
  xgboost_exporter.cpp
  linear_models_exporter.cpp
  neural_net_models_exporter.cpp
  coreml_export_utils.cpp
  mlmodel_wrapper.cpp
  MLModel/build/format/ArrayFeatureExtractor.pb.cc
  MLModel/build/format/BayesianProbitRegressor.pb.cc
  MLModel/build/format/CategoricalMapping.pb.cc
  MLModel/build/format/CustomModel.pb.cc
  MLModel/build/format/DataStructures.pb.cc
  MLModel/build/format/DictVectorizer.pb.cc
  MLModel/build/format/FeatureTypes.pb.cc
  MLModel/build/format/FeatureVectorizer.pb.cc
  MLModel/build/format/GLMClassifier.pb.cc
  MLModel/build/format/GLMRegressor.pb.cc
  MLModel/build/format/Identity.pb.cc
  MLModel/build/format/Imputer.pb.cc
  MLModel/build/format/Model.pb.cc
  MLModel/build/format/NeuralNetwork.pb.cc
  MLModel/build/format/NonMaximumSuppression.pb.cc
  MLModel/build/format/Normalizer.pb.cc
  MLModel/build/format/OneHotEncoder.pb.cc
  MLModel/build/format/SVM.pb.cc
  MLModel/build/format/Scaler.pb.cc
  MLModel/build/format/TextClassifier.pb.cc
  MLModel/build/format/TreeEnsemble.pb.cc
  MLModel/build/format/VisionFeaturePrint.pb.cc
  MLModel/build/format/WordTagger.pb.cc
  MLModel/src/ArrayFeatureExtractorValidator.cpp
  MLModel/src/BayesianProbitRegressionValidator.cpp
  MLModel/src/CategoricalMappingValidator.cpp
  MLModel/src/Comparison.cpp
  MLModel/src/CustomModelValidator.cpp
  MLModel/src/DataType.cpp
  MLModel/src/DictVectorizerValidator.cpp
  MLModel/src/FeatureVectorizerValidator.cpp
  MLModel/src/IdentityValidator.cpp
  MLModel/src/ImputerValidator.cpp
  MLModel/src/InterfaceValidators.cpp
  MLModel/src/LayerShapeConstraints.cpp
  MLModel/src/LinearModelValidator.cpp
  MLModel/src/Model.cpp
  MLModel/src/NeuralNetworkShapes.cpp
  MLModel/src/NeuralNetworkValidator.cpp
  MLModel/src/NonMaximumSuppressionValidator.cpp
  MLModel/src/NormalizerValidator.cpp
  MLModel/src/OneHotEncoderValidator.cpp
  MLModel/src/PipelineValidator.cpp
  MLModel/src/QuantizationValidationUtils.cpp
  MLModel/src/Result.cpp
  MLModel/src/SVMValidator.cpp
  MLModel/src/ScalarValidator.cpp
  MLModel/src/TextClassifierValidator.cpp
  MLModel/src/TreeEnsembleCommon.cpp
  MLModel/src/TreeEnsembleValidator.cpp
  MLModel/src/Utils.cpp
  MLModel/src/VisionFeaturePrintValidator.cpp
  MLModel/src/WordTaggerValidator.cpp
  MLModel/src/transforms/DictVectorizer.cpp
  MLModel/src/transforms/FeatureVectorizer.cpp
  MLModel/src/transforms/LinearModel.cpp
  MLModel/src/transforms/LogisticModel.cpp
  MLModel/src/transforms/NeuralNetwork.cpp
  MLModel/src/transforms/OneHotEncoder.cpp
  MLModel/src/transforms/Pipeline.cpp
  MLModel/src/transforms/TreeEnsemble.cpp
)
add_dependencies(unity_coreml_model_export boost)

# Use -w since most of this code lives in coremltools and is copied here.
# We should find and fix compiler warnings at the source (coremltools).
target_compile_options(unity_coreml_model_export PUBLIC "-w")

target_compile_definitions(unity_coreml_model_export PUBLIC CoreML=__tc_CoreML)