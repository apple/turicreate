project(TuriExternalDependencies)

# Handle the coremltools dependencies.

# If this variable is specified, then we pull the coremltools
#from that repo to compile in.It should just work here.
if(TC_COREMLTOOLS_MLMODEL_DIR)

  file(GLOB _mlmodel_files
    ${TC_COREMLTOOLS_MLMODEL_DIR}/build/format/*.cc
    ${TC_COREMLTOOLS_MLMODEL_DIR}/src/*.cpp
    ${TC_COREMLTOOLS_MLMODEL_DIR}/src/transforms/*.cpp)

  make_library(coremltools_mlmodel
    OBJECT
    SOURCES
      ${_mlmodel_files}
    REQUIRES protobuf)

target_include_directories(coremltools_mlmodel INTERFACE ${TC_COREMLTOOLS_MLMODEL_DIR}/)

else()

# If the above variable is not specified, then we use our built in version
# of coremltools.  Each of the files is added explicitly here so we can have
# a literal copy of coremltools in this repo, including (ignored)
# CMakeLists.txt files and thus upgrade it easily.
#
# To upgrade coremltools, copy it into this directory:
#
#
# 1. Run `git clean -fdx` in the coremltools repo.
#
# 2. In this directory, run `rsync -av --delete -f '- *.git/' <coremltools dir> coremltools/
#
# 3. Run `git add coremltools/` to add everything in.
#
# 4. Run 'find . | grep -E 'mlmodel/((src)|(build/format)).*((cpp)|(cc))'`
#    to generate the list of files to add in below.
#


make_library(coremltools_mlmodel OBJECT
  SOURCES
    coremltools/mlmodel/build/format/ArrayFeatureExtractor.pb.cc
    coremltools/mlmodel/build/format/BayesianProbitRegressor.pb.cc
    coremltools/mlmodel/build/format/CategoricalMapping.pb.cc
    coremltools/mlmodel/build/format/CustomModel.pb.cc
    coremltools/mlmodel/build/format/DataStructures.pb.cc
    coremltools/mlmodel/build/format/DictVectorizer.pb.cc
    coremltools/mlmodel/build/format/FeatureTypes.pb.cc
    coremltools/mlmodel/build/format/FeatureVectorizer.pb.cc
    coremltools/mlmodel/build/format/GLMClassifier.pb.cc
    coremltools/mlmodel/build/format/GLMRegressor.pb.cc
    coremltools/mlmodel/build/format/Identity.pb.cc
    coremltools/mlmodel/build/format/Imputer.pb.cc
    coremltools/mlmodel/build/format/Model.pb.cc
    coremltools/mlmodel/build/format/NeuralNetwork.pb.cc
    coremltools/mlmodel/build/format/NonMaximumSuppression.pb.cc
    coremltools/mlmodel/build/format/Normalizer.pb.cc
    coremltools/mlmodel/build/format/OneHotEncoder.pb.cc
    coremltools/mlmodel/build/format/SVM.pb.cc
    coremltools/mlmodel/build/format/Scaler.pb.cc
    coremltools/mlmodel/build/format/TextClassifier.pb.cc
    coremltools/mlmodel/build/format/TreeEnsemble.pb.cc
    coremltools/mlmodel/build/format/VisionFeaturePrint.pb.cc
    coremltools/mlmodel/build/format/WordTagger.pb.cc
    coremltools/mlmodel/src/ArrayFeatureExtractorValidator.cpp
    coremltools/mlmodel/src/BayesianProbitRegressionValidator.cpp
    coremltools/mlmodel/src/CategoricalMappingValidator.cpp
    coremltools/mlmodel/src/Comparison.cpp
    coremltools/mlmodel/src/CustomModelValidator.cpp
    coremltools/mlmodel/src/DataType.cpp
    coremltools/mlmodel/src/DictVectorizerValidator.cpp
    coremltools/mlmodel/src/FeatureVectorizerValidator.cpp
    coremltools/mlmodel/src/IdentityValidator.cpp
    coremltools/mlmodel/src/ImputerValidator.cpp
    coremltools/mlmodel/src/InterfaceValidators.cpp
    coremltools/mlmodel/src/LayerShapeConstraints.cpp
    coremltools/mlmodel/src/LinearModelValidator.cpp
    coremltools/mlmodel/src/Model.cpp
    coremltools/mlmodel/src/NeuralNetworkShapes.cpp
    coremltools/mlmodel/src/NeuralNetworkValidator.cpp
    coremltools/mlmodel/src/NonMaximumSuppressionValidator.cpp
    coremltools/mlmodel/src/NormalizerValidator.cpp
    coremltools/mlmodel/src/OneHotEncoderValidator.cpp
    coremltools/mlmodel/src/PipelineValidator.cpp
    coremltools/mlmodel/src/QuantizationValidationUtils.cpp
    coremltools/mlmodel/src/Result.cpp
    coremltools/mlmodel/src/SVMValidator.cpp
    coremltools/mlmodel/src/ScalarValidator.cpp
    coremltools/mlmodel/src/TextClassifierValidator.cpp
    coremltools/mlmodel/src/TreeEnsembleCommon.cpp
    coremltools/mlmodel/src/TreeEnsembleValidator.cpp
    coremltools/mlmodel/src/Utils.cpp
    coremltools/mlmodel/src/VisionFeaturePrintValidator.cpp
    coremltools/mlmodel/src/WordTaggerValidator.cpp
    coremltools/mlmodel/src/transforms/DictVectorizer.cpp
    coremltools/mlmodel/src/transforms/FeatureVectorizer.cpp
    coremltools/mlmodel/src/transforms/LinearModel.cpp
    coremltools/mlmodel/src/transforms/LogisticModel.cpp
    coremltools/mlmodel/src/transforms/NeuralNetwork.cpp
    coremltools/mlmodel/src/transforms/OneHotEncoder.cpp
    coremltools/mlmodel/src/transforms/Pipeline.cpp
    coremltools/mlmodel/src/transforms/TreeEnsemble.cpp
  REQUIRES
    protobuf
)

target_include_directories(coremltools_mlmodel INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/coremltools/>
  $<INSTALL_INTERFACE:coremltools/>)

endif()
