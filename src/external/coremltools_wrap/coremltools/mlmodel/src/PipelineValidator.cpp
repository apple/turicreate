//
//  PipelineValidator.cpp
//  libmlmodelspec
//
//  Created by Zach Nation on 12/21/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "Utils.hpp"
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
                                  ("Pipeline: Input '" + arg.name() + "' of model '" + model.description().GetTypeName()
                                   + "' not present in pipeline input or previous model."));
                }

                if(!CoreML::Specification::isEquivalent(arg, *(it->second))) {
                    return Result(ResultType::TYPE_MISMATCH,
                                  ("Pipeline: Input '" + arg.name() + "' of model '" + model.description().GetTypeName()
                                   + "' does not match the type previously specified by the pipeline input or the output of a previous model."));
                }
            }

            // validate the model itself and bail out if it's invalid
            Model wrapper(model);
            Result r = wrapper.validate();
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
