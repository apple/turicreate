//
//  UpdatableNeuralNetworkValidator.hpp
//  CoreML_framework
//

#include <stdio.h>
#include "../Validators.hpp"
#include <queue>
#include "NeuralNetworkValidatorGraph.hpp"

/*
 Top level function for validating whether a Neural network, marked as updatable
 is valid or not, which includes the check whether it is supported or not.
 */

template<typename T> ::CoreML::Result validateUpdatableNeuralNetwork(const T& nn);

template<typename T> ::CoreML::Result validateTrainingInputs(const ::CoreML::Specification::ModelDescription& modelDescription, const T& nn);
