//
//  Validator.hpp
//  libmlmodelspec
//
//  Created by Srikrishna Sridhar on 11/10/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#ifndef Validator_h
#define Validator_h

#include "Format.hpp"
#include "Result.hpp"
#include "../build/format/Model_enums.h"

#include "ValidatorUtils-inl.hpp"

namespace CoreML {

    namespace Specification {
        class Model;
        class ModelDescription;
        class Metadata;
        class Kernel;
    }

    /*
     * Template specialization of validation of the protobuf.
     *
     * @param  format Model spec format.
     * @return Result type of this operation.
     */
    template <MLModelType T> Result validate(const Specification::Model& format);


    /*
     * Validate feature descriptions in interface have supported names and type info
     *
     * @param  interface Model interface
     # @param modelVersion The version of the model for backwards compatibility
     * @return Result type of this operation.
     */
    Result validateFeatureDescriptions(const Specification::ModelDescription& interface, int modelVersion);

    /*
     * Validate an individual feature description
     *
     * @param  feture description
     # @param modelVersion The version of the model for backwards compatibility
     * @return Result type of this operation.
     */
    Result validateFeatureDescription(const Specification::FeatureDescription& desc, int modelVersion, bool isInput = true);

    /*
     * Validate model interface describes a valid transform
     *
     * @param  interface Model interface
     # @param modelVersion The version of the model for backwards compatibility
     * @return Result type of this operation.
     */
    Result validateModelDescription(const Specification::ModelDescription& interface, int modelVersion);

    /*
     * Validate model interface describes a valid regressor
     *
     * @param  interface Model interface
     * @return Result type of this operation.
     */
    Result validateRegressorInterface(const Specification::ModelDescription& interface, int modelVersion);


    /*
     * Validate model interface describes a valid classifier
     *
     * @param  interface Model interface
     * @return Result type of this operation.
     */
    template<typename T, typename U>
    Result validateClassifierInterface(const T& model,
                                       const U& modelParameters) {

        bool expected_class_is_int64;

        // validate class labels
        switch (modelParameters.ClassLabels_case()) {
            case U::kInt64ClassLabels:
                if (modelParameters.int64classlabels().vector_size() == 0) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "Classifier declared to have Int64 class labels must provide labels.");
                }

                if(modelParameters.stringclasslabels().vector_size() != 0) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "Classifier declared with Int64 class labels must provide exclusively Int64 class labels.");
                }

                expected_class_is_int64 = true;

                break;

            case U::kStringClassLabels:
                if (modelParameters.stringclasslabels().vector_size() == 0) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "Classifier declared to have String class labels must provide labels.");
                }

                if(modelParameters.int64classlabels().vector_size() != 0) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                    "Classifier declared with String class labels must provide exclusively String class labels.");
                }

                expected_class_is_int64 = false;

                break;

            case U::CLASSLABELS_NOT_SET:
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Classifier models must provide class labels.");
        }
        const Specification::ModelDescription& interface = model.description();

            // Validate feature descriptions
        Result result = validateFeatureDescriptions(interface, model.specificationversion());
        if (!result.good()) {
            return result;
        }

        const auto& predictedFeatureName = interface.predictedfeaturename();
        const auto& probOutputName = interface.predictedprobabilitiesname();


        if (predictedFeatureName == "") {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Specification is missing classifier predictedFeatureName");
        } else {
            auto expected_class = (expected_class_is_int64
                                   ? Specification::FeatureType::TypeCase::kInt64Type
                                   : Specification::FeatureType::TypeCase::kStringType);

            result = validateDescriptionsContainFeatureWithNameAndType(interface.output(),
                                                                       predictedFeatureName,
                                                                       {expected_class});
            if (!result.good()) {
                return result;
            }
        }

        if (probOutputName != "") {
            // TODO @znation: validate array length below
            // and value type (must be double? different for different classifiers?)
            // TODO Probability outputs are always dictionaries!
            result = validateDescriptionsContainFeatureWithNameAndType(interface.output(),
                                                                       probOutputName,
                                                                       {Specification::FeatureType::TypeCase::kMultiArrayType, // TODO ARRAY TYPE IS INVALID, REMOVE
                                                                        Specification::FeatureType::TypeCase::kDictionaryType});
            if (!result.good()) {
                return result;
            }
        }

        return Result();
    }

    /*
     * Validate optional inputs/outputs.
     * For most models, optional is not allowed (all inputs/outputs required).
     * Some models have different behavior.
     */
    Result validateOptional(const Specification::Model& format);

}
#endif /* Validator_h */
