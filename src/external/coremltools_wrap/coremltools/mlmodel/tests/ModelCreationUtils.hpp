//
//  ModelCreationUtils.h
//  CoreML_tests
//
//  Created by Anil Katti on 4/8/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#include "../src/Format.hpp"
#include "../src/Model.hpp"
#include "../src/NeuralNetwork/NeuralNetworkShapes.hpp"

#include "framework/TestUtils.hpp"

using namespace CoreML;

typedef struct {
    const char *name;
    int dimension;
} TensorAttributes;

Specification::NeuralNetwork* buildBasicUpdatableNeuralNetworkModel(Specification::Model& m);

Specification::NeuralNetwork* buildBasicNeuralNetworkModel(Specification::Model& m, bool isUpdatable, const TensorAttributes *inTensorAttr, const TensorAttributes *outTensorAttr, int numberOfLayers = 1);

Specification::NeuralNetworkClassifier* buildBasicNeuralNetworkClassifierModel(Specification::Model& m, bool isUpdatable, const TensorAttributes *inTensorAttr, std::vector<std::string> stringClassLabels, std::vector<int64_t> intClassLabels, bool includeBias);

Specification::KNearestNeighborsClassifier* buildBasicNearestNeighborClassifier(Specification::Model& m, bool isUpdatable, const TensorAttributes *inTensorAttr, const char *outTensorName);

Specification::Pipeline* buildEmptyPipelineModel(Specification::Model& m, bool isUpdatable, const TensorAttributes *inTensorAttr, const TensorAttributes *outTensorAttr);

Specification::Pipeline* buildEmptyPipelineModelWithStringOutput(Specification::Model& m, bool isUpdatable, const TensorAttributes *inTensorAttr, const char *outTensorName);

void addCategoricalCrossEntropyLossWithSoftmaxAndSGDOptimizer(Specification::Model& m, const char *softmaxInputName);

Specification::NeuralNetwork* addInnerProductLayer(Specification::Model& m, bool isUpdatable, const char *name, const TensorAttributes *inTensorAttr, const TensorAttributes *outTensorAttr);

Specification::NeuralNetwork* addSoftmaxLayer(Specification::Model& m, const char *name,  const char *input, const char *output);
