//
//  UpdatableModelValidatorTests.cpp
//  CoreML_framework
//
//  Created by aseem wadhwa on 2/12/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#include "../src/Format.hpp"
#include "../src/Model.hpp"
#include "../src/NeuralNetwork/NeuralNetworkShapes.hpp"

#include "framework/TestUtils.hpp"

using namespace CoreML;

template <class NeuralNetworkClass> void addLearningRate(NeuralNetworkClass *nn, Specification::Optimizer::OptimizerTypeCase optimizerType, double defaultValue, double minValue,  double maxValue);

template <class NeuralNetworkClass> void addMiniBatchSize(NeuralNetworkClass *nn, Specification::Optimizer::OptimizerTypeCase optimizerType, int64_t defaultValue, int64_t minValue, int64_t maxValue, std::set<int64_t> allowedValues = std::set<int64_t>());

template <class NeuralNetworkClass> void addEpochs(NeuralNetworkClass *nn, int64_t defaultValue, int64_t minValue, int64_t maxValue, std::set<int64_t> allowedValues = std::set<int64_t>());

template <class NeuralNetworkClass> void addShuffleAndSeed(NeuralNetworkClass *nn, int64_t defaultValue, int64_t minValue, int64_t maxValue, std::set<int64_t> allowedValues);

template <class NeuralNetworkClass> void addCategoricalCrossEntropyLoss(Specification::Model& m, NeuralNetworkClass *nn, const char *lossName, const char *softmaxInputName, const char *targetName);

template <class NeuralNetworkClass> void addMeanSquareError(Specification::Model& m, NeuralNetworkClass *nn, const char *lossName, const char *mseInputName, const char *targetName);

void addMomentum(Specification::NeuralNetwork *nn, Specification::Optimizer::OptimizerTypeCase optimizerType, double defaultValue, double minValue, double maxValue);

void addBeta1(Specification::NeuralNetwork *nn, Specification::Optimizer::OptimizerTypeCase optimizerType, double defaultValue, double minValue, double maxValue);

void addBeta2(Specification::NeuralNetwork *nn, Specification::Optimizer::OptimizerTypeCase optimizerType, double defaultValue, double minValue, double maxValue);

void addEps(Specification::NeuralNetwork *nn, Specification::Optimizer::OptimizerTypeCase optimizerType, double defaultValue, double minValue, double maxValue);
