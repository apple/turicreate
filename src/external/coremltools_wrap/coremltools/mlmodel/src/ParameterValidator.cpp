//
//  UpdatableNeuralNetworkValidator.cpp
//  CoreML_framework
//


#include "ParameterValidator.hpp"

using namespace CoreML;

Result validateInt64Parameter(const std::string& parameterName, const Specification::Int64Parameter& int64Parameter, bool shouldBePositive) {
    
    const ::google::protobuf::int64 defaultValue = int64Parameter.defaultvalue();

    if (shouldBePositive) {
        if (defaultValue <= 0) {
            std::string err = "Default Value (" + std::to_string(defaultValue) + ") for '" + parameterName + "' expected to be a positive value.";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
        }
    }

    if (int64Parameter.has_set()) {
        bool valueFoundInSet = false;
        for (const auto &value : int64Parameter.set().values()) {
            if (value == defaultValue) {
                valueFoundInSet = true;
                break;
            }
            if (value <= 0 && shouldBePositive) {
                std::string err = "Non-positive value (" + std::to_string(value) + ") in Allowed Values Set for '" + parameterName + "' is not allowed.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
        }
        if (false == valueFoundInSet) {
            std::string err = "Specified Default Value (" + std::to_string(defaultValue) + ") not found in Allowed Values Set for '" + parameterName + "'";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
        }
    }
    else if (int64Parameter.has_range()) {
        const Specification::Int64Range& range = int64Parameter.range();

        if (range.minvalue() <= 0 && shouldBePositive) {
            std::string err = "Non-positive min value (" + std::to_string(range.minvalue()) + ") in Allowed Value Range for '" + parameterName + "' is not allowed.";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
        }

        if (range.maxvalue() <= 0 && shouldBePositive) {
            std::string err = "Non-positive max value (" + std::to_string(range.maxvalue()) + ") in Allowed Value Range for '" + parameterName + "' is not allowed.";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
        }

        if (defaultValue < range.minvalue() || defaultValue > range.maxvalue()) {
            std::string err = "Specified Default Value (" + std::to_string(defaultValue) + ") out of Allowed Value Range for '" + parameterName + "'";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
        }

        if (range.minvalue() > range.maxvalue()) {
            std::string err = "Specified minimum value (" + std::to_string(defaultValue) + ") greater than maximum value for '" + parameterName + "'";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);

        }
    }
    
    return Result();
}

Result validateDoubleParameter(const std::string& parameterName, const Specification::DoubleParameter& doubleParameter) {
    
    const ::google::protobuf::double_t defaultValue = doubleParameter.defaultvalue();
    
    if (doubleParameter.has_range()) {
        const Specification::DoubleRange& range = doubleParameter.range();
        
        if (defaultValue < range.minvalue() || defaultValue > range.maxvalue()) {
            std::string err = "Specified Default Value (" + std::to_string(defaultValue) + ") out of Allowed Value Range for '" + parameterName + "'";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
        }
    }
    
    return Result();
}

