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


    if (pModel->specificationversion() == 0 || pModel->specificationversion() > MLMODEL_SPECIFICATION_VERSION_NEWEST) {
        // If mistakenly set specification verion or never set and left as default
        // lets start at the newest specification version and downgrade from there
        pModel->set_specificationversion(MLMODEL_SPECIFICATION_VERSION_NEWEST);
    }


    if (pModel->specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS14 && !hasIOS14Features(*pModel)) {
        pModel->set_specificationversion(MLMODEL_SPECIFICATION_VERSION_IOS13);
    }

    if (pModel->specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS13 && !hasIOS13Features(*pModel)) {
        pModel->set_specificationversion(MLMODEL_SPECIFICATION_VERSION_IOS12);
    }

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

        case Specification::NeuralNetworkLayer::LayerCase::kBatchedMatmul:
            return (isWeightParamOfType(layer.batchedmatmul().weights(),type) ||
                    isWeightParamOfType(layer.batchedmatmul().bias(), type));

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

        case Specification::NeuralNetworkLayer::LayerCase::kEmbeddingND:
            return (isWeightParamOfType(layer.embeddingnd().weights(), type) ||
                    isWeightParamOfType(layer.embeddingnd().bias(), type));

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
                    hasScenePrint(model) || hasUnsignedQuantizedWeights(model) ||
                    hasNonmaxSuppression(model) || hasBayesianProbitRegressor(model) ||
                    hasIOS12NewNeuralNetworkLayers(model));
    }
    return false;
}

bool CoreML::hasIOS13Features(const Specification::Model& model) {
    // New IOS13 features:
    // - no constraint on rank for NN inputs
    // - model is marked as updatable
    // - model parameters are specified
    // - model is of type kKNearestNeighborsClassifier
    // - model is of sound analysis preprocessing
    // - model is of type LinkedModel
    // - model is of type TextClassifier with revision == 2
    // - model is of type Gazetteer
    // - model is of type WordEmbedding
    // - (... add others here ...)

    if (model.isupdatable()) {
        return true;
    }

    bool result = false;
    switch (model.Type_case()) {
        case Specification::Model::kPipeline:
            for (auto &m : model.pipeline().models()) {
                result = result || hasIOS13Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineRegressor:
            for (auto &m : model.pipelineregressor().pipeline().models()) {
                result = result ||hasIOS13Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineClassifier:
            for (auto &m : model.pipelineclassifier().pipeline().models()) {
                result = result || hasIOS13Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kKNearestNeighborsClassifier:
        case Specification::Model::kLinkedModel:
            return true;
        case Specification::Model::kItemSimilarityRecommender:
            return hasItemSimilarityRecommender(model);
        case Specification::Model::kSoundAnalysisPreprocessing:
            return hasSoundAnalysisPreprocessing(model);
        case Specification::Model::kTextClassifier:
            return model.textclassifier().revision() == 2;
        case Specification::Model::kGazetteer:
            return model.gazetteer().revision() == 2;
        case Specification::Model::kWordEmbedding:
            return model.wordembedding().revision() == 2;
        default:
            return hasIOS13NeuralNetworkFeatures(model);
    }
    return false;
}

bool CoreML::hasDefaultValueForOptionalInputs(const Specification::Model& model) {
    // Checks if default optional value has been set or not
    for (const auto& input: model.description().input()) {
        if (input.type().isoptional()){
            switch (input.type().multiarraytype().defaultOptionalValue_case()) {
                case CoreML::Specification::ArrayFeatureType::kDoubleDefaultValue:
                case CoreML::Specification::ArrayFeatureType::kFloatDefaultValue:
                case CoreML::Specification::ArrayFeatureType::kIntDefaultValue:
                        return true;
                default:
                    break;
            }
        }
    }
    return false;
}

bool CoreML::hasFloat32InputsOrOutputsForNonmaxSuppression(const Specification::Model& model) {
    if (!hasNonmaxSuppression(model)) {
        // not NMS.
        return false;
    }

    auto inputs = model.description().input();
    for (const auto& input: inputs) {
        if (input.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
            if (input.type().multiarraytype().datatype() == Specification::ArrayFeatureType_ArrayDataType_FLOAT32) {
                return true;
            }
        }
    }

    auto outputs = model.description().output();
    for (const auto& output: outputs) {
        if (output.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
            if (output.type().multiarraytype().datatype() == Specification::ArrayFeatureType_ArrayDataType_FLOAT32) {
                return true;
            }
        }
    }

    return false;
}

bool CoreML::hasIOS14Features(const Specification::Model& model) {
    // New IOS14 features:
    // - new layers in Neural Network
    // - Non-zero values for optional inputs
    // - VisionFeaturePrint.Object
    // - Float32 input/output for Non-Maximum Suppression
    // - Apple Word Tagger using transfer learning (revision == 3)

    bool result = false;

    switch (model.Type_case()) {
        case Specification::Model::kPipeline:
            for (auto &m : model.pipeline().models()) {
                result = result || hasIOS14Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineRegressor:
            for (auto &m : model.pipelineregressor().pipeline().models()) {
                result = result || hasIOS14Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kPipelineClassifier:
            for (auto &m : model.pipelineclassifier().pipeline().models()) {
                result = result || hasIOS14Features(m);
                if (result) {
                    return true;
                }
            }
            break;
        case Specification::Model::kSerializedModel:
            return true;
        case Specification::Model::kWordTagger:
            return model.wordtagger().revision() == 3;
        default:
            return (hasIOS14NeuralNetworkFeatures(model) || hasObjectPrint(model) || hasFloat32InputsOrOutputsForNonmaxSuppression(model));
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

bool CoreML::hasAppleGazetteer(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kGazetteer);
}

bool CoreML::hasAppleWordEmbedding(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kWordEmbedding);
}

bool CoreML::hasAppleImageFeatureExtractor(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kVisionFeaturePrint);
}

bool CoreML::hasScenePrint(const Specification::Model& model) {
    return (hasAppleImageFeatureExtractor(model) && model.visionfeatureprint().has_scene());
}

bool CoreML::hasObjectPrint(const Specification::Model& model) {
    return (hasAppleImageFeatureExtractor(model) && model.visionfeatureprint().has_objects());
}

bool CoreML::hasNonmaxSuppression(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kNonMaximumSuppression);
}

bool CoreML::hasBayesianProbitRegressor(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kBayesianProbitRegressor);
}

bool CoreML::hasItemSimilarityRecommender(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kItemSimilarityRecommender);
}

bool CoreML::hasSoundAnalysisPreprocessing(const Specification::Model& model) {
    return (model.Type_case() == Specification::Model::kSoundAnalysisPreprocessing);
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

    // Return True if the model has the two new NN layers added in iOS 12, which are
    // resizeBilinear and CropResize

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

bool CoreML::isIOS12NeuralNetworkLayer(const Specification::NeuralNetworkLayer& layer) {

    // Return True if the NN layer is from the set exposed in iOS 12
    switch (layer.layer_case()) {
        case Specification::NeuralNetworkLayer::LayerCase::kConvolution:
            return (layer.input().size() == 1);
        case Specification::NeuralNetworkLayer::LayerCase::kInnerProduct:
            return !layer.innerproduct().int8dynamicquantize();
        case Specification::NeuralNetworkLayer::LayerCase::kBatchnorm:
        case Specification::NeuralNetworkLayer::LayerCase::kActivation:
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
            if (layer.upsample().linearupsamplemode() != Specification::UpsampleLayerParams_LinearUpsampleMode_DEFAULT) {
                return false;
            }
            if (layer.upsample().fractionalscalingfactor_size() > 0) {
                return false;
            }
        case Specification::NeuralNetworkLayer::LayerCase::kBias:
        case Specification::NeuralNetworkLayer::LayerCase::kL2Normalize:
        case Specification::NeuralNetworkLayer::LayerCase::kReshape:
        case Specification::NeuralNetworkLayer::LayerCase::kFlatten:
        case Specification::NeuralNetworkLayer::LayerCase::kPermute:
        case Specification::NeuralNetworkLayer::LayerCase::kReduce:
        case Specification::NeuralNetworkLayer::LayerCase::kLoadConstant:
        case Specification::NeuralNetworkLayer::LayerCase::kScale:
        case Specification::NeuralNetworkLayer::LayerCase::kSimpleRecurrent:
        case Specification::NeuralNetworkLayer::LayerCase::kGru:
        case Specification::NeuralNetworkLayer::LayerCase::kUniDirectionalLSTM:
        case Specification::NeuralNetworkLayer::LayerCase::kBiDirectionalLSTM:
        case Specification::NeuralNetworkLayer::LayerCase::kCrop:
        case Specification::NeuralNetworkLayer::LayerCase::kAverage:
        case Specification::NeuralNetworkLayer::LayerCase::kMax:
        case Specification::NeuralNetworkLayer::LayerCase::kMin:
        case Specification::NeuralNetworkLayer::LayerCase::kDot:
        case Specification::NeuralNetworkLayer::LayerCase::kMvn:
        case Specification::NeuralNetworkLayer::LayerCase::kEmbedding:
        case Specification::NeuralNetworkLayer::LayerCase::kSequenceRepeat:
        case Specification::NeuralNetworkLayer::LayerCase::kReorganizeData:
            if (layer.reorganizedata().mode() == Specification::ReorganizeDataLayerParams::PIXEL_SHUFFLE) {
                      return false;
            }
        case Specification::NeuralNetworkLayer::LayerCase::kSlice:
        case Specification::NeuralNetworkLayer::LayerCase::kCustom:
        case Specification::NeuralNetworkLayer::kResizeBilinear:
        case Specification::NeuralNetworkLayer::kCropResize:
            return true;
        default:
            return false;
    }
}


bool CoreML::hasIOS13NeuralNetworkFeatures(const Specification::Model& model) {

    /* check if any of the messages in NeuralNetwork.proto, that were added in iOS version 13, are being used.
      If they are, return True, otherwise return False.

     In particular, check for the presence of the following messages:
     1. any new layer type, which was not in iOS 12.
     2. if the value of enums "NeuralNetworkMultiArrayShapeMapping" or "NeuralNetworkImageShapeMapping" is non 0
     */

    switch (model.Type_case()) {
        case Specification::Model::TypeCase::kNeuralNetwork:
            if (model.neuralnetwork().arrayinputshapemapping() != Specification::NeuralNetworkMultiArrayShapeMapping::RANK5_ARRAY_MAPPING) {
                return true;
            }
            if (model.neuralnetwork().imageinputshapemapping() != Specification::NeuralNetworkImageShapeMapping::RANK5_IMAGE_MAPPING) {
                return true;
            }
        case Specification::Model::TypeCase::kNeuralNetworkRegressor:
            if (model.neuralnetworkregressor().arrayinputshapemapping() != Specification::NeuralNetworkMultiArrayShapeMapping::RANK5_ARRAY_MAPPING) {
                return true;
            }
            if (model.neuralnetworkregressor().imageinputshapemapping() != Specification::NeuralNetworkImageShapeMapping::RANK5_IMAGE_MAPPING) {
                return true;
            }
        case Specification::Model::TypeCase::kNeuralNetworkClassifier:
            if (model.neuralnetworkclassifier().arrayinputshapemapping() != Specification::NeuralNetworkMultiArrayShapeMapping::RANK5_ARRAY_MAPPING) {
                return true;
            }
            if (model.neuralnetworkclassifier().imageinputshapemapping() != Specification::NeuralNetworkImageShapeMapping::RANK5_IMAGE_MAPPING) {
                return true;
            }
        default:
            break;
    }

    // check for new layers: by checking if its NOT one of the layers supported in iOS 12
    auto layers = getNNSpec(model);
    if (layers) {
        for (int i=0; i< layers->size(); i++){
            const Specification::NeuralNetworkLayer& layer = (*layers)[i];
            if (!isIOS12NeuralNetworkLayer(layer)) {
                return true;
            }
        }
    }

    return false;
}

bool CoreML::hasIOS14NeuralNetworkFeatures(const Specification::Model& model) {

    // Return True if the model has the new Neural network features added in
    // ios 14

    if (hasDefaultValueForOptionalInputs(model)) {
        return true;
    }


    auto layers = getNNSpec(model);
    if (layers) {
        for (int i=0; i<layers->size(); i++){
            const Specification::NeuralNetworkLayer& layer = (*layers)[i];
            switch (layer.layer_case()) {
                case Specification::NeuralNetworkLayer::kCumSum:
                case Specification::NeuralNetworkLayer::kOneHot:
                case Specification::NeuralNetworkLayer::kClampedReLU:
                case Specification::NeuralNetworkLayer::kArgSort:
                case Specification::NeuralNetworkLayer::kPooling3D:
                case Specification::NeuralNetworkLayer::kGlobalPooling3D:
                case Specification::NeuralNetworkLayer::kSliceBySize:
                case Specification::NeuralNetworkLayer::kConvolution3D:
                    return true;
                case Specification::NeuralNetworkLayer::kSliceDynamic:
                    if (layer.input().size() == 7) {
                        return true;
                    } else if (layer.slicedynamic().squeezemasks_size()) {
                        return true;
                    }
                case Specification::NeuralNetworkLayer::kUpsample:
                    if (layer.upsample().linearupsamplemode() != Specification::UpsampleLayerParams_LinearUpsampleMode_DEFAULT) {
                        return true;
                    }
                    if (layer.upsample().fractionalscalingfactor_size() > 0) {
                        return true;
                    }
                case Specification::NeuralNetworkLayer::kReorganizeData:
                    if (layer.reorganizedata().mode() == Specification::ReorganizeDataLayerParams::PIXEL_SHUFFLE) {
                      return true;
                    }
                case Specification::NeuralNetworkLayer::kInnerProduct:
                    if (layer.innerproduct().int8dynamicquantize())
                        return true;
                case Specification::NeuralNetworkLayer::kBatchedMatmul:
                    if (layer.batchedmatmul().int8dynamicquantize())
                        return true;
                default:
                    continue;
            }
        }
    }
    return false;
}
