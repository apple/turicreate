//
//  ImputerValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_imputer>(const Specification::Model& format) {
        const auto& description = format.description();

        Result result;

        // Validate its a MLModel type.
        result = validateModelDescription(description, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        // Convenience typedefs
        typedef Specification::FeatureType FT;
        typedef Specification::Imputer::ReplaceValueCase RVC;


        std::vector<Specification::FeatureType::TypeCase> possible_types = {
            FT::kInt64Type, FT::kDoubleType, FT::kMultiArrayType,
            FT::kStringType, FT::kDictionaryType};

        // Validate the inputs, though we'll have to do more of this manually.
        result = validateDescriptionsContainFeatureWithTypes(description.input(), 1, possible_types);

        if (!result.good()) {
            return result;
        }

        // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(description.output(), 1, possible_types);

        if (!result.good()) {
            return result;
        }

            // From the above, we know that we have exactly one input and one output type.
        const auto& input = description.input()[0];
        const auto& output = description.output()[0];

        // make sure the input and the output match.
        if(output.type().Type_case() != input.type().Type_case()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Type of input feature does not match the output type feature.");
        }

        // If it's an array, we need to test sizes.
        if(input.type().Type_case() == FT::kMultiArrayType) {
            if(input.type().multiarraytype().shape_size() != 1) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                "Only 1 dimensional arrays input features are supported by the imputer.");
            }

            if(output.type().multiarraytype().shape_size() != 1
               || (input.type().multiarraytype().shape(0) != output.type().multiarraytype().shape(0))) {
                return  Result(ResultType::INVALID_MODEL_PARAMETERS,
                                "Shape of output array does not match shape of input array.");
            }
        }


        // Input and output types are allowed based on the values present in the
        auto checkInterfaceCompatible =
        [&](const std::set<Specification::FeatureType::TypeCase>& compatibleFeatureTypes,
            const std::set<Specification::Imputer::ReplaceValueCase>& compatibleReplaceTypes) {

            if(!compatibleFeatureTypes.count(input.type().Type_case())) {
                result = Result(ResultType::INVALID_MODEL_PARAMETERS,
                       "Type of input feature \"" + input.name() + "\" is not compatible with given imputed value type.");
                return false;
            }


            // Check to make sure the the replace value is fine
            if(format.imputer().ReplaceValue_case() != Specification::Imputer::REPLACEVALUE_NOT_SET) {
                if(!compatibleReplaceTypes.count(format.imputer().ReplaceValue_case())) {
                    result = Result(ResultType::INVALID_MODEL_PARAMETERS,
                                    "Type of given replace value not compatible with input feature type.");
                    return false;
                }
            }

            // We're all okay now.
            return true;
        };

        // Validate the different combinations of stuff.
        const auto& imputer = format.imputer();
        switch (imputer.ImputedValue_case()) {
            case Specification::Imputer::kImputedDoubleValue:
                if(!checkInterfaceCompatible({FT::kDoubleType, FT::kMultiArrayType}, {RVC::kReplaceDoubleValue})) {
                        return result;
                    }
                break;

            case Specification::Imputer::kImputedInt64Value:
                if(!checkInterfaceCompatible({FT::kInt64Type}, {RVC::kReplaceInt64Value})) {
                        return result;
                    }
                break;
            case Specification::Imputer::kImputedDoubleArray:
                if(!checkInterfaceCompatible({FT::kMultiArrayType}, {RVC::kReplaceDoubleValue})) {
                    return result;
                }

                    // Check to make sure the array sizes match
                if(input.type().multiarraytype().shape(0) != static_cast<int64_t>(imputer.imputeddoublearray().vector_size())) {
                    return  Result(ResultType::INVALID_MODEL_PARAMETERS,
                                   "Shape of imputed array value does not match shape of input array.");
                }

                break;
            case Specification::Imputer::kImputedStringValue:
                if(!checkInterfaceCompatible({FT::kStringType}, {RVC::kReplaceStringValue})) {
                    return result;
                }
                break;

            case Specification::Imputer::kImputedInt64Array:
                if(!checkInterfaceCompatible({FT::kMultiArrayType}, {RVC::kReplaceInt64Value})) {
                    return result;
                }

                    // Check to make sure the array sizes match
                if(input.type().multiarraytype().shape(0) != static_cast<int64_t>(imputer.imputedint64array().vector_size())) {
                    return  Result(ResultType::INVALID_MODEL_PARAMETERS,
                                   "Shape of imputed array value does not match shape of input array.");
                }
                break;

            case Specification::Imputer::kImputedStringDictionary:
            case Specification::Imputer::kImputedInt64Dictionary:
                    // This only works with a full on ungiven replacement value.
                if(!checkInterfaceCompatible({FT::kDictionaryType}, {})) {
                    return result;
                }
                break;

            case Specification::Imputer::IMPUTEDVALUE_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Imputer parameter must be set.");
                break;
        }

        return result;
    }
}
