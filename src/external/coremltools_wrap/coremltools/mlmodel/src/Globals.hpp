#ifndef MLMODEL_GLOBALS_HPP
#define MLMODEL_GLOBALS_HPP

#include <memory>
#include <string>
#include <vector>

#include "DataType.hpp"

#define MLMODEL_SPECIFICATION_VERSION MLMODEL_SPECIFICATION_VERSION_IOS12

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
    // - <fill in as we develope> ..
    static const int32_t MLMODEL_SPECIFICATION_VERSION_IOS12 = 3;

}

#endif
