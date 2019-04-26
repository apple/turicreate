//
//  StandardScalarValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_scaler>(const Specification::Model& format) {
        const auto& description = format.description();

            // Convenience typedefs
        typedef Specification::FeatureType FT;
        typedef Specification::Imputer::ReplaceValueCase RVC;

        Result result;

        // Validate its a MLModel type.
        result = validateModelDescription(description, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        // Validate the inputs
        result = validateDescriptionsContainFeatureWithTypes(description.input(), 1,
        {FT::kInt64Type, FT::kDoubleType, FT::kMultiArrayType});

        if (!result.good()) {
            return result;
        }

        // Validate the outputs
        result = validateDescriptionsContainFeatureWithTypes(description.output(), 1,
        {FT::kInt64Type, FT::kDoubleType, FT::kMultiArrayType});

        if (!result.good()) {
            return result;
        }

        // Make sure the input and output type match.
        // From the above, we know that we have exactly one input and one output type.
        const auto& input = description.input()[0];
        const auto& output = description.output()[0];

        if(input.type().Type_case() == FT::kInt64Type) {
            if((output.type().Type_case() != FT::kInt64Type)
               && (output.type().Type_case() != FT::kDoubleType)) {

                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Input type Int64 must output to Int64 or Double.");
            }
        } else if(output.type().Type_case() != input.type().Type_case()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Type of input feature does not match the output type feature.");
        }

            // If it's an array, we need to test sizes.
        if(input.type().Type_case() == FT::kMultiArrayType) {
            if(input.type().multiarraytype().shape_size() != 1) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Only 1 dimensional arrays input features are supported by the scaler.");
            }

            if(output.type().multiarraytype().shape_size() != 1
               || (input.type().multiarraytype().shape(0) != output.type().multiarraytype().shape(0))) {
                return  Result(ResultType::INVALID_MODEL_PARAMETERS,
                               "Shape of output array does not match shape of input array.");
            }

            // Now, make sure that the repeated values make sense.
            int64_t shift_size = static_cast<int64_t>(format.scaler().shiftvalue_size());

            if(!(shift_size == 0 || shift_size == 1
                 || shift_size == input.type().multiarraytype().shape(0))) {

                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "For input type array, specified shift values must be empty, a scalar, or a vector of the matching length.");
            }

                // Now, make sure that the repeated values make sense.
            int64_t scale_size = static_cast<int64_t>(format.scaler().scalevalue_size());

            if(!(scale_size == 0 || scale_size == 1
                 || scale_size == input.type().multiarraytype().shape(0))) {

                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "For input type array, specified scale values must be empty, a scalar, or a vector of the matching length.");
            }
        } else {
                // Now, make sure that the repeated values make sense.
            size_t shift_size = static_cast<size_t>(format.scaler().shiftvalue_size());

            if(!(shift_size == 0 || shift_size == 1)) {

                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "For a scalar imput type, specified shift value must be empty or a scalar.");
            }

                // Now, make sure that the repeated values make sense.
            size_t scale_size = static_cast<size_t>(format.scaler().scalevalue_size());

            if(!(scale_size == 0 || scale_size == 1)) {

                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "For input type array, specified scale values must be empty or a scalar.");
            }
        }

        return result;
    }
}
