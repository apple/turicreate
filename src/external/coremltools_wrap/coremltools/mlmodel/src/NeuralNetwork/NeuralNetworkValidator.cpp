//
//  NeuralNetworkValidator.cpp
//  libmlmodelspec
//
//  Created by Zach Nation on 12/21/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "NeuralNetworkValidator.hpp"
#include "NeuralNetworkValidatorUtils.hpp"
#include "../ParameterValidator.hpp"

#include <algorithm>
#include <functional>
#include <sstream>

using namespace CoreML;

#pragma mark NeuralNetworkSpecValidator constructor and static variables initializaion

NeuralNetworkSpecValidator::NeuralNetworkSpecValidator() {}

NeuralNetworkSpecValidator::NeuralNetworkSpecValidator(const std::map<std::string, std::set<std::string>> &blobsInitial,
                                                       const std::map<std::string, int> &ioBlobNameToRank,
                                                       bool ndArrayMode,
                                                       int loopDepth,
                                                       const std::map<std::string, int> &blobRanks) {
    blobs = blobsInitial; // copy it into the object variable
    ModelIOBlobNameToRank = ioBlobNameToRank;
    ndArrayInterpretation = ndArrayMode;
    loopStackDepth = loopDepth;
    blobNameToRank = blobRanks;
}

#pragma mark NeuralNetworkSpecValidator member functions

Result NeuralNetworkSpecValidator::validateLayer(const Specification::NeuralNetworkLayer& layer) {
    
    switch(layer.layer_case()) {
        case Specification::NeuralNetworkLayer::LayerCase::kConvolution:
            return validateConvolutionLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kInnerProduct:
            return validateInnerProductLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kBatchnorm:
            return validateBatchnormLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kActivation:
            return validateActivation(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kPooling:
            return validatePoolingLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kPadding:
            return validatePaddingLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kConcat:
            return validateConcatLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kLrn:
            return validateLRNLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSoftmax:
            return validateSoftmaxLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSplit:
            return validateSplitLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kAdd:
            return validateAddLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kMultiply:
            return validateMultiplyLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kUnary:
            return validateUnaryFunctionLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kUpsample:
            return validateUpsampleLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kBias:
            return validateBiasLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kL2Normalize:
            return validateL2NormLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReshape:
            return validateReshapeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kFlatten:
            return validateFlattenLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kPermute:
            return validatePermuteLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduce:
            return validateReduceLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kLoadConstant:
            return validateLoadConstantLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kScale:
            return validateScaleLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSimpleRecurrent:
            return validateSimpleRecurrentLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kGru:
            return validateGRULayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kUniDirectionalLSTM:
            return validateUniDirectionalLSTMLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kBiDirectionalLSTM:
            return validateBiDirectionalLSTMLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kCrop:
            return validateCropLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kAverage:
            return validateAverageLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kMax:
            return validateMaxLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kMin:
            return validateMinLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kDot:
            return validateDotLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kMvn:
            return validateMvnLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kEmbedding:
            return validateEmbeddingLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSequenceRepeat:
            return validateSequenceRepeatLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReorganizeData:
            return validateReorganizeDataLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSlice:
            return validateSliceLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kCustom:
            return validateCustomLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kResizeBilinear:
            return validateResizeBilinearLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kCropResize:
            return validateCropResizeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kTranspose:
            return validateTransposeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kBranch:
            return validateBranchLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kCopy:
            return validateCopyLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kBatchedMatmul:
            return validateBatchedMatmulLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kConcatND:
            return validateConcatNDLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSoftmaxND:
            return validateSoftmaxNDLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReverse:
            return validateReverseLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kFillLike:
            return validateFillLikeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kFillStatic:
            return validateFillStaticLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kFillDynamic:
            return validateFillDynamicLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kWhereBroadcastable:
            return validateWhereLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSin:
        case Specification::NeuralNetworkLayer::LayerCase::kCos:
        case Specification::NeuralNetworkLayer::LayerCase::kTan:
        case Specification::NeuralNetworkLayer::LayerCase::kAsin:
        case Specification::NeuralNetworkLayer::LayerCase::kAcos:
        case Specification::NeuralNetworkLayer::LayerCase::kAtan:
        case Specification::NeuralNetworkLayer::LayerCase::kSinh:
        case Specification::NeuralNetworkLayer::LayerCase::kCosh:
        case Specification::NeuralNetworkLayer::LayerCase::kTanh:
        case Specification::NeuralNetworkLayer::LayerCase::kAsinh:
        case Specification::NeuralNetworkLayer::LayerCase::kAcosh:
        case Specification::NeuralNetworkLayer::LayerCase::kAtanh:
            return validateTrigonometryLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kExp2:
            return validateExp2Layer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kMatrixBandPart:
            return validateMatrixBandPartLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kUpperTriangular:
            return validateUpperTriangularLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kLowerTriangular:
            return validateLowerTriangularLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kPowBroadcastable:
            return validatePowBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kGreaterThan:
        case CoreML::Specification::NeuralNetworkLayer::kGreaterEqual:
        case CoreML::Specification::NeuralNetworkLayer::kLessEqual:
        case CoreML::Specification::NeuralNetworkLayer::kLessThan:
        case CoreML::Specification::NeuralNetworkLayer::kEqual:
        case CoreML::Specification::NeuralNetworkLayer::kNotEqual:
        case CoreML::Specification::NeuralNetworkLayer::kLogicalAnd:
        case CoreML::Specification::NeuralNetworkLayer::kLogicalOr:
        case CoreML::Specification::NeuralNetworkLayer::kLogicalXor:
        case CoreML::Specification::NeuralNetworkLayer::kLogicalNot:
            return validateBooleanElementwiseLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kLoop:
            return validateLoopLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kLoopContinue:
        case CoreML::Specification::NeuralNetworkLayer::kLoopBreak:
            return validateLoopContinueBreakLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kErf:
        case CoreML::Specification::NeuralNetworkLayer::kGelu:
            return validateActivationLayers(layer);
        case CoreML::Specification::NeuralNetworkLayer::kRankPreservingReshape:
            return validateRankPreservingReshapeLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kExpandDims:
            return validateExpandDimsLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kFlattenTo2D:
            return validateFlattenTo2DLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReshapeLike:
            return validateReshapeLikeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReshapeStatic:
            return validateReshapeStaticLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReshapeDynamic:
            return validateReshapeDynamicLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kSqueeze:
            return validateSqueezeLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kBroadcastToLike:
            return validateBroadcastToLikeLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kBroadcastToStatic:
            return validateBroadcastToStaticLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kBroadcastToDynamic:
            return validateBroadcastToDynamicLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kAddBroadcastable:
            return validateAddBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kSubtractBroadcastable:
            return validateSubtractBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kMultiplyBroadcastable:
            return validateMultiplyBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kDivideBroadcastable:
            return validateDivideBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kMaxBroadcastable:
            return validateMaxBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kMinBroadcastable:
            return validateMinBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kFloorDivBroadcastable:
            return validateFloorDivBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kModBroadcastable:
            return validateModBroadcastableLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kGather:
            return validateGatherLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kScatter:
            return validateScatterLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kGatherND:
            return validateGatherNDLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kScatterND:
            return validateScatterNDLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kGatherAlongAxis:
            return validateGatherAlongAxisLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kScatterAlongAxis:
            return validateScatterAlongAxisLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kWhereNonZero:
            return validateWhereNonZeroLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kStack:
            return validateStackLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kSplitND:
            return validateSplitNDLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kLoadConstantND:
            return validateLoadConstantNDLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kCeil:
            return validateCeilLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kFloor:
            return validateFloorLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kRound:
            return validateRoundLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kSign:
            return validateSignLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kClip:
            return validateClipLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kSliceStatic:
            return validateSliceStaticLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kSliceDynamic:
            return validateSliceDynamicLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kGetShape:
            return validateGetShapeLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kTile:
            return validateTileLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kRangeStatic:
            return validateRangeStaticLayer(layer);
        case CoreML::Specification::NeuralNetworkLayer::kRangeDynamic:
            return validateRangeDynamicLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kEmbeddingND:
            return validateEmbeddingNDLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kSlidingWindows:
            return validateSlidingWindowsLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomNormalLike:
            return validateRandomNormalLikeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomNormalStatic:
            return validateRandomNormalStaticLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomNormalDynamic:
            return validateRandomNormalDynamicLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomUniformLike:
            return validateRandomUniformLikeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomUniformStatic:
            return validateRandomUniformStaticLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomUniformDynamic:
            return validateRandomUniformDynamicLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomBernoulliLike:
            return validateRandomBernoulliLikeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomBernoulliStatic:
            return validateRandomBernoulliStaticLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kRandomBernoulliDynamic:
            return validateRandomBernoulliDynamicLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kTopK:
            return validateTopKLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kArgMax:
            return validateArgMaxLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kArgMin:
            return validateArgMinLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kCategoricalDistribution:
            return validateCategoricalDistributionLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceL1:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceL2:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceMax:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceMin:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceSum:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceProd:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceMean:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceLogSum:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceSumSquare:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReduceLogSumExp:
            return validateReductionTypeLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kReverseSeq:
            return validateReverseSeqLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kLayerNormalization:
            return validateLayerNormalizationLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kConstantPad:
            return validateConstantPadLayer(layer);
        case Specification::NeuralNetworkLayer::LayerCase::kNonMaximumSuppression:
            return validateNMSLayer(layer);
        default:
            return validateFailUnknownType(layer);
    }
}

template<typename T>
Result NeuralNetworkSpecValidator::validateNeuralNetwork(const T& nn) {
    
    Result r;
    
    // Loop over the layers
    // For each layer, validate the following:
    // 1. inputtensor/outputtensor message, rank compatibilty with Model input/output ranks
    // 2. Check rank consistency across the network for all blobs: ranks are not allowed to change for the same blob
    // 3. Call layer specific validation function
    // 4. check that layer's inputs are already present in "availableBlobs" set
    // 5. check that layer's outputs are NOT already present in "availableBlobs" set
    // 6. Add the layer's outputs to the "availableBlobs" set
    for (const auto& layer : nn.layers()) {
        
        if (!r.good()) {
            return r;
        }
        
        // check for inputtensor message valididty
        if (ndArrayInterpretation) {
            if (layer.inputtensor_size() != 0) {
                if (layer.input_size() != layer.inputtensor_size()) {
                    std::string err = "Layer '" + std::string(layer.name()) + "''s input and inputTensors have different lengths";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
                int idx = 0;
                for (const auto& tensor: layer.inputtensor()) {
                    r = validateTensorMessage(tensor, layer);
                    if (!r.good()) {
                        return r;
                    }
                    if (!(ModelIOBlobNameToRank.find(layer.input(idx)) == ModelIOBlobNameToRank.end())) {
                        if (static_cast<int>(tensor.rank()) != ModelIOBlobNameToRank[layer.input(idx)]) {
                            std::string err = "Layer '" + std::string(layer.name()) + "''s input '" + std::string(layer.input(idx)) + \
                            "' is also an input to the model. However, for this tensor the rank provided in the layer description" + \
                            " does not match the one provided in the model description";
                            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                        }
                    }
                    // verify rank consistency of intermediate blobs:
                    // if a blob is both an input and output to layers, the rank specified at both places must match
                    if (blobNameToRank.find(layer.input(idx)) == blobNameToRank.end()) {
                        blobNameToRank[layer.input(idx)] = (int)tensor.rank();
                    } else {
                        if (blobNameToRank[layer.input(idx)] != (int)tensor.rank()) {
                            std::string err = "Inconsistent rank for the blob named '" + std::string(layer.input(idx)) + "'.";
                            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                        }
                    }
                    idx++;
                }
            }
            if (layer.outputtensor_size() != 0) {
                if (layer.output_size() != layer.outputtensor_size()) {
                    std::string err = "Layer '" + std::string(layer.name()) + "''s output and \"outputTensors\" property have different lengths";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
                int idx = 0;
                for (const auto& tensor: layer.outputtensor()) {
                    r = validateTensorMessage(tensor, layer);
                    if (!r.good()) {
                        return r;
                    }
                    if (!(ModelIOBlobNameToRank.find(layer.output(idx)) == ModelIOBlobNameToRank.end())) {
                        if (static_cast<int>(tensor.rank()) != ModelIOBlobNameToRank[layer.output(idx)]) {
                            std::string err = "Layer '" + std::string(layer.name()) + "''s output '" + std::string(layer.input(idx)) + \
                            "' is also an output of the model. However, for this tensor the rank provided in the layer description" + \
                            " does not match the one provided in the model description";
                            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                        }
                    }
                    // verify rank consistency of intermediate blobs:
                    // if a blob is both an input and output to layers, the rank specified at both places must match
                    if (blobNameToRank.find(layer.output(idx)) == blobNameToRank.end()) {
                        blobNameToRank[layer.output(idx)] = (int)tensor.rank();
                    } else {
                        if (blobNameToRank[layer.output(idx)] != (int)tensor.rank()) {
                            std::string err = "Inconsistent rank for the blob named '" + std::string(layer.output(idx)) + "'.";
                            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                        }
                    }
                    idx++;
                }
            }
        } // inputtensor, outputtensor validity end
        
        // First, we check the layer for internal correctness
        // this calls layer wise function
        r = validateLayer(layer);
        
        if (!r.good()) {
            return r;
        }
        
        // Check for topological defects: the layer's input must have been produced by a blob we have
        // already seen.
        for (const auto& input: layer.input()) {
            if (blobs.find(input) == blobs.end()) {
                std::string err = "Layer '" + std::string(layer.name()) + "' consumes an input named '"
                + std::string(input) + "' which is not present in this network.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
        
        // Check for topological defects: check that the same output isn't being produced in two different places.
        // unless its the "copy" layer
        for (const auto& output: layer.output()) {
            if (blobs.find(output) != blobs.end()) {
                if (layer.layer_case() != Specification::NeuralNetworkLayer::LayerCase::kCopy) {
                    std::string layer_name = *(blobs[output].begin());
                    std::string err = "Layer '" + std::string(layer.name()) + "' produces an output named '"
                    + std::string(output) + "' which is also an output produced by the layer '"
                    + layer_name + "'.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
            blobs[output].insert(layer.name());
        }
    } // loop over layers
    
    return Result();
}

#pragma mark Network-Wide Validation

template<typename T>
Result validateNeuralNetworkTopLevel(const Specification::ModelDescription& interface,
                                     const T& nn, std::set<std::string>& outputBlobNames,
                                     bool isUpdatable) {
    Result r;
    
    // First calculate the value of the flag "ndArrayInterpretation"
    // ndArrayInterpretation == False ==> iOS 11/12 (old) execution path can be used, i.e. all tensors are static rank 5.
    // ndArrayInterpretation == True ==>  Tensors can have any rank (including 5).
    
    bool ndArrayInterpretation = false;
    
    bool hasNonIOS12Layer = false;
    bool hasNewArrayShapeMapping = false;
    bool hasNewImageShapeMapping = false;
    bool hasMultiArrayInput = false;
    
    for (const auto& input: interface.input()) {
        if (input.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
            hasMultiArrayInput = true;
            break;
        }
    }
    
    if (nn.arrayinputshapemapping() != Specification::NeuralNetworkMultiArrayShapeMapping::RANK5_ARRAY_MAPPING) {
        hasNewArrayShapeMapping = true;
    }
    
    if (nn.imageinputshapemapping() != Specification::NeuralNetworkImageShapeMapping::RANK5_IMAGE_MAPPING) {
        hasNewImageShapeMapping = true;
    }
    
    for (const auto &layer: nn.layers()) {
        if (!isIOS12NeuralNetworkLayer(layer)) {
            hasNonIOS12Layer = true;
            break;
        }
    }
    
    if (hasNonIOS12Layer || hasNewArrayShapeMapping || hasNewImageShapeMapping) {
        ndArrayInterpretation = true;
    }
    
    if (hasNonIOS12Layer && !hasNewArrayShapeMapping && hasMultiArrayInput) {
        return Result(ResultType::INVALID_MODEL_INTERFACE,
                      "Neural Network Multi-Array input shape mapping cannot be 'RANK5_ARRAY_MAPPING' if the network contains a layer added in version 3 (iOS 13) or later.");
    }
    
    if (!hasNewArrayShapeMapping && hasNewImageShapeMapping && hasMultiArrayInput) {
        return Result(ResultType::INVALID_MODEL_INTERFACE,
                      "Neural Network Multi-Array input shape mapping cannot be 'RANK5_ARRAY_MAPPING' if the image input Shape mapping is not 'RANK5_IMAGE_MAPPING'");
    }
    
    //==================== End of logic to determine the value of "ndArrayInterpretation" ======================
    
    if (interface.input_size() == 0) {
        return Result(ResultType::INVALID_MODEL_INTERFACE,
                      "Neural networks require at least one input.");
    }
    
    if (interface.output_size() == 0) {
        return Result(ResultType::INVALID_MODEL_INTERFACE,
                      "Neural networks produce at least one output.");
    }
    
    if (nn.layers().size() == 0) {
        return Result(ResultType::INVALID_MODEL_PARAMETERS,
                      "Neural networks require at least one layer.");
    }
    
    if (std::all_of(interface.input().begin(), interface.input().end(),
                    [](const Specification::FeatureDescription& input) {
                        return input.type().isoptional();
                    })) {
                        return Result(ResultType::INVALID_MODEL_INTERFACE,
                                      "Neural networks require at least one non-optional input.");
                    }
    
    
    // Check the inputs and output types
    if (!std::all_of(interface.input().begin(),
                     interface.input().end(),
                     [](const Specification::FeatureDescription& input) {
                         return (input.type().Type_case() == Specification::FeatureType::kImageType
                                 || input.type().Type_case() == Specification::FeatureType::kMultiArrayType);
                     })) {
                         return Result(ResultType::INVALID_MODEL_INTERFACE,
                                       "Neural Networks require inputs to be images or MLMultiArray.");
                     }
    
    
    std::map<std::string, int> ioBlobNameToRank; // to collect ranks of input/output blobs from the shapes present in the description
    
    // populate "ioBlobNameToRank"
    if (ndArrayInterpretation) {
        for (const auto& input: interface.input()) {
            if (input.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
                if (nn.arrayinputshapemapping() == Specification::NeuralNetworkMultiArrayShapeMapping::RANK5_ARRAY_MAPPING) {
                    ioBlobNameToRank[input.name()] = 5;
                } else {
                    ioBlobNameToRank[input.name()] = input.type().multiarraytype().shape_size();
                }
            } else if (input.type().Type_case() == Specification::FeatureType::kImageType) {
                if (nn.imageinputshapemapping() == Specification::NeuralNetworkImageShapeMapping::RANK5_IMAGE_MAPPING) {
                    ioBlobNameToRank[input.name()] = 5;
                } else {
                    ioBlobNameToRank[input.name()] = 4;
                }
            }
        }
        for (const auto& output: interface.output()) {
            if (output.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
                if (output.type().multiarraytype().shape_size()) {
                    ioBlobNameToRank[output.name()] = output.type().multiarraytype().shape_size();
                }
            }
        }
    }
    
    // Collect Model input names and do some checking
    
    // inputBlobs: For each named data blob, the name of the node which produced it (there can be multiple in if-else branches)
    std::map<std::string, std::set<std::string>> inputBlobs;
    for (const auto& input: interface.input()) {
        // For input blobs, we'll give them a dummy producing layer name
        inputBlobs[input.name()] = {"__input"};
        if (input.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
            
            if (!ndArrayInterpretation) {
                
                // only vector-like (rank 1) or image-like (rank 3) inputs are allowed
                bool validShapeFound = false;
                if (input.type().multiarraytype().shape().size() > 0) {
                    if (!(input.type().multiarraytype().shape().size() == 1
                          || input.type().multiarraytype().shape().size() == 3)) {
                        return Result(ResultType::INVALID_MODEL_INTERFACE, "Input MLMultiArray to neural networks must have dimension 1 (vector) or 3 (image-like arrays).");
                    }
                    else {
                        validShapeFound = true;
                    }
                }
                bool flexibilityIsRank1or3 = true;
                switch (input.type().multiarraytype().ShapeFlexibility_case()) {
                    case CoreML::Specification::ArrayFeatureType::kEnumeratedShapes:
                        for (const auto &shape : input.type().multiarraytype().enumeratedshapes().shapes()) {
                            if(shape.shape_size() != 1 && shape.shape_size() != 3) {
                                flexibilityIsRank1or3 = false;
                                break;
                            }
                        }
                        break;
                    case CoreML::Specification::ArrayFeatureType::kShapeRange:
                        flexibilityIsRank1or3 = (input.type().multiarraytype().shaperange().sizeranges_size() == 1 ||
                                                 input.type().multiarraytype().shaperange().sizeranges_size() == 3);
                        break;
                    case CoreML::Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                        flexibilityIsRank1or3 = false;
                        break;
                }
                if (!flexibilityIsRank1or3 && !validShapeFound) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE, "Input MLMultiArray to neural networks must have dimension 1 (vector) or 3 (image-like arrays).");
                } else if (flexibilityIsRank1or3) {
                    validShapeFound = true;
                }
                if (!validShapeFound) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE, "Input MLMultiArray to neural networks must have dimension 1 (vector) or 3 (image-like arrays).");
                }
                
            } else { // validate input shape when "ndArrayInterpretation" is True
                
                int rank = input.type().multiarraytype().shape().size();
                if (!(rank > 0)) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE, "Input MLMultiArray to neural networks must have at least 1 dimension.");
                }
                switch (input.type().multiarraytype().ShapeFlexibility_case()) {
                    case CoreML::Specification::ArrayFeatureType::kEnumeratedShapes:
                        break;
                    case CoreML::Specification::ArrayFeatureType::kShapeRange:
                        if (input.type().multiarraytype().shaperange().sizeranges_size() != rank) {
                            return Result(ResultType::INVALID_MODEL_INTERFACE, "For MLMultiArray input: Rank of the flexible shape range must match the rank of the default shape.");
                            break;
                        }
                    case CoreML::Specification::ArrayFeatureType::SHAPEFLEXIBILITY_NOT_SET:
                        break;
                }
                
            } // if else block on spec version to check validity of input shape
        }
    }
    
    // validate the Neural Network message
    
    //  create an object to validate neural network message
    NeuralNetworkSpecValidator validator(inputBlobs, ioBlobNameToRank, ndArrayInterpretation, 0, ioBlobNameToRank);
    
    r = validator.validateNeuralNetwork(nn);
    
    if (!r.good()) {
        return r;
    }
    
    // gather all output blobs of the graph
    for (auto& blob: validator.blobs){
        if (inputBlobs.find(blob.first) == inputBlobs.end()) {
            outputBlobNames.insert(blob.first);
        } else {
            // if we are here, this means this blob is also present in the set of "inputBlobs"
            // but it can still be a genuine output blob if multiple layers are generating it (e.g. copy layer)
            if (blob.second.size() > 1) {
                outputBlobNames.insert(blob.first);
            }
        }
    }
    
    // Call the shaper: compatibility with iOS 12
    if (!ndArrayInterpretation) {
        // Compute the shapes
        try {
            NeuralNetworkShaper shaper(interface, nn.layers());
        }
        catch(std::runtime_error& e) {
            std::string err = std::string("Error determining network blob shapes: ") + std::string(e.what());
            return Result(ResultType::POTENTIALLY_INVALID_NEURAL_NETWORK_SHAPES, err);
        }
    }
    
    if (!r.good()) {
        return r;
    }
    
    if (isUpdatable) {
        r = validateUpdatableNeuralNetwork(nn);
        if (!r.good()) { return r; }

        r = validateTrainingInputs(interface, nn);
        if (!r.good()) { return r; }
    }
    
    return r;
    
}

namespace CoreML {
    
    template <>
    Result validate<MLModelType_neuralNetworkClassifier>(const Specification::Model& format) {
        // must have classifier parameters
        Result r = validateClassifierInterface(format, format.neuralnetworkclassifier());
        if (!r.good()) {
            return r;
        }
        
        std::set<std::string> outputBlobNames;
        r = validateNeuralNetworkTopLevel(format.description(), format.neuralnetworkclassifier(), outputBlobNames, format.isupdatable());
        
        if (!r.good()) {
            return r;
        }
        
        std::string probBlob = format.neuralnetworkclassifier().labelprobabilitylayername();
        // Check if the probability blob name was provided in the proto
        if (probBlob.compare("") != 0) {
            // Check if it corresponds to some output of the network
            if (outputBlobNames.find(probBlob) == outputBlobNames.end()) {
                std::string err = "For this neural network classifier, the probabilities are obtained from the layer '" + probBlob + "' which was not found in the network.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
        
        // Now, we need to check that all the model's output names are either blob names or the extra outputs
        // for a classifier
        for (const auto& output : format.description().output()) {
            // is it not an output blob?
            if (outputBlobNames.find(output.name()) == outputBlobNames.end()) {
                if (output.name().compare(format.description().predictedfeaturename()) != 0
                    && output.name().compare(format.description().predictedprobabilitiesname()) != 0) {
                    std::string err = "Output layer '" + output.name() + "' is not produced by any layer of the neural network.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
        
        return r;
        
    }
    
    template <>
    Result validate<MLModelType_neuralNetworkRegressor>(const Specification::Model& format) {
        // must have regressor parameters
        Result r = validateRegressorInterface(format.description(), format.specificationversion());
        if (!r.good()) {
            return r;
        }
        
        std::set<std::string> outputBlobNames;
        return validateNeuralNetworkTopLevel(format.description(), format.neuralnetworkregressor(), outputBlobNames, format.isupdatable());
    }
    
    template <>
    Result validate<MLModelType_neuralNetwork>(const Specification::Model& format) {
        
        
        
        const auto& interface = format.description();
        
        // This isn't true for classifiers and regressors -- need to template specialize it to make these work
        if (!std::all_of(interface.output().begin(),
                         interface.output().end(),
                         [](const Specification::FeatureDescription& output) {
                             return output.type().Type_case() == Specification::FeatureType::kMultiArrayType ||
                             output.type().Type_case() == Specification::FeatureType::kImageType;
                         })) {
                             return Result(ResultType::INVALID_MODEL_INTERFACE,
                                           "Neural Network outputs must be either an image or MLMultiArray.");
                         }
        
        std::set<std::string> outputBlobNames;
        
        Result r = validateNeuralNetworkTopLevel(format.description(), format.neuralnetwork(), outputBlobNames, format.isupdatable());
        
        if (r.good()) {
            // Make sure that all of the model interface's outputs are actually produced by some blob
            for (const auto& output : format.description().output()) {
                
                const std::string& name = output.name();
                
                std::string err;
                if (outputBlobNames.count(name) == 0) {
                    err = "Interface specifies output '" + name + "' which is not produced by any layer in the neural network.";
                    return Result(ResultType::INVALID_MODEL_INTERFACE, err);
                }
                outputBlobNames.erase(name);
                
            }
        }
        
        return r;
        
    }


}



