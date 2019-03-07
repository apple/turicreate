//
//  Pipeline.cpp
//  libmlmodelspec
//
//  Created by Hoyt Koepke on 11/17/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "Pipeline.hpp"
#include "../Format.hpp"

namespace CoreML {

    Pipeline::Pipeline(const std::string& description)
    : Model(description) {
        m_spec->mutable_pipeline();
    }

    Pipeline::Pipeline(const std::string& a, const std::string& b, const std::string& description, bool isClassifier)
    : Model(description)
    {
        auto* params = m_spec->mutable_description();
        params->set_predictedfeaturename(a);
        if (isClassifier) {
            params->set_predictedprobabilitiesname(b);
            m_spec->mutable_pipelineclassifier();
        } else {
            m_spec->mutable_pipelineregressor();
        }
    }


    Pipeline Pipeline::Regressor(const std::string& predictedValueOutputName,
                                 const std::string& description) {
        return Pipeline(predictedValueOutputName,
                        "",
                        description,
                        false /* isClassifier */);
    }

    Pipeline Pipeline::Classifier(const std::string& predictedClassOutputName,
                                  const std::string& probabilityOutputName,
                                  const std::string& description) {
        return Pipeline(predictedClassOutputName,
                        probabilityOutputName,
                        description,
                        true /* isClassifier */);
    }

    Pipeline Pipeline::Transformer(const std::string& description) {
        // Just a transformer.
        return Pipeline(description);
    }

    Pipeline::~Pipeline() = default;

    Pipeline::Pipeline(const Specification::Model &modelSpec) {
        m_spec = std::make_shared<Specification::Model>(modelSpec);
    }

    Result Pipeline::add(const Model& spec) {
        google::protobuf::RepeatedPtrField< ::CoreML::Specification::Model >* container = nullptr;
        switch (m_spec->Type_case()) {
            case Specification::Model::kPipeline:
                container = m_spec->mutable_pipeline()->mutable_models();
                break;
            case Specification::Model::kPipelineRegressor:
                container = m_spec->mutable_pipelineregressor()->mutable_pipeline()->mutable_models();
                break;
            case Specification::Model::kPipelineClassifier:
                container = m_spec->mutable_pipelineclassifier()->mutable_pipeline()->mutable_models();
                break;
            default:
                assert(false);
                break;
        }

        auto* contained = container->Add();
        *contained = spec.getProto();
        return Result();
    }

    std::vector<Model> Pipeline::getPipeline() const {
        auto p = m_spec->pipeline();

        std::vector<Model> out;
        auto container = p.models();

        int size = container.size();
        assert(size >= 0);
        out.reserve(static_cast<size_t>(size));

        for(auto model : container) {
            out.push_back(Model(model));
        }

        return out;
    }
}
