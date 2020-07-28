#ifndef MLMODEL_GLOBALS_HPP
#define MLMODEL_GLOBALS_HPP

#include <memory>
#include <string>
#include <vector>

#include "DataType.hpp"

#define MLMODEL_SPECIFICATION_VERSION MLMODEL_SPECIFICATION_VERSION_NEWEST

namespace CoreML {

    typedef std::vector<std::pair<std::string, FeatureType>> SchemaType;
    // Version 1 shipped as iOS 11.0
    static const int32_t MLMODEL_SPECIFICATION_VERSION_IOS11 = 1;
    // Version 2 supports fp16 weights and custom layers in neural network models. Shipped in iOS 11.2
    static const int32_t MLMODEL_SPECIFICATION_VERSION_IOS11_2 = 2;

    // Version 3 supports:
    // - custom models
    // - flexible sizes,
    // - Categorical sequences (string, int64),
    // - Word tagger
    // - Text classifier
    // - Vision feature print
    // - New neural network layers (resizeBilinear, cropResize)
    // - <fill in as we develop> ..
    static const int32_t MLMODEL_SPECIFICATION_VERSION_IOS12 = 3;

    // Version 4 supports:
    // - New NN layers, non rank 5 tensors
    // - Updatable models
    // - Exact shape / general rank mapping for neural networks
    // - Large expansion of supported neural network layers
    //   - Generalized operations
    //   - Control flow
    //   - Dynmaic layers
    //   - See NeuralNetwork.proto
    // - Nearest Neighbor Classifier
    // - Sound Analysis Prepreocessing
    // - Recommender
    // - Linked Model
    static const int32_t MLMODEL_SPECIFICATION_VERSION_IOS13 = 4;

    // version 5:
    // - New NN layers part of the proto message "NeuralNetworkLayer"
    // - Non-Zero default values for optional inputs in case of Neural Networks
    // - Float32 input/output for NonmaximumSuppression model
    static const int32_t MLMODEL_SPECIFICATION_VERSION_IOS14 = 5;

    static const int32_t MLMODEL_SPECIFICATION_VERSION_NEWEST = MLMODEL_SPECIFICATION_VERSION_IOS14;

}

#endif
