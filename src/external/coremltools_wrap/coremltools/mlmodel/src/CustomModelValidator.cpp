//
//  CustomModelValidator.cpp
//  libmlmodelspec
#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_customModel>(const Specification::Model& format) {
        const auto& interface = format.description();

        // Make sure that if there is a double output type, there is exactly one output index selected.
        if(!format.has_custommodel()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Model not an a CustomModel.");
        }

        Result result;

        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        const auto& customModel = format.custommodel();

        if (customModel.classname().empty()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "CustomModel must have non-empty className.");
        }

        const auto& parameters = customModel.parameters();
        for(const auto& param : parameters) {
            if (param.first.empty()) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "CustomModel.parameters must have non-empty string keys.");
            }

            switch (param.second.value_case()) {
                case Specification::CustomModel_CustomModelParamValue::VALUE_NOT_SET:
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "CustomModel.parameters['"+param.first+"'] does not have a set value");
                default:
                    break;
            }
        }

        return result;
    }
}
