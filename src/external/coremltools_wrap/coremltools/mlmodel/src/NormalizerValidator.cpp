//
//  NormalizerValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_normalizer>(const Specification::Model& format) {
        const auto& interface = format.description();

        Result result;

        // Validate its a MLModel type.
        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        // Validate the parameters
        auto normLx = format.normalizer().normtype();
        if (normLx != Specification::Normalizer_NormType::Normalizer_NormType_L1 &&
            normLx != Specification::Normalizer_NormType::Normalizer_NormType_L2 &&
            normLx != Specification::Normalizer_NormType::Normalizer_NormType_LMax) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "NormalizerValidator normLx invalid");
        }

        return result;
    }
}
