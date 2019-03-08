//
//  FeatureVectorizerValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_featureVectorizer>(const Specification::Model& format) {
        const auto& interface = format.description();

        Result result;

            // Validate its a MLModel type.
        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

            // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(interface.input(),
                                                             0,
                                                             {Specification::FeatureType::kInt64Type,
                                                                 Specification::FeatureType::kDoubleType,
                                                                 Specification::FeatureType::kMultiArrayType,
                                                                 Specification::FeatureType::kDictionaryType});
        if (!result.good()) {
            return result;
        }

            // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

            // Validate the parameters
        for (int i = 0; i < format.featurevectorizer().inputlist_size(); i++) {
            auto& element = format.featurevectorizer().inputlist(i);
            auto size = element.inputdimensions();
            if (size <= 0) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Dimension size must be greater tha zero.");
            }
        }

        return result;
    }
}
