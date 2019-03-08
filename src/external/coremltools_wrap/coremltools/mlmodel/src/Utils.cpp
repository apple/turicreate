//
//  Utils.cpp
//  mlmodelspec
//
//  Created by Bill March on 10/3/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include "Utils.hpp"

using namespace CoreML;

// Returning a pointer here because of verification issues with allocating this type on the stack
google::protobuf::RepeatedPtrField<Specification::NeuralNetworkLayer> const *getNNSpec(const Specification::Model& model)  {
    switch (model.Type_case()) {
        case Specification::Model::TypeCase::kNeuralNetwork:
            return &(model.neuralnetwork().layers());
        case Specification::Model::TypeCase::kNeuralNetworkRegressor:
            return &(model.neuralnetworkregressor().layers());
        case Specification::Model::TypeCase::kNeuralNetworkClassifier:
            return &(model.neuralnetworkclassifier().layers());
        default:
//            assert(false);
            // Don't freak out about new, we don't really get here
            return NULL;
    }
}

// Helper functions for determining model version
bool CoreML::hasCustomLayer(const Specification::Model& model) {
    auto layers = getNNSpec(model);
    if (layers) {
        for (int i =0; i< layers->size(); i++){
            const Specification::NeuralNetworkLayer& layer = (*layers)[i];
            if (layer.layer_case() == Specification::NeuralNetworkLayer::kCustom) {
                return true;
            }
        }
    }
    return false;
}


inline void collectCustomLayerNamesAndDescriptions(const Specification::Model &model, std::vector<StringPair> *output) {

    switch (model.Type_case()) {
        case Specification::Model::kPipeline:
            for (auto &m : model.pipeline().models()) {
                collectCustomLayerNamesAndDescriptions(m,output);
            }
            break;
        case Specification::Model::kPipelineRegressor:
            for (auto &m : model.pipelineregressor().pipeline().models()) {
                collectCustomLayerNamesAndDescriptions(m,output);
            }
            break;
        case Specification::Model::kPipelineClassifier:
            for (auto &m : model.pipelineclassifier().pipeline().models()) {
                collectCustomLayerNamesAndDescriptions(m,output);
            }
            break;
        default:
            auto layers = getNNSpec(model);
            if (layers) {
                for (int i =0; i< layers->size(); i++){
                    const Specification::NeuralNetworkLayer& layer = (*layers)[i];
                    if (layer.layer_case() == Specification::NeuralNetworkLayer::kCustom) {
                        output->push_back(std::make_pair(layer.custom().classname(), layer.custom().description()));
                    }
                }
            }
            break;
        }
}

std::vector<StringPair> CoreML::getCustomLayerNamesAndDescriptions(const Specification::Model& model) {
    std::vector<std::pair<std::string, std::string> > retval;
    collectCustomLayerNamesAndDescriptions(model, &retval);
    return retval;
}

inline void collectCustomModelNamesAndDescriptions(const Specification::Model &model, std::vector<StringPair> *output) {

    switch (model.Type_case()) {
        case Specification::Model::kPipeline:
            for (auto &m : model.pipeline().models()) {
                collectCustomModelNamesAndDescriptions(m,output);
            }
            break;
        case Specification::Model::kPipelineRegressor:
            for (auto &m : model.pipelineregressor().pipeline().models()) {
                collectCustomModelNamesAndDescriptions(m,output);
            }
            break;
        case Specification::Model::kPipelineClassifier:
            for (auto &m : model.pipelineclassifier().pipeline().models()) {
                collectCustomModelNamesAndDescriptions(m,output);
            }
            break;
        case Specification::Model::kCustomModel:
            output->push_back(std::make_pair(model.custommodel().classname(),model.custommodel().description()));
            break;
        default:
            break;
    }
}

std::vector<std::pair<std::string, std::string> > CoreML::getCustomModelNamesAndDescriptions(const Specification::Model& model) {
    std::vector<std::pair<std::string, std::string> > retval;
    collectCustomModelNamesAndDescriptions(model,&retval);
    return retval;
}


void CoreML::downgradeSpecificationVersion(Specification::Model *pModel) {

    if (!pModel) { return; }

    if (pModel->specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS12 && !hasIOS12Features(*pModel)) {
        pModel->set_specificationversion(MLMODEL_SPECIFICATION_VERSION_IOS11_2);
    }

    if (pModel->specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS11_2 && !hasIOS11_2Features(*pModel)) {
        pModel->set_specificationversion(MLMODEL_SPECIFICATION_VERSION_IOS11);
    }

    ::CoreML::Specification::Pipeline *pipeline = NULL;
    auto modelType = pModel->Type_case();
    if (modelType == Specification::Model::kPipeline) {
        pipeline = pModel->mutable_pipeline();
    } else if (modelType == Specification::Model::kPipelineRegressor) {
        pipeline = pModel->mutable_pipelineregressor()->mutable_pipeline();
    } else if (modelType == Specification::Model::kPipelineClassifier) {
        pipeline = pModel->mutable_pipelineclassifier()->mutable_pipeline();
    }

    if (pipeline) {
        for (int i=0; i< pipeline->models_size(); i++) {
            downgradeSpecificationVersion(pipeline->mutable_models(i));
        }
    }

}

static inline bool isWeightParamOfType(const Specification::WeightParams &weight,
                                       const WeightParamType& type) {
    return valueType(weight) == type;
}

static bool hasLSTMWeightParamOfType(const Specification::LSTMWeightParams& params,
                                               const WeightParamType& type) {

    return (isWeightParamOfType(params.inputgateweightmatrix(), type) ||
            isWeightParamOfType(params.forgetgateweightmatrix(), type) ||
            isWeightParamOfType(params.blockinputweightmatrix(), type) ||
            isWeightParamOfType(params.outputgateweightmatrix(), type) ||

            isWeightParamOfType(params.inputgaterecursionmatrix(), type) ||
            isWeightParamOfType(params.forgetgaterecursionmatrix(), type) ||
            isWeightParamOfType(params.blockinputrecursionmatrix(), type) ||
            isWeightParamOfType(params.outputgaterecursionmatrix(), type) ||

            isWeightParamOfType(params.inputgatebiasvector(), type) ||
            isWeightParamOfType(params.forgetgatebiasvector(), type) ||
            isWeightParamOfType(params.blockinputbiasvector(), type) ||
            isWeightParamOfType(params.outputgatebiasvector(), type) ||

            isWeightParamOfType(params.inputgatepeepholevector(), type) ||
            isWeightParamOfType(params.forgetgatepeepholevector(), type) ||
            isWeightParamOfType(params.outputgatepeepholevector(), type));
}

bool CoreML::hasWeightOfType(const Specification::NeuralNetworkLayer& layer,
                             const WeightParamType& type) {

    switch (layer.layer_case()) {
        case Specification::NeuralNetworkLayer::LayerCase::kConvolution:
            return (isWeightParamOfType(layer.convolution().weights(),type) ||
                    isWeightParamOfType(layer.convolution().bias(), type));

        case Specification::NeuralNetworkLayer::LayerCase::kInnerProduct:
            return (isWeightParamOfType(layer.innerproduct().weights(),type) ||
                    isWeightParamOfType(layer.innerproduct().bias(), type));

        case Specification::NeuralNetworkLayer::LayerCase::kBatchnorm:
            return (isWeightParamOfType(layer.batchnorm().gamma(), type) ||
                    isWeightParamOfType(layer.batchnorm().beta(), type) ||
                    isWeightParamOfType(layer.batchnorm().mean(), type) ||
                    isWeightParamOfType(layer.batchnorm().variance(), type));

        case Specification::NeuralNetworkLayer::LayerCase::kLoadConstant:
            return isWeightParamOfType(layer.loadconstant().data(), type);

        case Specification::NeuralNetworkLayer::LayerCase::kScale:
            return (isWeightParamOfType(layer.scale().scale(), type) ||
                    isWeightParamOfType(layer.scale().bias(), type));

        case Specification::NeuralNetworkLayer::LayerCase::kSimpleRecurrent:
            return (isWeightParamOfType(layer.simplerecurrent().weightmatrix(), type) ||
                    isWeightParamOfType(layer.simplerecurrent().recursionmatrix(), type) ||
                    isWeightParamOfType(layer.simplerecurrent().biasvector(), type));

        case Specification::NeuralNetworkLayer::LayerCase::kGru:
            return (isWeightParamOfType(layer.gru().updategateweightmatrix(), type) ||
                    isWeightParamOfType(layer.gru().resetgateweightmatrix(), type) ||
                    isWeightParamOfType(layer.gru().outputgateweightmatrix(), type) ||
                    isWeightParamOfType(layer.gru().updategaterecursionmatrix(), type) ||
                    isWeightParamOfType(layer.gru().resetgaterecursionmatrix(), type) ||
                    isWeightParamOfType(layer.gru().outputgaterecursionmatrix(), type) ||
                    isWeightParamOfType(layer.gru().updategatebiasvector(), type) ||
                    isWeightParamOfType(layer.gru().resetgatebiasvector(), type) ||
                    isWeightParamOfType(layer.gru().outputgatebiasvector(), type));

        case Specification::NeuralNetworkLayer::LayerCase::kEmbedding:
            return (isWeightParamOfType(layer.embedding().weights(), type) ||
                    isWeightParamOfType(layer.embedding().bias(), type));

        case Specification::NeuralNetworkLayer::LayerCase::kUniDirectionalLSTM:
            return hasLSTMWeightParamOfType(layer.unidirectionallstm().weightparams(), type);

        case Specification::NeuralNetworkLayer::LayerCase::kBiDirectionalLSTM:
            return (hasLSTMWeightParamOfType(layer.bidirectionallstm().weightparams(0), type) ||
                    hasLSTMWeightParamOfType(layer.bidirectionallstm().weightparams(1), type));

        case Specification::NeuralNetworkLayer::LayerCase::kActivation:
            if(layer.activation().NonlinearityType_case() == Specification::ActivationParams::NonlinearityTypeCase::kPReLU) {
                return isWeightParamOfType(layer.activation().prelu().alpha(), type);
            } else if(layer.activation().NonlinearityType_case() == Specification::ActivationParams::NonlinearityTypeCase::kParametricSoftplus) {
                return (isWeightParamOfType(layer.activation().parametricsoftplus().alpha(), type) ||
                        isWeightParamOfType(layer.activation().parametricsoftplus().beta(), type));
            }
        default:
            break;
    }
    return false;
}

bool CoreML::hasfp16Weights(const Specification::Model& model) {
    // If any of the weight param is of type FP16, the model has FP16 weight
    return hasWeightOfType(model, FLOAT16);
}

bool CoreML::hasUnsignedQuantizedWeights(const Specification::Model& model) {
    return hasWeightOfType(model, QUINT);
}

bool CoreML::hasWeightOfType(const Specification::Model& model, const WeightParamType& wt) {
    auto layers = getNNSpec(model);
    if(layers) {
        for(int i =0; i< layers->size(); i++){
            const Specification::NeuralNetworkLayer& layer = (*layers)[i];
            if(hasWeightOfType(layer,wt)) {
                return true;
            }
        }
    }
    return false;
}

// We'll check if the model has ONLY the IOS12 shape specifications
// if the old ones are also filled in with something plausible, then there is nothing
// preventing us from running on older versions of Core ML.
bool CoreML::hasFlexibleShapes(const Specification::Model& model) {

    auto inputs = model.description().input();
    for (const auto& input: inputs) {
        if (input.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
            if (input.type().multiarraytype().ShapeFlexibility_case() != Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET) {
                return true;
            }
        }
        else if (input.type().Type_case() == Specification::FeatureType::kImageType) {
            if (input.type().imagetype().SizeFlexibility_case() != Specification::ImageFeatureType::SIZEFLEXIBILITY_NOT_SET) {
                return true;
            }
        }
    }
    return false;
}

bool CoreML::hasIOS11_2Features(const Specification::Model& model) {
    bool result = false;
    switch (model.Type_case()) {
        case Specification::Model::kPipeline:
            for (auto &m : model.pipeline().models()) {
                result = result || hasIOS11_2Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineRegressor:
            for (auto &m : model.pipelineregressor().pipeline().models()) {
                result = result || hasIOS11_2Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineClassifier:
            for (auto &m : model.pipelineclassifier().pipeline().models()) {
                result = result || hasIOS11_2Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        default:
            return (hasCustomLayer(model) || hasfp16Weights(model));
    }
    return false;
}

bool CoreML::hasIOS12Features(const Specification::Model& model) {
    // New IOS12 features: flexible shapes, custom model, sequence feature type,
    // text classifier, word tagger, vision feature print, unsigned integer quantization
    bool result = false;
    switch (model.Type_case()) {
        case Specification::Model::kPipeline:
            for (auto &m : model.pipeline().models()) {
                result = result || hasIOS12Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineRegressor:
            for (auto &m : model.pipelineregressor().pipeline().models()) {
                result = result ||hasIOS12Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineClassifier:
            for (auto &m : model.pipelineclassifier().pipeline().models()) {
                result = result || hasIOS12Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        default:
            return (hasFlexibleShapes(model) || hasCustomModel(model) || hasCategoricalSequences(model) ||
                    hasAppleTextClassifier(model) || hasAppleWordTagger(model) ||
                    hasAppleImageFeatureExtractor(model) || hasUnsignedQuantizedWeights(model) ||
                    hasNonmaxSuppression(model) || hasBayesianProbitRegressor(model) ||
                    hasIOS12NewNeuralNetworkLayers(model));
    }
    return false;
}

bool CoreML::hasCustomModel(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kCustomModel);
}

bool CoreML::hasAppleWordTagger(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kWordTagger);
}

bool CoreML::hasAppleTextClassifier(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kTextClassifier);
}

bool CoreML::hasAppleImageFeatureExtractor(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kVisionFeaturePrint);
}

bool CoreML::hasNonmaxSuppression(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kNonMaximumSuppression);
}

bool CoreML::hasBayesianProbitRegressor(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kBayesianProbitRegressor);
}

bool CoreML::hasCategoricalSequences(const Specification::Model& model) {

    for (int i=0; i<model.description().input_size(); i++) {
        auto &feature = model.description().input(i);
        if (feature.type().Type_case() == Specification::FeatureType::kSequenceType) {
            switch (feature.type().sequencetype().Type_case()) {
                case Specification::SequenceFeatureType::kStringType:
                case Specification::SequenceFeatureType::kInt64Type:
                    return true;
                default:
                    break;
            }
        }
    }

    for (int i=0; i<model.description().output_size(); i++) {
        auto &feature = model.description().output(i);
        if (feature.type().Type_case() == Specification::FeatureType::kSequenceType) {
            switch (feature.type().sequencetype().Type_case()) {
                case Specification::SequenceFeatureType::kStringType:
                case Specification::SequenceFeatureType::kInt64Type:
                    return true;
                default:
                    break;
            }
        }
    }

    return false;
}

bool CoreML::hasIOS12NewNeuralNetworkLayers(const Specification::Model& model) {
    auto layers = getNNSpec(model);
    if (layers) {
        for (int i=0; i< layers->size(); i++){
            const Specification::NeuralNetworkLayer& layer = (*layers)[i];
            if (layer.layer_case() == Specification::NeuralNetworkLayer::kResizeBilinear) {
                return true;
            }
            if (layer.layer_case() == Specification::NeuralNetworkLayer::kCropResize) {
                return true;
            }
        }
    }
    return false;
}

bool CoreML::hasModelOrSubModelProperty(const Specification::Model& model, const std::function<bool(const Specification::Model&)> &boolFunc) {
    bool result = false;
    switch (model.Type_case()) {
        case Specification::Model::kPipeline:
            for (auto &m : model.pipeline().models()) {
                result = result || boolFunc(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineRegressor:
            for (auto &m : model.pipelineregressor().pipeline().models()) {
                result = result || boolFunc(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineClassifier:
            for (auto &m : model.pipelineclassifier().pipeline().models()) {
                result = result || boolFunc(m);
                if (result) {
                    return true;
                }
            }
            break;
        default:
            return boolFunc(model);
    }
    return false;
}
