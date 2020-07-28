//
//  PipelineValidator.cpp
//  libmlmodelspec
//
//  Created by Zach Nation on 12/21/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "../Utils.hpp"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"

#include <string>
#include <unordered_map>

namespace CoreML {

    static Result validate(const Specification::Model& spec, const Specification::Pipeline& pipelineParams) {
        const int nModels = pipelineParams.models_size();
        if (nModels == 0) {
            // empty chain is not allowed
            return Result(ResultType::INVALID_MODEL_PARAMETERS, "Pipeline must contain one or more models.");
        }

        // build a type table from the list of models
        std::unordered_map<std::string, const CoreML::Specification::FeatureDescription*> typeTable;

        // First, populate the type table with the given inputs from the pipeline
        for(const auto& input : spec.description().input() ) {
            typeTable[input.name()] = &input;
        }

        // iterate over models and check the types at each one, making sure they match up with previous versions.
        for (const auto& model : pipelineParams.models()) {
            const auto& inputs = model.description().input();

            // validate current transform against previously known valid state
            for (const CoreML::Specification::FeatureDescription& arg : inputs) {

                auto it = typeTable.find(arg.name());

                if(it == typeTable.end()) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  ("Pipeline: the input '" + arg.name() + "' of model '" + model.description().GetTypeName()
                                   + "' does not present in pipeline input or previous model."));
                }

                if(!CoreML::Specification::isEquivalent(arg, *(it->second))) {
                    return Result(ResultType::TYPE_MISMATCH,
                                  ("Pipeline: the input '" + arg.name() + "' of model '" + model.description().GetTypeName()
                                   + "' does not match the type previously specified by the pipeline input or the output of a previous model."
                                   + " For the second case, make sure the input and previous model's output has the matching name and shapes."));
                }
            }

            // validate the model itself and bail out if it's invalid
            Result r = Model::validate(model);
            if (!r.good()) { return r; }

            // Now add in the outputs of this model to the mix.
            const auto& outputs = model.description().output();
            for (const CoreML::Specification::FeatureDescription& arg : outputs) {
                typeTable[arg.name()] = &arg;
            }
        }

        // Finally, validate that the outputs of the pipeline model match what is outputted by the models.
        for(const auto& output : spec.description().output() ) {
            auto it = typeTable.find(output.name());

            if(it == typeTable.end()) {

                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              ("Pipeline output '" + output.name() + "' not present in pipeline input or a contained model."));
            }

            if(!CoreML::Specification::isEquivalent(output, *(it->second))) {
                return Result(ResultType::TYPE_MISMATCH,
                              ("Type of pipeline output '" + output.name() + "' does not match type produced in pipeline input."));
            }
        }
        
        if (spec.isupdatable()) {
            for (int modelIdx = 0; modelIdx < nModels - 1; modelIdx++) {
                auto model = pipelineParams.models(modelIdx);
                if (model.isupdatable()) {
                    return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION,
                                  ("Only the last model in the pipeline can be updatable. Model at position '" + std::to_string(modelIdx) + "' is marked as updatable."));
                }
            }
            if (false == pipelineParams.models(nModels - 1).isupdatable()) {
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION,
                              ("Last model in an updatable pipeline model should be marked as updatable."));
            }
        } else {
            for (int modelIdx = 0; modelIdx < nModels; modelIdx++) {
                auto model = pipelineParams.models(modelIdx);
                if (model.isupdatable()) {
                    return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION,
                                  ("Found an updatable model at '" + std::to_string(modelIdx) + "' inside a non-updatable pipeline."));
                }
            }
        }

        const int nNames = pipelineParams.names_size();
        if (nNames > 0) {
            if (nNames != nModels) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS,
                              ("The number of pipeline model names '" + std::to_string(nNames) +
                               "' doesn't match the number of models '" + std::to_string(nModels) + "'"));
            }
            std::set<std::string> names;
            for (int modelIdx = 0; modelIdx < nNames; modelIdx++) {
                auto modelName = pipelineParams.names(modelIdx);
                if (names.find(modelName) != names.end()) {
                    return Result(ResultType::INVALID_MODEL_PARAMETERS,
                                  ("Pipeline model name '" + modelName +
                                   "' at index '" +std::to_string(modelIdx) + " has already been used for previous models"));
                }
                names.insert(modelName);
            }

        }
        // if we get here, no input of any model caused a type mismatch with
        // any other prior model in the chain, or had an independent validation
        // error on its own.
        return Result();
    }

    template <>
    Result validate<MLModelType_pipelineClassifier>(const Specification::Model& pipeline) {
        const auto& pipelineParams = pipeline.pipelineclassifier().pipeline();
        // TODO -- validate classifier interface
        return validate(pipeline, pipelineParams);
    }

    template <>
    Result validate<MLModelType_pipelineRegressor>(const Specification::Model& pipeline) {
        const auto& pipelineParams = pipeline.pipelineregressor().pipeline();
        // TODO -- validate regressor interface
        return validate(pipeline, pipelineParams);
    }

    template <>
    Result validate<MLModelType_pipeline>(const Specification::Model& pipeline) {
        return validate(pipeline, pipeline.pipeline());
    }

}
