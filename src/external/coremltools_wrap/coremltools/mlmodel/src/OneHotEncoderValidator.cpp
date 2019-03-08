//
//  OneHotEncoderValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_oneHotEncoder>(const Specification::Model& format) {
        const auto& interface = format.description();

        Result result;

        // Validate its a MLModel type.
        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1,
                                                             { Specification::FeatureType::kInt64Type,
                                                                 Specification::FeatureType::kStringType });
        if (!result.good()) {
            return result;
        }

        // If sparse then the input must be dictionary
        bool sparse = format.onehotencoder().outputsparse();

        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1,
                                                             {(sparse ? Specification::FeatureType::kDictionaryType : Specification::FeatureType::kMultiArrayType)});
        // Validate the outputs
        if (!result.good()) {
            return result;
        }

        // Validate the parameters
        switch (format.onehotencoder().CategoryType_case()) {
            case Specification::OneHotEncoder::kInt64Categories:
                break;
            case Specification::OneHotEncoder::kStringCategories:
                break;
            case Specification::OneHotEncoder::CATEGORYTYPE_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "OneHotEncoder parameter incorrect type");
        }

        return result;
    }
}
