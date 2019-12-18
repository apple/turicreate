//
//  UpdatableNeuralNetworkValidator.hpp
//  CoreML_framework
//

#include <stdio.h>
#include "Validators.hpp"
#include <queue>

using namespace CoreML;

Result validateInt64Parameter(const std::string& parameterName, const Specification::Int64Parameter& int64Parameter, bool shouldBePositive);

Result validateDoubleParameter(const std::string& parameterName, const Specification::DoubleParameter& doubleParameter);
