cmake_minimum_required(VERSION 3.0.0)
project(mlmodel)

include_directories(
  ..
  ../deps/protobuf/src
)

set(CMAKE_CXX_FLAGS " \
    ${CMAKE_CXX_FLAGS} \
    --std=c++14 \
    -DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER \
")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS " \
      ${CMAKE_CXX_FLAGS} \
      -Wglobal-constructors \
  ")
endif()
set(CMAKE_EXE_LINKER_FLAGS " \
    ${CMAKE_EXE_LINKER_FLAGS} \
    --std=c++14 \
")

add_library(mlmodel_test
    STATIC

    tests/LinearModelTests.cpp
    tests/TreeEnsembleTests.cpp
    tests/OneHotEncoderTests.cpp
    tests/InterfaceTests.cpp
    tests/UtilsTests.cpp
    tests/NNShaperTest.cpp
    tests/NNShapeTests.cpp
    tests/NNValidatorTests.cpp
    tests/ModelContainerTests.cpp
    tests/SaveLoadTests.cpp
    tests/BayesianProbitRegressionValidatorTests.cpp
    tests/NNShapeTests.cpp
    tests/NNShaperTest.cpp
    tests/KNNValidatorTests.cpp
    tests/UpdatableModelValidatorTests.cpp
    tests/ParameterTests.cpp
    tests/ModelCreationUtils.cpp
    tests/SoundAnalysisPreprocessingValidatorTests.cpp
)

set_property(TARGET mlmodel_test
    PROPERTY POSITION_INDEPENDENT_CODE ON
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
    src/Model.cpp
    src/Result.cpp
    src/Utils.cpp

    src/ArrayFeatureExtractorValidator.cpp
    src/BayesianProbitRegressionValidator.cpp
    src/CategoricalMappingValidator.cpp
    src/CustomModelValidator.cpp
    src/DictVectorizerValidator.cpp
    src/FeatureVectorizerValidator.cpp
    src/GazetteerValidator.cpp
    src/IdentityValidator.cpp
    src/ImputerValidator.cpp
    src/InterfaceValidators.cpp
    src/LinearModelValidator.cpp
    src/NeuralNetwork/NeuralNetworkValidator.cpp
    src/NeuralNetwork/NeuralNetworkLayerValidator.cpp
    src/NonMaximumSuppressionValidator.cpp
    src/NormalizerValidator.cpp
    src/OneHotEncoderValidator.cpp
    src/PipelineValidator.cpp
    src/ScalarValidator.cpp
    src/SVMValidator.cpp
    src/TreeEnsembleValidator.cpp
    src/TextClassifierValidator.cpp
    src/VisionFeaturePrintValidator.cpp
    src/WordEmbeddingValidator.cpp
    src/WordTaggerValidator.cpp
    src/QuantizationValidationUtils.cpp
    src/KNearestNeighborsClassifierValidator.cpp
    src/ParameterValidator.cpp
    src/ItemSimilarityRecommenderValidator.cpp
    src/ItemSimilarityRecommenderCommon.cpp
    src/TreeEnsembleCommon.cpp
    src/SoundAnalysisPreprocessingValidator.cpp
    src/LinkedModelValidator.cpp

    src/transforms/LinearModel.cpp
    src/transforms/Pipeline.cpp
    src/transforms/TreeEnsemble.cpp
    src/transforms/NeuralNetwork.cpp
    src/transforms/FeatureVectorizer.cpp
    src/transforms/DictVectorizer.cpp
    src/transforms/OneHotEncoder.cpp
    src/transforms/ItemSimilarityRecommender.cpp

    src/LayerShapeConstraints.cpp
    src/NeuralNetwork/NeuralNetworkShapes.cpp
    src/NeuralNetwork/UpdatableNeuralNetworkValidator.cpp
    src/QuantizationValidationUtils.cpp
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
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/format/${proto_fn}.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/format/${proto_fn}.pb.h
        COMMENT "Generating proto files for ${proto_fn}..."
        COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
            --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/format/
            -I${CMAKE_CURRENT_SOURCE_DIR}/format
            ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
        DEPENDS protoc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/format/${proto_fn}_enum.h
        COMMENT "Generating enums for ${proto_fn}..."
        COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
            --plugin=protoc-gen-enum=mlmodel/enumgen
            --enum_out=${CMAKE_CURRENT_BINARY_DIR}/format/
            -I${CMAKE_CURRENT_SOURCE_DIR}/format/
            ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
        DEPENDS enumgen protoc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_command(
        OUTPUT
            ${CMAKE_BINARY_DIR}/coremltools/proto/${proto_fn}_pb2.py
        COMMENT "Generating Python for ${proto_fn}..."
        COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
            --python_out=${CMAKE_BINARY_DIR}/coremltools/proto
            -I${CMAKE_CURRENT_SOURCE_DIR}/format/
            ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
        COMMAND python
            -m lib2to3
            -wn
            --no-diff
            -f import
            ${CMAKE_BINARY_DIR}/coremltools/${proto_fn}_pb2.py
        DEPENDS protoc
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    # For the CoreML framework we read the source file locations for these, and
    # so it can be useful to update the source tree in addition.  So we repeat
    # all of the above with different outputs.
    if(OVERWRITE_PB_SOURCE)
        add_custom_target(tgt_${proto_fn}_source ALL
            COMMENT "Generating (source) proto files for ${proto_fn}..."
            COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
                --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/build/format/
                -I${CMAKE_CURRENT_SOURCE_DIR}/format
                ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
            DEPENDS protoc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_target(tgt_${proto_fn}_enums ALL
            COMMENT "Generating (source) enums for ${proto_fn}..."
            COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
                --plugin=protoc-gen-enum=mlmodel/enumgen
                --enum_out=${CMAKE_CURRENT_SOURCE_DIR}/build/format/
                -I${CMAKE_CURRENT_SOURCE_DIR}/format/
                ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
            DEPENDS enumgen protoc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        add_custom_target(tgt_${proto_fn}_python ALL
            COMMENT "Generating (source) Python for ${proto_fn}..."
            COMMAND ${CMAKE_BINARY_DIR}/deps/protobuf/cmake/protoc
                --python_out=${CMAKE_SOURCE_DIR}/coremltools/proto
                -I${CMAKE_CURRENT_SOURCE_DIR}/format/
                ${CMAKE_CURRENT_SOURCE_DIR}/format/${proto_fn}.proto
            COMMAND python
                -m lib2to3
                -wn
                --no-diff
                -f import
                ${CMAKE_SOURCE_DIR}/coremltools/proto/${proto_fn}_pb2.py
            DEPENDS protoc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
        # Record dependencies for 'protosrc' target.
        list(APPEND proto_depends tgt_${proto_fn}_source)
        list(APPEND proto_depends tgt_${proto_fn}_enums)
        list(APPEND proto_depends tgt_${proto_fn}_python)
    endif()
endforeach()
# Add a custom 'protosrc' target that only builds the source objects from proto
# files.
add_custom_target(protosrc DEPENDS ${proto_depends})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/format)
