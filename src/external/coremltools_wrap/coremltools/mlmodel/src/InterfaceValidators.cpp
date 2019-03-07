//
//  InterfaceValidators.cpp
//  libmlmodelspec
//
//  Created by Michael Siracusa on 12/8/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"
#include "Globals.hpp"

namespace CoreML {



    Result validateSizeRange(const Specification::SizeRange &range) {
        if (range.upperbound() > 0 && range.lowerbound() > static_cast<unsigned long long>(range.upperbound())) {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Size range is invalid (" + std::to_string(range.lowerbound()) + ", " + std::to_string(range.upperbound()) + " ).");
        }
        return Result();
    }

    Result validateFeatureDescription(const Specification::FeatureDescription& desc, int modelVersion, bool isInput) {
        if (desc.name() == "") {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Feature description must have a non-empty name.");
        }

        if (!desc.has_type()) {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Feature description " + desc.name() + " must specify a valid feature type.");
        }

        const auto& type = desc.type();
        switch (type.Type_case()) {
            case Specification::FeatureType::kDoubleType:
            case Specification::FeatureType::kInt64Type:
            case Specification::FeatureType::kStringType:
                // in general, non-parametric types need no further validation
                break;

            case Specification::FeatureType::kMultiArrayType:
            {
                const auto &defaultShape = type.multiarraytype().shape();
                bool hasExplicitDefault = (type.multiarraytype().shape_size() != 0);
                bool hasImplictDefault = false;
                // Newer versions use the updated shape constraints for images and multi-arrays
                if (modelVersion >= MLMODEL_SPECIFICATION_VERSION_IOS12) {

                    switch (type.multiarraytype().ShapeFlexibility_case()) {

                        case Specification::ArrayFeatureType::kEnumeratedShapes: {

                            hasImplictDefault = true;

                            if (type.multiarraytype().enumeratedshapes().shapes_size() == 0) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of multiarray feature '" + desc.name() + "' has enumerated zero permitted sizes.");
                            }

                            for (auto &shape : type.multiarraytype().enumeratedshapes().shapes()) {
                                if (shape.shape_size() == 0) {
                                    return Result(ResultType::INVALID_MODEL_INTERFACE,
                                                  "Description of multiarray feature '" + desc.name() + "' has enumerated shapes with zero dimensions.");
                                }
                            }

                            if (!hasExplicitDefault) {
                                break;
                            }

                            bool foundDefault = false;
                            for (auto &shape : type.multiarraytype().enumeratedshapes().shapes()) {
                                if (shape.shape_size() != defaultShape.size()) { continue; }
                                foundDefault = true;
                                for (int d = 0; d < shape.shape_size(); d++) {
                                    if (defaultShape[d] != shape.shape(d)) { foundDefault = false; break; }
                                }
                                if (foundDefault) { break; }
                            }

                            if (!foundDefault) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of multiarray feature '" + desc.name() + "' has a default shape specified " +
                                              " which is not within the allowed enumerated shapes specified.");
                            }

                            break;
                        }
                        case Specification::ArrayFeatureType::kShapeRange: {

                            hasImplictDefault = true;

                            const auto& sizeRanges = type.multiarraytype().shaperange().sizeranges();
                            for (int i = 0; i < sizeRanges.size(); i++) {
                                const auto &range = sizeRanges[i];
                                Result res = validateSizeRange(range);
                                if (!res.good()) {
                                    return Result(ResultType::INVALID_MODEL_INTERFACE,
                                                  "Description of multiarray feature '" + desc.name() +
                                                  "' has an invalid range for dimension " + std::to_string(i) + ". " +
                                                  res.message());
                                }
                            }

                            if (!hasExplicitDefault) {
                                break;
                            }

                            // Check if default is compatible
                            if (defaultShape.size() != sizeRanges.size()) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of multiarray feature '" + desc.name() +
                                              "' has a default " + std::to_string(defaultShape.size()) + "-d shape but a " +
                                              std::to_string(sizeRanges.size()) + "-d shape range");
                            }

                             for (int i = 0; i < sizeRanges.size(); i++) {
                                 if (defaultShape[i] < (int)sizeRanges[i].lowerbound() ||
                                     (sizeRanges[i].upperbound() >= 0 && defaultShape[i] > sizeRanges[i].upperbound())) {

                                     return Result(ResultType::INVALID_MODEL_INTERFACE,
                                                   "Description of multiarray feature '" + desc.name() +
                                                   "' has a default shape that is out of the specified shape range");
                                 }
                             }

                            break;
                        }
                        case Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                            break;
                    }

                }

                if (isInput && !hasExplicitDefault && !hasImplictDefault) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE,
                                  "Description of multiarray feature '" + desc.name() + "' has missing shape constraints.");
                }

                if (hasExplicitDefault) {
                    for (int i=0; i < type.multiarraytype().shape_size(); i++) {
                        const auto &value = type.multiarraytype().shape(i);
                        if (value < 0) {
                            return Result(ResultType::INVALID_MODEL_INTERFACE,
                                          "Description of multiarray feature '" + desc.name() + "' has an invalid shape. "
                                          "Element " + std::to_string(i) + " has non-positive value " + std::to_string(value) + ".");
                        }
                    }
                }

                switch (type.multiarraytype().datatype()) {
                    case Specification::ArrayFeatureType_ArrayDataType_DOUBLE:
                    case Specification::ArrayFeatureType_ArrayDataType_FLOAT32:
                    case Specification::ArrayFeatureType_ArrayDataType_INT32:
                        break;
                    default:
                        return Result(ResultType::INVALID_MODEL_INTERFACE,
                                      "Description of multiarray feature '" + desc.name() + "' has an invalid or unspecified dataType. "
                                      "It must be specified as DOUBLE, FLOAT32 or INT32");
                }
                break;

            }
            case Specification::FeatureType::kDictionaryType:
                switch (type.dictionarytype().KeyType_case()) {
                    case Specification::DictionaryFeatureType::KeyTypeCase::kInt64KeyType:
                    case Specification::DictionaryFeatureType::KeyTypeCase::kStringKeyType:
                        break;
                    case Specification::DictionaryFeatureType::KeyTypeCase::KEYTYPE_NOT_SET:
                        return Result(ResultType::INVALID_MODEL_INTERFACE,
                                      "Description of dictionary feature '" + desc.name() + "' must contain a key type of either Int64 or String.");
                }
                break;

            case Specification::FeatureType::kImageType: {

                int64_t defaultWidth = type.imagetype().width();
                int64_t defaultHeight = type.imagetype().height();
                bool hasDefault = (defaultWidth > 0 && defaultHeight > 0);

                if (modelVersion >= MLMODEL_SPECIFICATION_VERSION_IOS12) {

                    switch (type.imagetype().SizeFlexibility_case()) {

                        case Specification::ImageFeatureType::kEnumeratedSizes: {

                            if (type.imagetype().enumeratedsizes().sizes_size() == 0) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of image feature '" + desc.name() + "' has enumerated zero permitted sizes.");
                            }

                            if (!hasDefault) {
                                defaultWidth = (int64_t)type.imagetype().enumeratedsizes().sizes(0).width();
                                defaultHeight = (int64_t)type.imagetype().enumeratedsizes().sizes(0).height();
                                hasDefault = true;
                                break;
                            }

                            bool foundDefault = false;
                            for (auto &size : type.imagetype().enumeratedsizes().sizes()) {
                                if (defaultWidth == (int64_t)size.width() && defaultHeight == (int64_t)size.height()) {
                                    foundDefault = true;
                                    break;
                                }
                            }

                            if (!foundDefault) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of image feature '" + desc.name() + "' has a default size of " +
                                              std::to_string(defaultWidth) + " x " + std::to_string(defaultHeight) +
                                              " which is not within the allowed enumerated sizes specified.");
                            }

                            break;
                        }
                        case Specification::ImageFeatureType::kImageSizeRange:
                        {
                            const auto& widthRange = type.imagetype().imagesizerange().widthrange();
                            Result res = validateSizeRange(widthRange);
                            if (!res.good()) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of image feature '" + desc.name() + "' has an invalid flexible width range. "
                                               + res.message());
                            }

                            const auto& heightRange = type.imagetype().imagesizerange().heightrange();
                            res = validateSizeRange(heightRange);
                            if (!res.good()) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of image feature '" + desc.name() + "' has an invalid flexible height range. "
                                              + res.message());
                            }

                            if (!hasDefault) {
                                defaultWidth = (int64_t)widthRange.lowerbound();
                                defaultHeight = (int64_t)heightRange.lowerbound();
                                hasDefault = true;
                                break;
                            }

                            if (defaultWidth < (int64_t)widthRange.lowerbound() ||
                                (widthRange.upperbound() >=0 && defaultWidth > widthRange.upperbound())) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of image feature '" + desc.name() + "' default width "
                                              + std::to_string(defaultWidth) + " is not within specified flexible width range");
                            }

                            if (defaultHeight < (int64_t)heightRange.lowerbound() ||
                                (heightRange.upperbound() >=0 && defaultHeight > heightRange.upperbound())) {
                                return Result(ResultType::INVALID_MODEL_INTERFACE,
                                              "Description of image feature '" + desc.name() + "' default height "
                                              + std::to_string(defaultHeight) + " is not within specified flexible height range");
                            }


                            break;
                        }
                        default:
                            break;
                    }

                }

                if (defaultWidth <= 0) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE,
                                  "Description of image feature '" + desc.name() +
                                  "' has missing or non-positive width " + std::to_string(type.imagetype().width()) + ".");
                }

                if (defaultHeight <= 0) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE,
                                  "Description of image feature '" + desc.name() +
                                  "' has missing or non-positive height " + std::to_string(type.imagetype().height()) + ".");
                }

                switch (type.imagetype().colorspace()) {
                    case Specification::ImageFeatureType_ColorSpace_GRAYSCALE:
                    case Specification::ImageFeatureType_ColorSpace_RGB:
                    case Specification::ImageFeatureType_ColorSpace_BGR:
                        break;
                    default:
                        return Result(ResultType::INVALID_MODEL_INTERFACE,
                                      "Description of image feature '" + desc.name() +
                                      "' has missing or invalid colorspace. It must be RGB, BGR or GRAYSCALE.");
                }
                break;
            }
            case Specification::FeatureType::kSequenceType: {

                if (modelVersion < MLMODEL_SPECIFICATION_VERSION_IOS12) {
                    return  Result(ResultType::INVALID_MODEL_INTERFACE,
                                   "Sequence types are only valid in specification verison >= " + std::to_string(MLMODEL_SPECIFICATION_VERSION_IOS12)+
                                   ". This model has version " + std::to_string(modelVersion));
                }

                // Validate size
                Result res = validateSizeRange(type.sequencetype().sizerange());
                if (!res.good()) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE,
                                  "Description of sequence feature '" + desc.name() + "' has invalid allowed sizes. "
                                  + res.message());
                }

                // Validate type

                switch (type.sequencetype().Type_case()) {
                    case Specification::SequenceFeatureType::kInt64Type:
                    case Specification::SequenceFeatureType::kStringType:
                        break;
                    case Specification::SequenceFeatureType::TYPE_NOT_SET:
                        return Result(ResultType::INVALID_MODEL_INTERFACE,
                                      "Description of sequence feature '" + desc.name() + "' has invalid or missing type. "
                                      "Only Int64 and String sequences are currently supported");


                }
                break;
            }
            case Specification::FeatureType::TYPE_NOT_SET:
                // these aren't equal to anything, even themselves
                return Result(ResultType::INVALID_MODEL_INTERFACE,
                              "Feature description has an unspecified or invalid type for feature '" + desc.name() + "'.");
        }

        // If we got here, the feature description is valid.
        return Result();
    }

    Result validateFeatureDescriptions(const Specification::ModelDescription& interface, int modelVersion) {
        // a model must have at least one input and one output
        if (interface.input_size() < 1) {
            return Result(ResultType::INVALID_MODEL_INTERFACE, "Models must have one or more inputs.");
        }
        if (interface.output_size() < 1) {
            return Result(ResultType::INVALID_MODEL_INTERFACE, "Models must have one or more outputs.");
        }

        for (const auto& input : interface.input()) {
            Result r = validateFeatureDescription(input, modelVersion, true);
            if (!r.good()) { return r; }
        }

        for (const auto& output : interface.output()) {
            Result r = validateFeatureDescription(output, modelVersion, false);
            if (!r.good()) { return r; }
        }

        // If we got here, all inputs/outputs seem good independently of each other.
        return Result();
    }

    Result validateModelDescription(const Specification::ModelDescription& interface, int modelVersion) {

        Result result = validateFeatureDescriptions(interface, modelVersion);
        if (!result.good()) {
            return result;
        }

        return result;

    }


    Result validateRegressorInterface(const Specification::ModelDescription& description, int modelVersion) {

        if (description.predictedfeaturename() == "") {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Specification is missing regressor predictedFeatureName.");
        }

        // Validate feature descriptions
        Result result = validateFeatureDescriptions(description, modelVersion);
        if (!result.good()) {
            return result;
        }

        result = validateDescriptionsContainFeatureWithNameAndType(description.output(),
                                                                   description.predictedfeaturename(),
                                                                   {Specification::FeatureType::kDoubleType, Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }
        return result;
    }

    /*
     * Validate optional inputs/outputs.
     * For most models, optional is not allowed (all inputs/outputs required).
     * Some will template-specialize to override.
     */
    inline Result validateOptionalOutputs(const Specification::ModelDescription& interface) {
        for (const auto& output : interface.output()) {
            if (output.type().isoptional()) {
                return Result(ResultType::INVALID_MODEL_INTERFACE, "Outputs cannot be optional.");
            }
        }
        return Result();
    }

    static Result validateOptionalGeneric(const Specification::ModelDescription& interface) {
        for (const auto& input : interface.input()) {
            if (input.type().isoptional()) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Features cannot be optional to this type of model.");
            }
        }
        return validateOptionalOutputs(interface);
    }

    inline Result validateOptionalTree(const Specification::ModelDescription& interface) {
        return validateOptionalOutputs(interface);
    }

    inline Result validateOptionalNN(const Specification::ModelDescription& description) {
        // just need to check that not all inputs are optional
        bool hasNotOptional = false;
        for (const auto& input : description.input()) {
            if (!input.type().isoptional()) {
                hasNotOptional = true;
                break;
            }
        }
        if (!hasNotOptional) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "At least one feature for a neural network must NOT be optional.");
        }
        return Result();
    }

    Result validateOptional(const Specification::Model& format) {
        Result r;
        switch (format.Type_case()) {
            case Specification::Model::kImputer:
                // Imputed values can be handled by replacing a particular value, so
                // optional is not required.
                break;
            case Specification::Model::kNeuralNetwork:
            case Specification::Model::kNeuralNetworkRegressor:
            case Specification::Model::kNeuralNetworkClassifier:
                r = validateOptionalNN(format.description());
                break;
            case Specification::Model::kTreeEnsembleRegressor:
            case Specification::Model::kTreeEnsembleClassifier:
                // allow arbitrary optional in tree inputs, just check outputs
                break;
            case Specification::Model::kPipeline:
            case Specification::Model::kPipelineRegressor:
            case Specification::Model::kPipelineClassifier:
                // pipeline has valid optional inputs iff the models inside are valid.
                // this should be guaranteed by the pipeline validator.
                break;
            case Specification::Model::kIdentity:
                // anything goes for the identity function
                break;
            default:
                r = validateOptionalGeneric(format.description());
        }
        if (!r.good()) {
            return r;
        }

        return validateOptionalOutputs(format.description());
    }

}
