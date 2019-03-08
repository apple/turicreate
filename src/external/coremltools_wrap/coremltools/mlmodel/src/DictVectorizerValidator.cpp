//
//  DictVectorizerValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_dictVectorizer>(const Specification::Model& format) {
        const auto& interface = format.description();

        Result result;

        // Validate its a MLModel type
        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {Specification::FeatureType::kDictionaryType});
        if (!result.good()) {
            return result;
        }

        // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kDictionaryType});
        if (!result.good()) {
            return result;
        }

        // Validate the parameters
        switch (format.dictvectorizer().Map_case()) {
            case Specification::DictVectorizer::kInt64ToIndex:
            case Specification::DictVectorizer::kStringToIndex:
                break;
            case Specification::DictVectorizer::MAP_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "DictVectorizerValidator parameter not set");
        }

        return result;
    }
}
