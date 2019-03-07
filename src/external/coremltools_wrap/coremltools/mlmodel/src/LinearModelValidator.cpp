//
//  LinearRegressionValidator.hpp
//  libmlmodelspec
//
//  Created by Srikrishna Sridhar on 11/10/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {


    template <>
    Result validate<MLModelType_glmRegressor>(const Specification::Model& format) {
        const Specification::ModelDescription& interface = format.description();

        // Must have a regressor interface (since GLMRegressor is an MLRegressor)
        Result result = validateRegressorInterface(interface, format.specificationversion());
        if (!result.good()) {
            return result;
        }

        // Check allowed output input & types are supported by this model.
        for (int i = 0; i < interface.input_size(); i++) {
            result = validateSchemaTypes({
                            Specification::FeatureType::kDoubleType,
                            Specification::FeatureType::kMultiArrayType,
                            Specification::FeatureType::kInt64Type
                      }, interface.input(i));
            if (!result.good()) {
                return result;
            }
        }
        for (int i = 0; i < interface.output_size(); i++) {
            result = validateSchemaTypes({
                Specification::FeatureType::kDoubleType,
            }, interface.output(i));

            if (!result.good()) {
                return result;
            }
        }

        // Linear regression specific type checking.
        // -------------------------------------------------------------------------
        auto lr = format.glmregressor();
        if (lr.weights_size() != lr.offset_size()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "Weights and offsets must be the same size.");
        }

        if (lr.weights_size() > 0) {
            int expected_size = lr.weights(0).value_size();
            for (int i = 1; i < lr.weights_size(); i++) {
                if (lr.weights(i).value_size() != expected_size) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "All weight coefficients must be the same size.");
                }
            }
        }
        return Result();
    }

    template <>
    Result validate<MLModelType_glmClassifier>(const Specification::Model& format) {

        // Check has a classifier interface
        Specification::ModelDescription interface = format.description();
        Result result = validateClassifierInterface(format, format.glmclassifier());
        if (!result.good()) {
            return result;
        }

        // Check that inputs are vectorizable
        result = validateDescriptionsAreAllVectorizableTypes(interface.input());
        if (!result.good()) {
            return result;
        }

        Specification::GLMClassifier glmClassifier = format.glmclassifier();

        // Check PostEvluationTransfrom and ClassEncoding have allowed values
        if(glmClassifier.postevaluationtransform() != Specification::GLMClassifier::Logit
           && glmClassifier.postevaluationtransform() != Specification::GLMClassifier::Probit) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Invalid post evaluation transform");
        }
        if(glmClassifier.classencoding() != Specification::GLMClassifier::ReferenceClass
           && glmClassifier.classencoding() != Specification::GLMClassifier::OneVsRest) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Invalid class encoding");
        }

        // Check has weights and same size as offsets
        int numCoefficient = glmClassifier.weights_size();
        if(numCoefficient == 0) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "The number of DoubleArrays in weights must be greater than zero");
        }
        if(numCoefficient != glmClassifier.offset_size()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "The number of DoubleArrays in weights must match number of offsets");
        }

        // Check number of weight vector makes sense based on the number of classes and the class encoding
        int numClasses;
        switch (glmClassifier.ClassLabels_case()) {
            case Specification::GLMClassifier::kInt64ClassLabels:
                numClasses = glmClassifier.int64classlabels().vector_size();
                break;
            case Specification::GLMClassifier::kStringClassLabels:
                numClasses = glmClassifier.stringclasslabels().vector_size();
                break;
            case Specification::GLMClassifier::CLASSLABELS_NOT_SET:
                numClasses = -1;
                break;
        }
        if(glmClassifier.classencoding() == Specification::GLMClassifier::ReferenceClass) {
            if (numClasses != -1 && numCoefficient != numClasses - 1) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                          "With ReferenceClass encoding the number of DoubleArrays in weights must be one less than number of classes");
            }
        } else { // One vs Rest encoding
            if(numClasses == 2) {
                if(numCoefficient != 1) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "When using OneVsRest encoding for only two classes, the number of DoubleArrays in weights must be one");
                }
            } else if (numClasses != -1 && numCoefficient != numClasses) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  "With OneVsRest encoding the number of DoubleArrays in weights must equal the number of classes");
            }
        }

        // Probit is only supported for binary classification
        if(numClasses > 2 && glmClassifier.postevaluationtransform() == Specification::GLMClassifier::Probit) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Probit post evaluation transform is only supported for binary classification");
        }

        // Check all DoubleArrays in weights have the same, nonzero length
        auto weights = glmClassifier.weights();
        if(weights[0].value_size() == 0) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Weight DoubleArrays must have nonzero length");
        }
        for(int i = 1; i < weights.size(); i++) {
            if(weights[0].value_size() != weights[i].value_size()) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Weight DoubleArrays must have the same length");
            }
        }

        return Result();
    }

}
