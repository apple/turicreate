    //
    //  ArrayFeatureExtractorValidator
    //  libmlmodelspec
    //

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_arrayFeatureExtractor>(const Specification::Model& format) {
        const auto& interface = format.description();

            // Make sure that if there is a double output type, there is exactly one output index selected.
        if(!format.has_arrayfeatureextractor()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Model not an array feature extractor.");
        }


        Result result;

            // Validate its a MLModel type.
        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

            // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(interface.input(),
                                                             1,
                                                             {Specification::FeatureType::kMultiArrayType
                                                             });
        if (!result.good()) {
            return result;
        }

            // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1,
                                                             {Specification::FeatureType::kDoubleType,
                                                                 Specification::FeatureType::kInt64Type,
                                                                 Specification::FeatureType::kMultiArrayType});

        if (!result.good()) {
            return result;
        }

        if(interface.input().size() != 1) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Exactly one input array column must be specified.");
        }

        if(interface.output(0).type().Type_case() == Specification::FeatureType::kDoubleType
           && format.arrayfeatureextractor().extractindex_size() != 1) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "If output type is Double in interface, exactly one extraction index must be specified.");
        }


            // TODO: check that the index is valid given the length of the input array.
            // auto index = format.arrayfeatureextractor().inputindex();

        return result;
    }
}
