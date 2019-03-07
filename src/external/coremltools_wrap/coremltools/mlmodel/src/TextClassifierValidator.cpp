//
//  TextClassifierValidator.cpp
//  mlmodel
//
//  Created by Tao Jia on 3/21/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#include "Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../build/format/Model.pb.h"

namespace CoreML {

    template <>
    Result validate<MLModelType_textClassifier>(const Specification::Model& format) {
        const auto& interface = format.description();

        // make sure model is a word tager
        if (!format.has_textclassifier()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model not a text classifier.");
        }

        Result result;

        // Validate the inputs: only one input with string type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {Specification::FeatureType::kStringType});
        if (!result.good()) {
            return result;
        }

        // Validate the outputs: only three outputs with sequence type are allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kStringType});
        if (!result.good()) {
            return result;
        }

        // Validate the model parameters
        const auto &textClassifier = format.textclassifier();
        if (textClassifier.revision() == 0) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model revision number not set. Must be >= 1");
        }

        int numClassLabels;
        switch (textClassifier.ClassLabels_case()) {
            case Specification::CoreMLModels::TextClassifier::kStringClassLabels:
                numClassLabels = textClassifier.stringclasslabels().vector_size();
                break;
            case Specification::CoreMLModels::TextClassifier::CLASSLABELS_NOT_SET:
                numClassLabels = -1;
                break;
        }

        if (numClassLabels <= 0) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model output class label not set. Must have at least one class label");
        }

        if (textClassifier.modelparameterdata().empty()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model parameter data not set");
        }

        return result;
    }

}
