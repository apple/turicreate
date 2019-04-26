//
//  CategoricalMappingValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_categoricalMapping>(const Specification::Model& format) {
        auto interface = format.description();

        Result result;

        // Validate its a MLModel type.
        result = validateModelDescription(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        auto mapping_type = format.categoricalmapping().MappingType_case();
        auto defval_type = format.categoricalmapping().ValueOnUnknown_case();
        Specification::FeatureType::TypeCase requiredInputType;
        Specification::FeatureType::TypeCase requiredOutputType;
        Specification::SequenceFeatureType::TypeCase requiredInputSeqType;
        Specification::SequenceFeatureType::TypeCase requiredOutputSeqType;

        switch(mapping_type) {
            case Specification::CategoricalMapping::MappingTypeCase::kStringToInt64Map:

                if(defval_type == Specification::CategoricalMapping::ValueOnUnknownCase::kStrValue) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "ValueOnUnknown set to string value while mapping produces int64.");
                }
                requiredInputType = Specification::FeatureType::kStringType;
                requiredOutputType = Specification::FeatureType::kInt64Type;
                requiredInputSeqType = Specification::SequenceFeatureType::kStringType;
                requiredOutputSeqType = Specification::SequenceFeatureType::kInt64Type;

                break;

            case Specification::CategoricalMapping::MappingTypeCase::kInt64ToStringMap:
                if(defval_type == Specification::CategoricalMapping::ValueOnUnknownCase::kInt64Value) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "ValueOnUnknown set to Int64 value while mapping produces string.");
                }
                requiredOutputType = Specification::FeatureType::kStringType;
                requiredInputType = Specification::FeatureType::kInt64Type;
                requiredOutputSeqType = Specification::SequenceFeatureType::kStringType;
                requiredInputSeqType = Specification::SequenceFeatureType::kInt64Type;

                break;

            case Specification::CategoricalMapping::MappingTypeCase::MAPPINGTYPE_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Mapping not set.");
        }

        // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {requiredInputType, Specification::FeatureType::kSequenceType});
        if (!result.good()) {
            return result;
        }

        // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {requiredOutputType, Specification::FeatureType::kSequenceType});
        if (!result.good()) {
            return result;
        }

        // Check if the input was a sequence
        if (interface.input(0).type().Type_case() == Specification::FeatureType::kSequenceType) {

            // Make sure its the correct input type
            if (interface.input(0).type().sequencetype().Type_case() != requiredInputSeqType) {
                return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE,
                              std::string("Input sequence type does not match input type ") +
                              MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(requiredInputType)) +
                              "of categorical mapping.");
            }

            // Make sure the outupt is a sequence as well
            if (interface.output(0).type().Type_case() != Specification::FeatureType::kSequenceType) {
                return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE,
                              "Output of a sequence categorical mapping must be a sequence");
            }

            // Make sure the otuput is the correct type
            if (interface.output(0).type().sequencetype().Type_case() != requiredOutputSeqType) {
                return Result(ResultType::UNSUPPORTED_FEATURE_TYPE_FOR_MODEL_TYPE,
                              std::string("Output sequence type does not match input type ") +
                              MLFeatureTypeType_Name(static_cast<MLFeatureTypeType>(requiredOutputType)) +
                              "of categorical mapping.");
            }
        }


        // Validate the parameters
        return result;
    }
}
