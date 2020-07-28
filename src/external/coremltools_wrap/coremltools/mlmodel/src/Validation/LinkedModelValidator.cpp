//
//  LinkedModelValidator.cpp
//  libmlmodelspec
#include "../Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_linkedModel>(const Specification::Model& format) {
        const auto& interface = format.description();

        // Make sure that if there is a double output type, there is exactly one output index selected.
        if (!format.has_linkedmodel()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Model not an a LinkedModel.");
        }

        if (format.isupdatable()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "LinkedModel cannot be marked as updatable");
        }
        
        Result result;

        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        if (format.linkedmodel().LinkType_case() == Specification::LinkedModel::LINKTYPE_NOT_SET) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "LinkedModel.LinkType not set.");
        }

        auto linkedFileSpec = format.linkedmodel().linkedmodelfile();
        if (linkedFileSpec.linkedmodelfilename().defaultvalue().empty()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "LinkedModel.linkedModelFile.linkedModeFileName.defaultValue cannot be empty.");
        }

        if (linkedFileSpec.linkedmodelsearchpath().defaultvalue().empty()) {
            // This is allowed and is the same as having "." or "./" as search path
        }

        return result;
    }
}
