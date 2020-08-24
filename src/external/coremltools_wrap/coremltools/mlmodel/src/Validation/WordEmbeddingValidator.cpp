//
//  WordEmbeddingValidator.cpp
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
    Result validate<MLModelType_wordEmbedding>(const Specification::Model& format) {
        const auto& interface = format.description();
        
        // make sure model is a word embedding
        if (!format.has_wordembedding()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model not a word embedding.");
        }
        
        Result result;
        
        // Validate the inputs: only one input with string type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.input(), 1, {Specification::FeatureType::kStringType});
        if (!result.good()) {
            return result;
        }
        
        // Validate the output: only one output with double vector (1d multiArray) type is allowed
        result = validateDescriptionsContainFeatureWithTypes(interface.output(), 1, {Specification::FeatureType::kMultiArrayType});
        if (!result.good()) {
            return result;
        }
        
        // Validate the model parameters
        const auto &wordEmbedding = format.wordembedding();
        if (wordEmbedding.revision() < 2) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model revision number missing or invalid. Must be >= 2");
        }
        
        if (wordEmbedding.modelparameterdata().empty()) {
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Model parameter data not set");
        }
        
        return result;
    }
    
}
