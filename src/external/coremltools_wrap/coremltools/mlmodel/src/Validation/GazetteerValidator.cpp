//
//  GazetteerValidator.cpp
//  mlmodel
//
//  Created by scp on 7/1/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#include "../Result.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "../../build/format/Model.pb.h"

namespace CoreML {
    
    template <>
    Result validate<MLModelType_gazetteer>(const Specification::Model& format) {
        const auto& interface = format.description();
        
        // make sure model is a gazetteer
        if (!format.has_gazetteer()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model not a gazetteer.");
        }
        
        Result result;
        
        // Validate the inputs: only one input with string type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {Specification::FeatureType::kStringType});
        if (!result.good()) {
            return result;
        }
        
        // Validate the output: only one output feature with string type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kStringType});
        if (!result.good()) {
            return result;
        }
        
        // Validate the model parameters
        const auto &gazetteer = format.gazetteer();
        if (gazetteer.revision() < 2) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model revision number missing or invalid. Must be >= 2");
        }
        
        int numClassLabels;
        switch (gazetteer.ClassLabels_case()) {
            case Specification::CoreMLModels::Gazetteer::kStringClassLabels:
                numClassLabels = gazetteer.stringclasslabels().vector_size();
                break;
            case Specification::CoreMLModels::Gazetteer::CLASSLABELS_NOT_SET:
                numClassLabels = -1;
                break;
        }
        
        if (numClassLabels <= 0) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model output class label not set. Must have at least one class label");
        }
        
        if (gazetteer.modelparameterdata().empty()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model parameter data not set");
        }
        
        return result;
    }
    
}
