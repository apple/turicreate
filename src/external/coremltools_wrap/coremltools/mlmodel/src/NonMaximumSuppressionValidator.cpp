//
//  NonMaximumSuppressionValidator
//  libmlmodelspec
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_nonMaximumSuppression>(const Specification::Model& format) {
        const auto& description = format.description();
        const auto& nms = format.nonmaximumsuppression();

        //const auto& _confidenceInputFeatureName = nms.confidenceinputfeaturename();
        const auto& inputs = description.input();
        const auto& outputs = description.output();

        // Convenience typedefs
        typedef Specification::FeatureType FT;
        typedef Specification::Imputer::ReplaceValueCase RVC;

        Result result;

        // Validate it's an MLModel type
        result = validateModelDescription(description, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        int confidenceInputIndex = -1;
        int coordinatesInputIndex = -1;
        int iouThresholdInputIndex = -1;
        int confidenceThresholdInputIndex = -1;

        // Make sure each input corresponds a feature name
        for (int i = 0; i < inputs.size(); ++i) {
            const std::string& name = inputs[i].name();
            bool used = false;

            if (nms.confidenceinputfeaturename().compare(name) == 0) {
                confidenceInputIndex = i;
                used = true;
            }

            if (nms.coordinatesinputfeaturename().compare(name) == 0) {
                coordinatesInputIndex = i;
                used = true;
            }

            if (nms.iouthresholdinputfeaturename().compare(name) == 0) {
                iouThresholdInputIndex = i;
                used = true;
            }

            if (nms.confidencethresholdinputfeaturename().compare(name) == 0) {
                confidenceThresholdInputIndex = i;
                used = true;
            }

            // The input was never used
            if (!used) {
                return Result(ResultType::TOO_MANY_FEATURES_FOR_MODEL_TYPE,
                              "Input feature '" + name + "' was not requested by any of the input feature names (e.g. confidenceInputFeatureName).");
            }
        }

        if (confidenceInputIndex == -1) {
            return Result(ResultType::INTERFACE_FEATURE_NAME_MISMATCH,
                          "Expected feature '" + nms.confidenceinputfeaturename() + "' (as defined by confidenceInputFeatureName) to the model is not present in the model description.");
        }

        if (coordinatesInputIndex == -1) {
            return Result(ResultType::INTERFACE_FEATURE_NAME_MISMATCH,
                          "Expected feature '" + nms.coordinatesinputfeaturename() + "' (as defined by coordinatesInputFeatureName) to the model is not present in the model description.");
        }

        int confidenceOutputIndex = -1;
        int coordinatesOutputIndex = -1;

        for (int i = 0; i < outputs.size(); ++i) {
            const std::string& name = outputs[i].name();
            bool used = false;

            if (nms.confidenceoutputfeaturename().compare(name) == 0) {
                confidenceOutputIndex = i;
                used = true;
            }

            if (nms.coordinatesoutputfeaturename().compare(name) == 0) {
                coordinatesOutputIndex = i;
                used = true;
            }

            // The output was never used
            if (!used) {
                return Result(ResultType::TOO_MANY_FEATURES_FOR_MODEL_TYPE,
                              "Output feature '" + name + "' was not requested by any of the output feature names (e.g. confidenceOutputFeatureName).");
            }
        }

        if (confidenceOutputIndex == -1) {
            return Result(ResultType::INTERFACE_FEATURE_NAME_MISMATCH,
                          "Expected feature '" + nms.confidenceoutputfeaturename() + "' (as defined by confidenceOutputFeatureName) from the model is not present in the model description.");
        }

        if (coordinatesOutputIndex == -1) {
            return Result(ResultType::INTERFACE_FEATURE_NAME_MISMATCH,
                          "Expected feature '" + nms.coordinatesoutputfeaturename() + "' (as defined by coordinatesOutputFeatureName) from the model is not present in the model description.");
        }

        // Validate the inputs
        result = validateDescriptionsContainFeatureWithNameAndType(description.input(),
                                                                   nms.confidenceinputfeaturename(),
                                                                   {FT::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        result = validateDescriptionsContainFeatureWithNameAndType(description.input(),
                                                                   nms.coordinatesinputfeaturename(),
                                                                   {FT::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        // Validate the optional inputs
        if (iouThresholdInputIndex >= 0) {
            result = validateDescriptionsContainFeatureWithNameAndType(description.input(),
                                                                       nms.iouthresholdinputfeaturename(),
                                                                       {FT::kDoubleType});
            if (!result.good()) {
                return result;
            }
        }


        if (confidenceThresholdInputIndex >= 0) {
            result = validateDescriptionsContainFeatureWithNameAndType(description.input(),
                                                                       nms.confidencethresholdinputfeaturename(),
                                                                       {FT::kDoubleType});
            if (!result.good()) {
                return result;
            }
        }

        // Validate the outputs
        result = validateDescriptionsContainFeatureWithNameAndType(description.output(),
                                                                   nms.confidenceoutputfeaturename(),
                                                                   {FT::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        result = validateDescriptionsContainFeatureWithNameAndType(description.output(),
                                                                   nms.coordinatesoutputfeaturename(),
                                                                   {FT::kMultiArrayType});
        if (!result.good()) {
            return result;
        }

        if (!(nms.iouthreshold() >= 0.0 && nms.iouthreshold() <= 1.0)) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "iouThreshold must be a value between 0.0 and 1.0.");

        }

        // Confidence can be greater than 1.0 if the user chooses to have non-probabilistic confidences.
        // However, since 0.0 is considered no confidence and used as a filler value when output shape
        // is larger than number of suppressed predictions, non-negative values are not allowed.
        if (!(nms.confidencethreshold() >= 0.0)) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "confidenceThreshold must be a non-negative value. If you do not want to eliminate any predictions based on confidence, set it to 0.0.");

        }

        // Make sure the input and output type match.
        // From the above, we know that we have exactly one input and one output type.
        const auto& confidence_in = description.input()[confidenceInputIndex];
        const auto& coordinates_in = description.input()[coordinatesInputIndex];
        const auto& confidence_out = description.output()[confidenceOutputIndex];
        const auto& coordinates_out = description.output()[coordinatesOutputIndex];

        if (confidence_in.type().multiarraytype().datatype() != Specification::ArrayFeatureType_ArrayDataType_DOUBLE) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Input confidence must have dataType DOUBLE");
        }

        if (confidence_out.type().multiarraytype().datatype() != Specification::ArrayFeatureType_ArrayDataType_DOUBLE) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Output confidence must have dataType DOUBLE");
        }

        if (coordinates_in.type().multiarraytype().datatype() != Specification::ArrayFeatureType_ArrayDataType_DOUBLE) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Input coordinates must have dataType DOUBLE");
        }

        if (coordinates_out.type().multiarraytype().datatype() != Specification::ArrayFeatureType_ArrayDataType_DOUBLE) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Output confidence must have dataType DOUBLE");
        }

        auto rankOfFlexibleShape = [] (const CoreML::Specification::ArrayFeatureType &marray) -> size_t {
            switch (marray.ShapeFlexibility_case()) {
                case CoreML::Specification::ArrayFeatureType::kEnumeratedShapes:
                    return marray.enumeratedshapes().shapes(0).shape_size();
                case CoreML::Specification::ArrayFeatureType::kShapeRange:
                    return marray.shaperange().sizeranges_size();
                case CoreML::Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                    return 0;
            }
        };

        auto hasFlexibleShape  = [] (const CoreML::Specification::ArrayFeatureType &marray) -> size_t {
            return marray.ShapeFlexibility_case() !=  CoreML::Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET;
        };

        auto hasEnumeratedShapes = [] (const CoreML::Specification::ArrayFeatureType &marray) -> size_t {
            return marray.ShapeFlexibility_case() ==  CoreML::Specification::ArrayFeatureType::kEnumeratedShapes;
        };

        auto hasShapeRange = [] (const CoreML::Specification::ArrayFeatureType &marray) -> size_t {
            return marray.ShapeFlexibility_case() ==  CoreML::Specification::ArrayFeatureType::kShapeRange;
        };


        size_t confOldShapeDim = confidence_out.type().multiarraytype().shape_size();
        size_t confNewShapeDim = rankOfFlexibleShape(confidence_out.type().multiarraytype());
        size_t coordsOldShapeDim = coordinates_out.type().multiarraytype().shape_size();
        size_t coordsNewShapeDim = rankOfFlexibleShape(coordinates_out.type().multiarraytype());

        // These three options are all allowed, anything else will fail
        bool confUnspecified = (confOldShapeDim == 0 && confNewShapeDim == 0);
        bool confUseOld = (confOldShapeDim == 2 && confNewShapeDim == 0);
        bool confUseNew = (confOldShapeDim == 0 && confNewShapeDim == 2);

        if (!(confUnspecified || confUseOld || confUseNew)) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "If shape information is provided for confidence output, 2 dimensions must be specified using either shape (deprecated) or allowedShapes.");
        }

        bool coordsUnspecified = (confOldShapeDim == 0 && confNewShapeDim == 0);
        bool coordsUseOld = (confOldShapeDim == 2 && confNewShapeDim == 0);
        bool coordsUseNew = (confOldShapeDim == 0 && confNewShapeDim == 2);

        if (!(coordsUnspecified || coordsUseOld || coordsUseNew)) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "If shape information is provided for coordinates output, 2 dimensions must be specified using either shape (deprecated) or allowedShapes.");
        }

        // For now, require consistent usage of new/old
        if ((confUseOld && !coordsUseOld) || (confUseNew && !coordsUseNew)) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Confidence and coordinates cannot use a mix of shape (deprecated) and allowedShapes.");
        }

        // If both use old shape, make sure shapes are consistent
        if (confUseOld && coordsUseOld &&
            confidence_out.type().multiarraytype().shape(0) != coordinates_out.type().multiarraytype().shape(0)) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Confidence and coordinates output shapes must be consistent (must have the same size along dimension 0).");
        }

        // If both use new shape, make sure shapes are consistent
        if (confUseNew && coordsUseNew) {
            bool confIsRange = hasShapeRange(confidence_out.type().multiarraytype());
            bool coordsIsRange = hasShapeRange(coordinates_out.type().multiarraytype());

            // Only allow range flexibility
            if (!confIsRange || !coordsIsRange) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Confidence and coordinates output shapes fexibility must both be ranges");
            }

            // Check they have the same range across dimension 0
            const auto &confRange = confidence_out.type().multiarraytype().shaperange().sizeranges();
            const auto &coordRange = confidence_out.type().multiarraytype().shaperange().sizeranges();

            if (confRange[0].lowerbound() != coordRange[0].lowerbound() ||
                confRange[0].upperbound() != coordRange[0].upperbound()) {

                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              "Confidence and coordinates output shapes must be consistent (must have the same range of sizes along dimension 0).");
            }

        }

        // If shape constraints on number of classes are defined, it must be consistent with
        // the number of labels.
        int numLabelNames;
        switch (nms.ClassLabels_case()) {
            case Specification::NonMaximumSuppression::kInt64ClassLabels:
                numLabelNames = nms.int64classlabels().vector_size();
                break;
            case Specification::NonMaximumSuppression::kStringClassLabels:
                numLabelNames = nms.stringclasslabels().vector_size();
                break;
            case Specification::NonMaximumSuppression::CLASSLABELS_NOT_SET:
                numLabelNames = 0;
                break;
        }

        if (confUseOld && numLabelNames > 0 &&
            confidence_out.type().multiarraytype().shape(1) != numLabelNames) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Number of classes is not consistent for class labels (" + std::to_string(numLabelNames) + ") and dimension 1 of output confidence shape (" + std::to_string(confidence_out.type().multiarraytype().shape(1)) + ")");
        }

        if (confUseNew && numLabelNames > 0) {
            // Already checked confUseNew is a range above
            // Just check the range is correct

            const auto &confRange = confidence_out.type().multiarraytype().shaperange().sizeranges();

            if ((numLabelNames < confRange[1].lowerbound() ||
                  (confRange[1].upperbound() >= 0  && numLabelNames > confRange[1].upperbound()))) {
                      return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                    "Number of classes is not consistent for class labels (" + std::to_string(numLabelNames) + ") and dimension 1 of output confidence shape range");
            }
        }

        return result;
    }
}
