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

    build/format/ArrayFeatureExtractor.pb.cc
    build/format/BayesianProbitRegressor.pb.cc
    build/format/CategoricalMapping.pb.cc
    build/format/CustomModel.pb.cc
    build/format/DataStructures.pb.cc
    build/format/DictVectorizer.pb.cc
    build/format/FeatureTypes.pb.cc
    build/format/FeatureVectorizer.pb.cc
    build/format/GLMClassifier.pb.cc
    build/format/GLMRegressor.pb.cc
    build/format/Identity.pb.cc
    build/format/Imputer.pb.cc
    build/format/Model.pb.cc
    build/format/NeuralNetwork.pb.cc
    build/format/NonMaximumSuppression.pb.cc
    build/format/Normalizer.pb.cc
    build/format/OneHotEncoder.pb.cc
    build/format/SVM.pb.cc
    build/format/Scaler.pb.cc
    build/format/TreeEnsemble.pb.cc
    build/format/TextClassifier.pb.cc
    build/format/VisionFeaturePrint.pb.cc
    build/format/WordTagger.pb.cc


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
    src/IdentityValidator.cpp
    src/ImputerValidator.cpp
    src/InterfaceValidators.cpp
    src/LinearModelValidator.cpp
    src/NeuralNetworkValidator.cpp
    src/NonMaximumSuppressionValidator.cpp
    src/NormalizerValidator.cpp
    src/OneHotEncoderValidator.cpp
    src/PipelineValidator.cpp
    src/ScalarValidator.cpp
    src/SVMValidator.cpp
    src/TreeEnsembleValidator.cpp
    src/TextClassifierValidator.cpp
    src/VisionFeaturePrintValidator.cpp
    src/WordTaggerValidator.cpp
    src/QuantizationValidationUtils.cpp

    src/TreeEnsembleCommon.cpp

    src/transforms/LinearModel.cpp
    src/transforms/Pipeline.cpp
    src/transforms/TreeEnsemble.cpp
    src/transforms/NeuralNetwork.cpp
    src/transforms/FeatureVectorizer.cpp
    src/transforms/DictVectorizer.cpp
    src/transforms/OneHotEncoder.cpp

    src/LayerShapeConstraints.cpp
    src/NeuralNetworkShapes.cpp
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

target_link_libraries(enumgen
    libprotobuf
)

add_custom_target(protosrc
    COMMAND mkdir -p build/format
    COMMAND protoc --cpp_out=build/format/ -Iformat/ format/*.proto
    COMMAND protoc --plugin=protoc-gen-enum=enumgen --enum_out=build/format/ -Iformat/ format/*.proto
    COMMAND protoc --python_out=../coremltools/proto/ -Iformat/ format/*.proto
    COMMAND python -m lib2to3 -wn --no-diff -f import ../coremltools/proto/*.py
    DEPENDS enumgen
    WORKING_DIRECTORY .
    COMMENT "Generating protobuf generated sources from .proto."
    SOURCES format/ArrayFeatureExtractor.proto
        format/BayesianProbitRegressor.proto
        format/CategoricalMapping.proto
        format/CustomModel.proto
        format/DataStructures.proto
        format/DictVectorizer.proto
        format/FeatureTypes.proto
        format/FeatureVectorizer.proto
        format/GLMClassifier.proto
        format/GLMRegressor.proto
        format/Identity.proto
        format/Imputer.proto
        format/Model.proto
        format/NeuralNetwork.proto
        format/NonMaximumSuppression.proto
        format/Normalizer.proto
        format/OneHotEncoder.proto
        format/SVM.proto
        format/Scaler.proto
        format/TextClassifier.proto
        format/TreeEnsemble.proto
        format/VisionFeaturePrint.proto
        format/WordTagger.proto
)
