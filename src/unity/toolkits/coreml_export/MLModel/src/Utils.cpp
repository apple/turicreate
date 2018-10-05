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
bool CoreML::hasCustomLayer(Specification::Model& model) {
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

WeightParamType CoreML::getLSTMWeightParamType(const Specification::LSTMWeightParams& params) {
    
    // We assume all the weight param types are set correctly
    WeightParamType retval = FLOAT32;
    
    if (valueType(params.inputgateweightmatrix()) == FLOAT16
        || valueType(params.forgetgateweightmatrix()) == FLOAT16
        || valueType(params.blockinputweightmatrix()) == FLOAT16
        || valueType(params.outputgateweightmatrix()) == FLOAT16

        || valueType(params.inputgaterecursionmatrix()) == FLOAT16
        || valueType(params.forgetgaterecursionmatrix()) == FLOAT16
        || valueType(params.blockinputrecursionmatrix()) == FLOAT16
        || valueType(params.outputgaterecursionmatrix()) == FLOAT16

        || valueType(params.inputgatebiasvector()) == FLOAT16
        || valueType(params.forgetgatebiasvector()) == FLOAT16
        || valueType(params.blockinputbiasvector()) == FLOAT16
        || valueType(params.outputgatebiasvector()) == FLOAT16

        || valueType(params.inputgatepeepholevector()) == FLOAT16
        || valueType(params.forgetgatepeepholevector()) == FLOAT16
        || valueType(params.outputgatepeepholevector()) == FLOAT16) {
        retval = FLOAT16;
    }
    
    return retval;
    
}


WeightParamType CoreML::getWeightParamType(const Specification::NeuralNetworkLayer& layer) {
    
    WeightParamType retval = FLOAT32;
    
    switch (layer.layer_case()) {
        case Specification::NeuralNetworkLayer::LayerCase::kConvolution:
            
            if (valueType(layer.convolution().weights()) == FLOAT16
                || valueType(layer.convolution().bias()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kInnerProduct:
            if (valueType(layer.innerproduct().weights()) == FLOAT16
                || valueType(layer.innerproduct().bias()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kBatchnorm:
            if (valueType(layer.batchnorm().gamma()) == FLOAT16
                || valueType(layer.batchnorm().beta()) == FLOAT16
                || valueType(layer.batchnorm().mean()) == FLOAT16
                || valueType(layer.batchnorm().variance()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kLoadConstant:
            if (valueType(layer.loadconstant().data()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kScale:
            if (valueType(layer.scale().scale()) == FLOAT16
                || valueType(layer.scale().bias()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kSimpleRecurrent:
            if (valueType(layer.simplerecurrent().weightmatrix()) == FLOAT16
                || valueType(layer.simplerecurrent().recursionmatrix()) == FLOAT16
                || valueType(layer.simplerecurrent().biasvector()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kGru:
            if (valueType(layer.gru().updategateweightmatrix()) == FLOAT16
                || valueType(layer.gru().resetgateweightmatrix()) == FLOAT16
                || valueType(layer.gru().outputgateweightmatrix()) == FLOAT16
                || valueType(layer.gru().updategaterecursionmatrix()) == FLOAT16
                || valueType(layer.gru().resetgaterecursionmatrix()) == FLOAT16
                || valueType(layer.gru().outputgaterecursionmatrix()) == FLOAT16
                || valueType(layer.gru().updategatebiasvector()) == FLOAT16
                || valueType(layer.gru().resetgatebiasvector()) == FLOAT16
                || valueType(layer.gru().outputgatebiasvector()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kUniDirectionalLSTM:
            retval = getLSTMWeightParamType(layer.unidirectionallstm().weightparams());
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kEmbedding:
            if (valueType(layer.embedding().weights()) == FLOAT16
                || valueType(layer.embedding().bias()) == FLOAT16)
                retval = FLOAT16;
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kBiDirectionalLSTM: {
            retval = getLSTMWeightParamType(layer.bidirectionallstm().weightparams(0));
            if (retval == FLOAT32)
                retval = getLSTMWeightParamType(layer.bidirectionallstm().weightparams(1));
            break;
        }
        case Specification::NeuralNetworkLayer::LayerCase::kActivation:
            if (layer.activation().NonlinearityType_case() == Specification::ActivationParams::NonlinearityTypeCase::kPReLU) {
                retval = valueType(layer.activation().prelu().alpha());
            }
            else if (layer.activation().NonlinearityType_case() == Specification::ActivationParams::NonlinearityTypeCase::kParametricSoftplus) {
                retval = valueType(layer.activation().parametricsoftplus().alpha());
                if (retval == FLOAT32) {
                    retval = valueType(layer.activation().parametricsoftplus().beta());
                }
            }
            break;
        case Specification::NeuralNetworkLayer::LayerCase::kPooling:
        case Specification::NeuralNetworkLayer::LayerCase::kPadding:
        case Specification::NeuralNetworkLayer::LayerCase::kConcat:
        case Specification::NeuralNetworkLayer::LayerCase::kLrn:
        case Specification::NeuralNetworkLayer::LayerCase::kSoftmax:
        case Specification::NeuralNetworkLayer::LayerCase::kSplit:
        case Specification::NeuralNetworkLayer::LayerCase::kAdd:
        case Specification::NeuralNetworkLayer::LayerCase::kMultiply:
        case Specification::NeuralNetworkLayer::LayerCase::kUnary:
        case Specification::NeuralNetworkLayer::LayerCase::kUpsample:
        case Specification::NeuralNetworkLayer::LayerCase::kBias:
        case Specification::NeuralNetworkLayer::LayerCase::kL2Normalize:
        case Specification::NeuralNetworkLayer::LayerCase::kReshape:
        case Specification::NeuralNetworkLayer::LayerCase::kFlatten:
        case Specification::NeuralNetworkLayer::LayerCase::kPermute:
        case Specification::NeuralNetworkLayer::LayerCase::kReduce:
        case Specification::NeuralNetworkLayer::LayerCase::kCrop:
        case Specification::NeuralNetworkLayer::LayerCase::kAverage:
        case Specification::NeuralNetworkLayer::LayerCase::kMax:
        case Specification::NeuralNetworkLayer::LayerCase::kMin:
        case Specification::NeuralNetworkLayer::LayerCase::kDot:
        case Specification::NeuralNetworkLayer::LayerCase::kMvn:
        case Specification::NeuralNetworkLayer::LayerCase::kSequenceRepeat:
        case Specification::NeuralNetworkLayer::LayerCase::kReorganizeData:
        case Specification::NeuralNetworkLayer::LayerCase::kSlice:
        case Specification::NeuralNetworkLayer::kCustom:
        case Specification::NeuralNetworkLayer::LAYER_NOT_SET:
            break;
    }
    
    return retval;
    
}

bool CoreML::hasfp16Weights(Specification::Model& model) {

    // We assume here that the validator has already enforced that models don't fill in both
    // the fp16 and float weight fields.
    auto layers = getNNSpec(model);
    if (layers) {
        for (int i =0; i< layers->size(); i++){
            const Specification::NeuralNetworkLayer& layer = (*layers)[i];
            WeightParamType weights = getWeightParamType(layer);
            if (weights == FLOAT16)
                return true;
        }
    }
    return false;
}

// We'll check if the model has ONLY the IOS12 shape specifications
// if the old ones are also filled in with something plausible, then there is nothing
// preventing us from running on older versions of Core ML.
bool CoreML::hasOnlyFlexibleShapes(Specification::Model& model) {
    
    auto inputs = model.description().input();
    for (const auto& input: inputs) {
        if (input.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
            if (input.type().multiarraytype().shape_size() == 0) {
                return true;
            }
        }
        else if (input.type().Type_case() == Specification::FeatureType::kImageType) {
            // We'll only upgrade the spec here if the width or height are unset -- these should be some usable defaults
            if (input.type().imagetype().width() == 0 || input.type().imagetype().height() == 0) {
                return true;
            }
        }
    }
    
    return false;
    
}

bool CoreML::hasIOS12Features(Specification::Model& model) {
    
    // New features: flexible shapes, custom model, sequence feature type,
    // text classifier, word tagger, vision feature print
    if (hasOnlyFlexibleShapes(model) || hasCustomModel(model) || hasCategoricalSequences(model)
        || hasAppleTextClassifier(model) || hasAppleWordTagger(model) || hasAppleImageFeatureExtractor(model)) {
        return true;
    }
    
    return false;
    
}

bool CoreML::hasCustomModel(Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kCustomModel);
}

bool CoreML::hasAppleWordTagger(Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kWordTagger);
}

bool CoreML::hasAppleTextClassifier(Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kTextClassifier);
}

bool CoreML::hasAppleImageFeatureExtractor(Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kVisionFeaturePrint);
}

bool CoreML::hasCategoricalSequences(Specification::Model& model) {

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
