include("${CMAKE_SOURCE_DIR}/cmake/coreml-utils.cmake")

include_directories(
  ..
  ../deps/protobuf/src
  src
)

add_definitions(-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS " \
      ${CMAKE_CXX_FLAGS} \
      -Wglobal-constructors \
  ")
endif()

add_library(mlmodel_test
    STATIC

    tests/BayesianProbitRegressionValidatorTests.cpp
    tests/InterfaceTests.cpp
    tests/KNNValidatorTests.cpp
    tests/LinearModelTests.cpp
    tests/ModelContainerTests.cpp
    tests/ModelCreationUtils.cpp
    tests/NNShaperTest.cpp
    tests/NNShapeTests.cpp
    tests/NNValidatorTests.cpp
    tests/ParameterTests.cpp
    tests/OneHotEncoderTests.cpp
    tests/SaveLoadTests.cpp
    tests/SoundAnalysisPreprocessingValidatorTests.cpp
    tests/VisionFeaturePrintValidatorTests.cpp
    tests/TreeEnsembleTests.cpp
    tests/UpdatableModelValidatorTests.cpp
    tests/UtilsTests.cpp
)

set_property(TARGET mlmodel_test
    PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_include_directories(mlmodel_test
    PRIVATE tests
)

target_link_libraries(mlmodel_test
    mlmodel
)

add_executable(mlmodel_test_runner
    test_runner.cpp
)

target_link_libraries(mlmodel_test_runner
    mlmodel_test
)

add_library(mlmodel
    STATIC

    ../deps/protobuf/src/google/protobuf/io/zero_copy_stream_impl.cc

    ${CMAKE_CURRENT_BINARY_DIR}/format/ArrayFeatureExtractor.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/BayesianProbitRegressor.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/CategoricalMapping.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/CustomModel.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/DataStructures.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/DictVectorizer.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/FeatureTypes.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/FeatureVectorizer.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/Gazetteer.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/GLMClassifier.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/GLMRegressor.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/Identity.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/Imputer.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/ItemSimilarityRecommender.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/LinkedModel.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/Model.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/NearestNeighbors.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/NeuralNetwork.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/NonMaximumSuppression.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/Normalizer.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/OneHotEncoder.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/Parameters.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/SVM.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/Scaler.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/SoundAnalysisPreprocessing.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/TreeEnsemble.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/TextClassifier.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/VisionFeaturePrint.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/WordEmbedding.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/format/WordTagger.pb.cc

    src/Comparison.cpp
    src/DataType.cpp
    src/ItemSimilarityRecommenderCommon.cpp
    src/LayerShapeConstraints.cpp
    src/Model.cpp
    src/NeuralNetworkBuffer.cpp
    src/Result.cpp
    src/TreeEnsembleCommon.cpp
    src/Utils.cpp

    src/transforms/LinearModel.cpp
    src/transforms/Pipeline.cpp
    src/transforms/TreeEnsemble.cpp
    src/transforms/NeuralNetwork.cpp
    src/transforms/FeatureVectorizer.cpp
    src/transforms/DictVectorizer.cpp
    src/transforms/OneHotEncoder.cpp
    src/transforms/ItemSimilarityRecommender.cpp

    src/Validation/ArrayFeatureExtractorValidator.cpp
    src/Validation/BayesianProbitRegressionValidator.cpp
    src/Validation/CategoricalMappingValidator.cpp
    src/Validation/CustomModelValidator.cpp
    src/Validation/DictVectorizerValidator.cpp
    src/Validation/FeatureVectorizerValidator.cpp
    src/Validation/GazetteerValidator.cpp
    src/Validation/IdentityValidator.cpp
    src/Validation/ImputerValidator.cpp
    src/Validation/InterfaceValidators.cpp
    src/Validation/ItemSimilarityRecommenderValidator.cpp
    src/Validation/KNearestNeighborsClassifierValidator.cpp
    src/Validation/LinearModelValidator.cpp
    src/Validation/LinkedModelValidator.cpp
    src/Validation/NeuralNetwork/NeuralNetworkLayerValidator.cpp
    src/Validation/NeuralNetwork/NeuralNetworkShapes.cpp
    src/Validation/NeuralNetwork/NeuralNetworkValidator.cpp
    src/Validation/NeuralNetwork/UpdatableNeuralNetworkValidator.cpp
    src/Validation/NonMaximumSuppressionValidator.cpp
    src/Validation/NormalizerValidator.cpp
    src/Validation/OneHotEncoderValidator.cpp
    src/Validation/ParameterValidator.cpp
    src/Validation/PipelineValidator.cpp
    src/Validation/QuantizationValidationUtils.cpp
    src/Validation/ScalarValidator.cpp
    src/Validation/SoundAnalysisPreprocessingValidator.cpp
    src/Validation/SVMValidator.cpp
    src/Validation/TextClassifierValidator.cpp
    src/Validation/TreeEnsembleValidator.cpp
    src/Validation/VisionFeaturePrintValidator.cpp
    src/Validation/WordEmbeddingValidator.cpp
    src/Validation/WordTaggerValidator.cpp
)

set_property(TARGET mlmodel
    PROPERTY POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(mlmodel
  libprotobuf-lite
)

add_executable(enumgen
    EXCLUDE_FROM_ALL
    tools/enumgen.cpp
    ../deps/protobuf/src/google/protobuf/compiler/plugin.pb.cc
)

set(proto_files
    ArrayFeatureExtractor
    BayesianProbitRegressor
    CategoricalMapping
    CustomModel
    DataStructures
    DictVectorizer
    FeatureTypes
    FeatureVectorizer
    Gazetteer
    GLMClassifier
    GLMRegressor
    Identity
    Imputer
    ItemSimilarityRecommender
    LinkedModel
    Model
    NearestNeighbors
    NeuralNetwork
    NonMaximumSuppression
    Normalizer
    OneHotEncoder
    Parameters
    SVM
    Scaler
    SoundAnalysisPreprocessing
    TreeEnsemble
    TextClassifier
    VisionFeaturePrint
    WordEmbedding
    WordTagger
)

target_link_libraries(enumgen
    libprotobuf
)

option(OVERWRITE_PB_SOURCE
       "Regenerates protobuf sources in the source tree, too." OFF)

# This will get filled with the dependencies for the 'protosrc' target.
set(proto_depends)
foreach(proto_fn IN ITEMS ${proto_files})
    coreml_add_build_proto(${proto_fn} "")
endforeach()
# Add a custom 'protosrc' target that only builds the source objects from proto
# files.
add_custom_target(protosrc DEPENDS ${proto_depends})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/format)
