//
//  NeuralNetworkLayerValidator.cpp
//  mlmodel
//
//  Created by aseem wadhwa on 10/27/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#include "NeuralNetworkValidator.hpp"
#include "NeuralNetworkValidatorUtils.hpp"

#include <algorithm>
#include <sstream>

using namespace CoreML;

//    ConvolutionLayerParams convolution = 4;
Result NeuralNetworkSpecValidator::validateConvolutionLayer(const Specification::NeuralNetworkLayer& layer) {
    
    Result r;
    r = validateInputCount(layer, 1, 2);
    if (!r.good()) {return r;}
    
    r = validateOutputCount(layer, 1, 1);
    if (!r.good()) {return r;}
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Convolution", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Convolution", 4, -1, blobNameToRank);
        if (!r.good()) {return r;}
    } else {
        if (layer.input_size() != 1) {
            std::string err = "Convolution Layer '" + layer.name() +
            "' does not support weight as input tensor when RANK5_ARRAY_MAPPING == true.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    
    // We need to check if the ConvolutionPaddingType is set
    if (layer.convolution().ConvolutionPaddingType_case() == Specification::ConvolutionLayerParams::ConvolutionPaddingTypeCase::CONVOLUTIONPADDINGTYPE_NOT_SET) {
        std::string err = "Padding type for convolution layer '" + layer.name() + "' is not set.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    const auto& params = layer.convolution();
    bool is_deconv = params.isdeconvolution();
    if (is_deconv && layer.input_size() != 1) {
        std::string err = "Deconvolution Layer '" + layer.name() + "' does not support weight as input tensor.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }

    if (layer.input_size() != 1 && (
        ((params.dilationfactor_size() > 0 && params.dilationfactor(0) > 1) ||
        (params.dilationfactor_size() > 1 && params.dilationfactor(1) > 1))
        )) {
        std::string err = "Convolution layer: '" + layer.name() + "' , dilated convolution does not support weight as input tensor.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }

    uint64_t kernelChannels = params.kernelchannels();
    uint64_t outputChannels = params.outputchannels();
    uint64_t nGroups = params.ngroups();
    if (nGroups == 0) {
        // default value specified in protobuf
        nGroups = 1;
    }
    uint64_t kernelHeight;
    if (params.kernelsize_size() > 0) {
        kernelHeight = params.kernelsize(0);
    }
    else {
        // this is the default specified in the protobuf file
        kernelHeight = 3;
    }
    uint64_t kernelWidth;
    if (params.kernelsize_size() > 1) {
        kernelWidth = params.kernelsize(1);
    }
    else {
        kernelWidth = 3;
    }

    if (layer.input_size() > 1) {
        return r;
    }

    bool has_bias = params.hasbias();
    if (has_bias && layer.input_size() != 1) {
        std::string err = "Convolution layer: '" + layer.name() + "' with dynamic weight does not support static bias.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }

    WeightParamType weightsValueType, biasValueType;
    weightsValueType = valueType(params.weights());
    biasValueType = valueType(params.bias());
    
    // Check weight/bias value types. Only float32 or float16 parameters can be populated at any time
    if ( (weightsValueType == UNSPECIFIED) || (has_bias && biasValueType == UNSPECIFIED)) {
        std::string err = "Convolution layer '" + layer.name() + "'  has invalid weights/bias fields.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    if (has_bias){
        if ((weightsValueType == CoreML::FLOAT16 && biasValueType == CoreML::FLOAT32) ||
            (weightsValueType == CoreML::FLOAT32 && biasValueType == CoreML::FLOAT16)){
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Convolution layer " + layer.name() +
                       "has unmatched precisions of weights/bias They should either be half or full precision.");
            return r;
        }
    }
    
    // Get populated weight and bias sizes
    // Check weights
    uint64_t expected_weight_size = 0;
    // conv: outputChannels, kernelChannels, kernelHeight, kernelWidth
    // deconv: kernelChannels, outputChannels / nGroups, kernelHeight, kernelWidth
    if (is_deconv) {
        expected_weight_size = kernelChannels * (outputChannels / nGroups) * kernelHeight * kernelWidth;
    }
    else {
        expected_weight_size = outputChannels * kernelChannels * kernelHeight * kernelWidth;
    }
    uint64_t weight_size = 0;
    if (weightsValueType == FLOAT32 || weightsValueType == FLOAT16) {
        if (weightsValueType == FLOAT32) {
            weight_size = static_cast<uint64_t>(params.weights().floatvalue().size());
        } else {
            weight_size = static_cast<uint64_t>(params.weights().float16value().size() / 2);
        }
        if (weight_size != expected_weight_size) {
            if (is_deconv) {
                std::string err = "Deconvolution layer '" + layer.name() + "' has weight matrix of size " + std::to_string(weight_size) + " to encode a " + std::to_string(kernelChannels) + " x " + std::to_string(outputChannels/nGroups) + " x " + std::to_string(kernelHeight) + " x " + std::to_string(kernelWidth) + " convolution.";
                r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                return r;
            }
            else {
                std::string err = "Convolution layer '" + layer.name() + "' has weight matrix of size " + std::to_string(weight_size) + " to encode a " + std::to_string(outputChannels) + " x " + std::to_string(kernelChannels) + " x " + std::to_string(kernelHeight) + " x " + std::to_string(kernelWidth) + " convolution.";
                r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                return r;
            }
        }
    } // if (weightsValueType == FLOAT32 || weightsValueType == FLOAT16)
    else if (weightsValueType == QUINT) {
        r = validateGeneralWeightParams(params.weights(), expected_weight_size, outputChannels, "Convolution", layer.name(), "weight");
        if (!r.good()) return r;
    } else { // EMPTY
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Layer " + layer.name() + "has not specified weights.");
        return r;
    }
    
    // Check the bias
    uint64_t bias_size = 0;
    if (has_bias) {
        if (biasValueType == FLOAT32 || biasValueType == FLOAT16){
            if (biasValueType == FLOAT32){
                bias_size = static_cast<uint64_t>(params.bias().floatvalue().size());
            } else {
                bias_size = static_cast<uint64_t>(params.bias().float16value().size() / 2);
            }
            if (bias_size != outputChannels) {
                std::string err = "Convolution layer '" + layer.name() + "' has a bias vector of size " +
                std::to_string(bias_size) + " but should be " + std::to_string(outputChannels) + ".";
                r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                return r;
            }
        } else if (biasValueType == QUINT){
            // quantization of bias vector should be 1
            r = validateGeneralWeightParams(params.bias(), outputChannels, 1, "Convolution", layer.name(), "bias");
            if (!r.good()) return r;
        } else { // EMPTY
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Layer " + layer.name() + "has not specified bias.");
            return r;
        }
    }
    return r;
}

static Result validateInnerProductWeightsBias(const Specification::NeuralNetworkLayer& layer,
                                              const Specification::WeightParams& weights,
                                              const Specification::WeightParams& bias) {
    
    Result r;
    
    bool has_bias;
    uint64_t num_inputs;
    uint64_t num_outputs;
    std::string layer_type;
    
    switch(layer.layer_case()) {
        case Specification::NeuralNetworkLayer::LayerCase::kInnerProduct: {
            const auto& params = layer.innerproduct();
            num_inputs = params.inputchannels();
            num_outputs = params.outputchannels();
            has_bias = params.hasbias();
            layer_type = "Inner product";
            break;
        }
        case Specification::NeuralNetworkLayer::LayerCase::kBatchedMatmul: {
            const auto& params = layer.batchedmatmul();
            num_inputs = params.weightmatrixfirstdimension();
            num_outputs = params.weightmatrixseconddimension();
            has_bias = params.hasbias();
            layer_type = "BatchedMatMul";
            break;
        }
        default:
            return r;
    }
    
    WeightParamType weightsValueType, biasValueType;
    weightsValueType = valueType(weights);
    biasValueType = valueType(bias);
    
    // Check for weight and bias value type
    if ((weightsValueType == UNSPECIFIED) || (has_bias && biasValueType == UNSPECIFIED)) {
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, layer_type + " layer '" + layer.name() + "' has invalid weights/bias fields.");
        return r;
    }
    // Check that weight and bias should both be FP16/32, or quantized
    if (has_bias){
        if ((weightsValueType == CoreML::FLOAT16 && biasValueType == CoreML::FLOAT32) ||
            (weightsValueType == CoreML::FLOAT32 && biasValueType == CoreML::FLOAT16)){
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, layer_type + " layer '" + layer.name() +
                       "has unmatched precisions of weights/bias They should either be half or full precision.");
            return r;
        }
    }
    
    // Check weights
    uint64_t weight_size = 0;
    if (weightsValueType == FLOAT32 || weightsValueType == FLOAT16){
        if (weightsValueType == FLOAT32) {
            weight_size = static_cast<uint64_t>(weights.floatvalue().size());
        } else {
            weight_size = static_cast<uint64_t>(weights.float16value().size() / 2);
        }
        if (num_inputs * num_outputs != weight_size) {
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Layer '" + layer.name() +
                       " has incorrect weight matrix size " + std::to_string(weight_size) +
                       " to encode a " + std::to_string(num_inputs) + " x " +
                       std::to_string(num_outputs) + " " + layer_type + ".");
            return r;
        }
    } else if (weightsValueType == QUINT){
        r = validateGeneralWeightParams(weights, num_inputs * num_outputs, num_outputs,
                                        layer_type, layer.name(), "weight");
        if (!r.good()) return r;
    }
    
    // Check the bias
    uint64_t bias_size = 0;
    if (has_bias){
        if (biasValueType == FLOAT32 || biasValueType == FLOAT16){
            if (biasValueType == FLOAT32){
                bias_size = static_cast<uint64_t>(bias.floatvalue().size());
            } else {
                bias_size = static_cast<uint64_t>(bias.float16value().size() / 2);
            }
            if (bias_size != num_outputs) {
                std::string err = "Layer '" + layer.name() + "' has incorrect bias vector size " +
                std::to_string(bias_size) + " (expected " + std::to_string(num_outputs) + ").";
                r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                return r;
            }
        } else if (biasValueType == QUINT){
            r = validateGeneralWeightParams(bias, num_outputs, 1, layer_type,
                                            layer.name(), "bias");
            if (!r.good()) return r;
        }
    } else if (!has_bias && bias_size > 0) {
        std::string err = "Bias vector being ignored since \"hasBias\" flag not set.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    return r;
}

//    InnerProductLayerParams innerProduct = 5;
Result NeuralNetworkSpecValidator::validateInnerProductLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    if (!r.good()) {
        return r;
    }
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "InnerProduct", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "InnerProduct", 1, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.innerproduct();
    
    r = validateInnerProductWeightsBias(layer, params.weights(), params.bias());
    
    return r;
}

//    BatchnormLayerParams batchnorm = 6;
Result NeuralNetworkSpecValidator::validateBatchnormLayer(const Specification::NeuralNetworkLayer& layer) {
    
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Batchnorm", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Batchnorm", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    // Check parameters types
    bool has_f32_params = ((valueType(layer.batchnorm().gamma()) == FLOAT32) || (valueType(layer.batchnorm().beta()) == FLOAT32) ||
                           (valueType(layer.batchnorm().mean()) == FLOAT32)  || (valueType(layer.batchnorm().variance()) == FLOAT32));
    bool has_f16_params = ((valueType(layer.batchnorm().gamma()) == FLOAT16) || (valueType(layer.batchnorm().beta()) == FLOAT16) ||
                           (valueType(layer.batchnorm().mean()) == FLOAT16)  || (valueType(layer.batchnorm().variance()) == FLOAT16));
    bool invalid_params = ((valueType(layer.batchnorm().gamma()) == UNSPECIFIED) || (valueType(layer.batchnorm().beta()) == UNSPECIFIED) ||
                           (valueType(layer.batchnorm().mean()) == UNSPECIFIED)  || (valueType(layer.batchnorm().variance()) == UNSPECIFIED));
    if ((has_f32_params && has_f16_params) || invalid_params) {
        std::string err = "Batchnorm layer '" + layer.name() + "' parameters have values for both full and half precision. Parameters "
        "should either be specified in half or full precision, mixed parameters are not supported.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    // Check parameters length
    uint64_t num_channels = static_cast<uint64_t>(layer.batchnorm().channels());
    r = validateGeneralWeightParams(layer.batchnorm().gamma(), num_channels, 1, "BatchNorm", layer.name(), "gamma");
    if (!r.good()) return r;
    r = validateGeneralWeightParams(layer.batchnorm().beta(), num_channels, 1, "BatchNorm", layer.name(), "beta");
    if (!r.good()) return r;
    // Check existence of mean / variance
    if (!layer.batchnorm().computemeanvar()) {
        if (valueType(layer.batchnorm().mean()) == EMPTY || valueType(layer.batchnorm().mean()) == EMPTY){
            const std::string err = "Batchnorm layer '" + layer.name() + "' is missing mean and variance.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        r = validateGeneralWeightParams(layer.batchnorm().mean(), num_channels, 1, "BatchNorm", layer.name(), "mean");
        if (!r.good()) return r;
        r = validateGeneralWeightParams(layer.batchnorm().variance(), num_channels, 1, "BatchNorm", layer.name(), "variance");
        if (!r.good()) return r;
    }
    return r;
}

//    ActivationLayerParams activation = 6;
Result NeuralNetworkSpecValidator::validateActivation(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()) {return r;}
    if (ndArrayInterpretation) {
        if (layer.activation().NonlinearityType_case() == Specification::ActivationParams::NonlinearityTypeCase::kPReLU) {
            r = validateInputOutputRankEquality(layer, "ActivationPReLU", blobNameToRank);
            if (!r.good()) {return r;}
            r = validateRankCount(layer, "ActivationPReLU", 3, -1, blobNameToRank);
            if (!r.good()) {return r;}
        }
        if (layer.activation().NonlinearityType_case() == Specification::ActivationParams::NonlinearityTypeCase::kParametricSoftplus) {
            r = validateInputOutputRankEquality(layer, "ActivationParametricSoftplus", blobNameToRank);
            if (!r.good()) {return r;}
            r = validateRankCount(layer, "ActivationParametricSoftplus", 3, -1, blobNameToRank);
            if (!r.good()) {return r;}
        }
    }
    
    return validateActivationParams(layer.activation());
}

//    PoolingLayerParams pooling = 8;
Result NeuralNetworkSpecValidator::validatePoolingLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (!r.good()) {return r;}
    
    r = validateOutputCount(layer, 1, 1);
    if (!r.good()) {return r;}
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Pooling", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Pooling", 4, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    // We need to check if the PoolingPaddingType is set
    if (layer.pooling().PoolingPaddingType_case() == Specification::PoolingLayerParams::PoolingPaddingTypeCase::POOLINGPADDINGTYPE_NOT_SET) {
        std::string err = "Padding type for the pooling layer '" + layer.name() + "' is not set.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    return r;
}

//    PaddingLayerParams padding = 9;
Result NeuralNetworkSpecValidator::validatePaddingLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Padding", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Padding", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.padding();
    if (!(params.paddingamounts().borderamounts_size() == 0
          || params.paddingamounts().borderamounts_size() == 2)) {
        std::string err = "Padding layer " + layer.name() + " specifies " + std::to_string(params.paddingamounts().borderamounts_size()) + " padding amounts but it must either specify 2 (for x and y axes), or 0 for the default values.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    if (params.PaddingType_case() == Specification::PaddingLayerParams::PaddingTypeCase::PADDINGTYPE_NOT_SET) {
        std::string err = "Padding layer " + layer.name() + " padding type is not set.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    return r;
}

//    LRNLayerParams lrn = 11;
Result NeuralNetworkSpecValidator::validateLRNLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "LRNLayer", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "LRNLayer", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    if (layer.lrn().k() < 0.0) {
        std::string err = "Parameter 'K' for the LRN layer '" + layer.name() + "' must be positive.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    return r;
}

//    SplitLayerParams split = 13;
Result NeuralNetworkSpecValidator::validateSplitLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        // between 2 and any number of outputs
        r = validateOutputCount(layer, 2, -1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Split", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Split", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
        
        // check that all outputs have same rank
        int rank = 0;
        if (blobNameToRank.find(layer.output(0)) != blobNameToRank.end()) {
            rank = blobNameToRank.at(layer.output(0));
        } else {
            return r;
        }
        
        for (const auto& output : layer.output()) {
            if (blobNameToRank.find(output) != blobNameToRank.end()) {
                if (rank != blobNameToRank.at(output)) {
                    std::string err;
                    err = "Layer '" + std::string(layer.name()) + "' of type 'Split' must have equal ranks for its outputs, but they are not equal.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
    }
    
    return r;
}

//    AddLayerParams add = 14;
Result NeuralNetworkSpecValidator::validateAddLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    // 1 or more inputs
    r = validateInputCount(layer, 1, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

//    MultiplyLayerParams multiply = 15;
Result NeuralNetworkSpecValidator::validateMultiplyLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    // 1 or more inputs
    r = validateInputCount(layer, 1, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

//    UnaryFunctionLayerParams unary = 16;
Result NeuralNetworkSpecValidator::validateUnaryFunctionLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Unary", blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    return r;
}

//    UpsampleLayerParams upsample = 17;
Result NeuralNetworkSpecValidator::validateUpsampleLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Upsample", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Upsample", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.upsample();
    // scaling factor must be 2D if provided
    if (!(params.scalingfactor_size() == 0 || params.scalingfactor_size() == 2)) {
        std::string err = "Scaling factor in the upsampling layer '" + layer.name() + "' must be a vector of size 2 (i.e height, width) but is a vector of size " + std::to_string(params.scalingfactor_size()) + ".";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    return r;
}

//    BiasLayerParams bias = 18;
Result NeuralNetworkSpecValidator::validateBiasLayer(const Specification::NeuralNetworkLayer& layer) {
    
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    if (!r.good()) {
        return r;
    }
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Bias", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Bias", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.bias();
    WeightParamType paramType = valueType(params.bias());
    
    // Only float32 or float16 parameters can be populated at any time
    if (paramType == UNSPECIFIED) {
        std::string err = "Bias product layer '" + layer.name() + "' has both full precision and half precision weights and/or bias fields populated";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }

    if (params.shape_size() != 1 && params.shape_size() != 3) {
        std::string err = "Bias layer '" + layer.name() + "' cannot be " + std::to_string(params.shape_size()) + " dimensional. Must be 1D or 3D.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }

    // shape can be ``[1]``, ``[C]``, ``[1, H, W]`` or ``[C, H, W]``
    size_t total_shape = 1;
    for (int i = 0; i < params.shape_size(); i++) {
        total_shape *= params.shape(i);
    }
    if (params.shape_size() == 3 && params.shape(0) > 1){
        r = validateGeneralWeightParams(params.bias(), total_shape, params.shape(0), "Bias", layer.name(), "bias");
    } else {
        r = validateGeneralWeightParams(params.bias(), total_shape, 1, "Bias", layer.name(), "bias");
    }

    return r;
}

//    L2NormLayerParams l2norm = 19;
Result NeuralNetworkSpecValidator::validateL2NormLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "L2Normalize", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "L2Normalize", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    return r;
}

//    ReshapeLayerParams reshape = 20;
Result NeuralNetworkSpecValidator::validateReshapeLayer(const Specification::NeuralNetworkLayer& layer) {
    
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Reshape", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Reshape", 5, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.reshape();
    if (params.targetshape_size() != 3 && params.targetshape_size() != 4) {
        std::string err = "Reshape layer '" + layer.name() + "' target shape must be 3D or 4D.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    return r;
}

//    FlattenLayerParams flatten = 21;
Result NeuralNetworkSpecValidator::validateFlattenLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Flatten", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Flatten", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    return r;
}

//    PermuteLayerParams permute = 22;
Result NeuralNetworkSpecValidator::validatePermuteLayer(const Specification::NeuralNetworkLayer& layer) {
    
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Permute", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Permute", 5, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.permute();
    if (params.axis_size() != 4) {
        std::string err = "Permute layer '" + layer.name() + "' must have 4D axis parameters.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    return r;
}

//    ReduceLayerParams reduce = 23;
Result NeuralNetworkSpecValidator::validateReduceLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (ndArrayInterpretation && layer.inputtensor_size() > 0) {
        r = validateInputOutputRankEquality(layer, "Reduce", blobNameToRank);
        if (!r.good()) {return r;}
        int rank = static_cast<int>(layer.inputtensor(0).rank());
        bool sufficientInputRank = true;
        std::string err;
        
        switch (layer.reduce().axis()) {
            case Specification::ReduceLayerParams::CHW:
                if (rank < 3) {sufficientInputRank = false;}
                break;
            case Specification::ReduceLayerParams::HW:
                if (rank < 2) {sufficientInputRank = false;}
                break;
            case Specification::ReduceLayerParams::H:
                if (rank < 1) {sufficientInputRank = false;}
                break;
            case Specification::ReduceLayerParams::W:
                if (rank < 1) {sufficientInputRank = false;}
                break;
            case Specification::ReduceLayerParams::C:
                if (rank < 1) {sufficientInputRank = false;}
                break;
            case CoreML::Specification::ReduceLayerParams_ReduceAxis_ReduceLayerParams_ReduceAxis_INT_MIN_SENTINEL_DO_NOT_USE_:
            case CoreML::Specification::ReduceLayerParams_ReduceAxis_ReduceLayerParams_ReduceAxis_INT_MAX_SENTINEL_DO_NOT_USE_:
                err = "Reduce layer: '" + std::string(layer.name()) + "': unknown value for parameter 'axis'.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (!sufficientInputRank) {
            err = "Reduce layer '" + std::string(layer.name()) + "': input's rank is smaller than the dimensions provided in the axis parameter";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateReorganizeDataLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "ReorganizeData", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "ReorganizeData", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& reorg = layer.reorganizedata();
    if (static_cast<int>(reorg.blocksize()) < 2) {
        std::string err = "Block size for layer '" + layer.name() + "' must be > 1.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSliceLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (ndArrayInterpretation && layer.inputtensor_size() > 0) {
        r = validateInputOutputRankEquality(layer, "Slice", blobNameToRank);
        if (!r.good()) {return r;}
        int rank = static_cast<int>(layer.inputtensor(0).rank());
        bool sufficientInputRank = true;
        std::string err;
        
        switch (layer.slice().axis()) {
            case Specification::SliceLayerParams_SliceAxis_CHANNEL_AXIS:
                if (rank < 3) {sufficientInputRank = false;}
                break;
            case Specification::SliceLayerParams_SliceAxis_HEIGHT_AXIS:
                if (rank < 2) {sufficientInputRank = false;}
                break;
            case Specification::SliceLayerParams_SliceAxis_WIDTH_AXIS:
                if (rank < 1) {sufficientInputRank = false;}
                break;
            default:
                err = "Slice layer: '" + std::string(layer.name()) + "': unknown value for parameter 'axis'.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (!sufficientInputRank) {
            err = "Slice layer '" + std::string(layer.name()) + "': input's rank is smaller than the dimension provided in the axis parameter";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    
    const auto& slice = layer.slice();
    int stride = static_cast<int>(slice.stride());
    if (stride < 1) {
        std::string err = "Stride length for the slice layer '" + layer.name() + "' must be > 1.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    int64_t start = (int64_t)slice.startindex();
    int64_t end = slice.endindex();
    if ((end > 0 && end < start )
        || (end < 0 && start < 0 && start > end)) {
        std::string err = "Slice layer " + layer.name() + " has an end index before the start index.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    return r;
}

//    LoadConstantLayerParams loadConstant = 24;
Result NeuralNetworkSpecValidator::validateLoadConstantLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 0);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    if (!r.good()) {
        return r;
    }
    
    if (ndArrayInterpretation) {
        if (blobNameToRank.find(layer.output(0)) != blobNameToRank.end()) {
            int rank = blobNameToRank.at(layer.output(0));
            r = checkRank(layer, "LoadConstant", 5, 5, "output", rank);
            if (!r.good()) {return r;}
        }
    }
    
    const auto& params = layer.loadconstant();
    WeightParamType paramType = valueType(params.data());
    
    // Only float32 or float16 parameters can be populated at any time
    if (paramType == UNSPECIFIED) {
        std::string err = "Load constant layer '" + layer.name() + "' has both full precision and half precision weight fields populated";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    if (params.shape_size() != 3) {
        std::string err = "Load constant layer '" + layer.name() + "' must be a 3D constant.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    size_t total_shape = 1;
    for (int i = 0; i < params.shape_size(); i++) {
        total_shape *= params.shape(i);
    }
    if (params.shape_size() == 3 && params.shape(0) > 1){
        r = validateGeneralWeightParams(params.data(), total_shape, params.shape(0), "LoadConstant", layer.name(), "constants");
    } else {
        r = validateGeneralWeightParams(params.data(), total_shape, 1, "LoadConstant", layer.name(), "constants");
    }
    
    if (!r.good()) return r;
    
    return Result();
}

//    ScaleLayerParams scale = 25;
Result NeuralNetworkSpecValidator::validateScaleLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    if (!r.good()) {
        return r;
    }
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Scale", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Scale", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.scale();
    
    bool has_bias = params.hasbias();
    WeightParamType scaleValueType, biasValueType;
    scaleValueType = valueType(params.scale());
    biasValueType = valueType(params.bias());
    
    // Check for scale and bias value type. Only float32 or float16 parameters can be populated at any time.
    // Both bias and weights should have the same value types
    if ( (scaleValueType == UNSPECIFIED) || (has_bias && biasValueType == UNSPECIFIED)) {
        std::string err = "Scale layer '" + layer.name() + "' has invalid scale/bias fields.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    if (has_bias){
        if ((scaleValueType == CoreML::FLOAT16 && biasValueType == CoreML::FLOAT32) ||
            (scaleValueType == CoreML::FLOAT32 && biasValueType == CoreML::FLOAT16)){
            const std::string err = "Scale layer '" + layer.name() +
            "' has invalid scale/bias fields. Field value types should match and should either be half or full precision.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }
    }
    
    // Checks scale shape and size
    if (!(params.shapescale_size() == 1 || params.shapescale_size() == 3)) { // check shape
        std::string err = "The shape vector for the scale layer '" + layer.name() + "' is " +
        std::to_string(params.shapescale_size()) + " dimensional but should be 1D or 3D.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    size_t total_scale_shape = 1;
    for (int i = 0; i < params.shapescale_size(); i++) {
        total_scale_shape *= params.shapescale(i);
    }
    
    if (params.shapescale_size() == 3 && params.shapescale(0) > 1){
        r = validateGeneralWeightParams(params.scale(), total_scale_shape, params.shapescale(0), "Scale", layer.name(), "scale");
    } else {
        r = validateGeneralWeightParams(params.scale(), total_scale_shape, 1, "Scale", layer.name(), "scale");
    }
    if (!r.good()) return r;
    
    // Checks bias shape and size
    if (has_bias) {
        if (!(params.shapebias_size() == 1 || params.shapebias_size() == 3)) {
            std::string err = "The bias vector for scale layer '" + layer.name() + "' is " +
            std::to_string(params.shapebias_size()) + " dimensional but should be either 1D or 3D.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        size_t total_bias_shape = 1;
        for (int i = 0; i < params.shapebias_size(); i++) {
            total_bias_shape *= params.shapebias(i);
        }
        if (params.shapebias_size() == 3 && params.shapebias(0) > 1){
            r = validateGeneralWeightParams(params.bias(), total_bias_shape, params.shapebias(0), "Scale", layer.name(), "bias");
        } else {
            r = validateGeneralWeightParams(params.bias(), total_bias_shape, 1, "Scale", layer.name(), "bias");
        }
        if (!r.good()) return r;
    }
    return Result();
}

//    SimpleRecurrentLayerParams simpleRecurrent = 26;
Result NeuralNetworkSpecValidator::validateSimpleRecurrentLayer(const Specification::NeuralNetworkLayer& layer) {
    
    Result r;
    // Must specify hidden state
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 2, 2);
    }
    if (!r.good()) {
        return r;
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "SimpleRecurrent", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "SimpleRecurrent", 5, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.simplerecurrent();
    bool hasBiasVector = params.hasbiasvector();
    WeightParamType weightMatrixValueType, recursionMatrixValueType, biasVectorValueType;
    weightMatrixValueType = valueType(params.weightmatrix());
    recursionMatrixValueType = valueType(params.recursionmatrix());
    biasVectorValueType = valueType(params.biasvector());
    
    // Verify all weights are of valid type
    if((weightMatrixValueType == UNSPECIFIED) || recursionMatrixValueType == UNSPECIFIED || (hasBiasVector && biasVectorValueType == UNSPECIFIED)){
        std::string err = "Simple recurrent layer '" + layer.name() + "' has invalid weightMatrix/recusionMatrix/Bias fields.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    // Verify either weightMatrix, recursionMatrix, and biasVector are all FP32 or FP16, or one of them is quantized
    if (weightMatrixValueType != QUINT && recursionMatrixValueType != QUINT){
        if (weightMatrixValueType != recursionMatrixValueType ||
            (hasBiasVector && biasVectorValueType != QUINT && weightMatrixValueType != biasVectorValueType) ){
            std::string err = "Simple recurrent layer '" + layer.name() + "' has invalid weightMatrix/recusionMatrix/Bias fields. "
            "Field value types should match and should either be half or full precision.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }
    }
    
    // Check weight matrix size
    // input matrix
    uint64_t input_matrix_size = params.inputvectorsize() * params.outputvectorsize();
    r = validateGeneralWeightParams(params.weightmatrix(), input_matrix_size,
                                    params.outputvectorsize(),
                                    "SimpleRNN", layer.name(), "WeightMatrix");
    if (!r.good()) return r;
    // recurrent matrix
    uint64_t recurrent_matrix_size = params.outputvectorsize() * params.outputvectorsize();
    r = validateGeneralWeightParams(params.recursionmatrix(), recurrent_matrix_size,
                                    params.outputvectorsize(), "SimpleRNN",
                                    layer.name(), "RecursionMatrix");
    if (!r.good()) return r;
    // bias
    if (hasBiasVector){
        r = validateGeneralWeightParams(params.biasvector(), params.outputvectorsize(), 1,
                                        "SimpleRNN", layer.name(), "BiasVector");
        if (!r.good()) return r;
    }
    // Validate the activations as well
    return validateRecurrentActivationParams(layer.simplerecurrent().activation());
    
}

//    GRULayerParams gru = 27;
Result NeuralNetworkSpecValidator::validateGRULayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    
    // Must specify hidden states
    r = validateInputCount(layer, 1, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 2, 2);
    }
    if (!r.good()) {
        return r;
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "GRU", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "GRU", 5, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.gru();
    bool hasBiasVectors = params.hasbiasvectors();
    
    std::vector<CoreML::WeightParamType> weightTypeList;
    weightTypeList.push_back(valueType(params.updategateweightmatrix()));
    weightTypeList.push_back(valueType(params.updategaterecursionmatrix()));
    weightTypeList.push_back(valueType(params.resetgateweightmatrix()));
    weightTypeList.push_back(valueType(params.resetgaterecursionmatrix()));
    weightTypeList.push_back(valueType(params.outputgateweightmatrix()));
    weightTypeList.push_back(valueType(params.outputgaterecursionmatrix()));
    if(hasBiasVectors){
        weightTypeList.push_back(valueType(params.updategatebiasvector()));
        weightTypeList.push_back(valueType(params.resetgatebiasvector()));
        weightTypeList.push_back(valueType(params.outputgatebiasvector()));
    }
    if(!isWeightParamTypeCompatible(weightTypeList)){
        const std::string err = "GRU layer '" + layer.name() + "' has invalid weight/recursion matrix or bias fields. "
        "Field value types should match and should be either half or full precision";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    // allen-continue
    
    
    // Check the size of the input matrices
    const uint64_t input_matrix_size = params.inputvectorsize() * params.outputvectorsize();
    const uint64_t outSize = params.outputvectorsize();
    r = validateGeneralWeightParams(params.updategateweightmatrix(), input_matrix_size,
                                    outSize, "GRU", layer.name(), "update gate weight matrix");
    if (!r.good()) return r;
    r = validateGeneralWeightParams(params.resetgateweightmatrix(), input_matrix_size,
                                    outSize, "GRU", layer.name(), "reset gate weight matrix");
    if (!r.good()) return r;
    r = validateGeneralWeightParams(params.outputgateweightmatrix(), input_matrix_size,
                                    outSize, "GRU", layer.name(), "output gate weight matrix");
    if (!r.good()) return r;
    
    // Check the size of the recurrent matrices
    const uint64_t recurrent_matrix_size = params.outputvectorsize() * params.outputvectorsize();
    r = validateGeneralWeightParams(params.updategaterecursionmatrix(), recurrent_matrix_size,
                                    outSize, "GRU", layer.name(), "update gate recursion matrix");
    if (!r.good()) return r;
    r = validateGeneralWeightParams(params.resetgaterecursionmatrix(), recurrent_matrix_size,
                                    outSize, "GRU", layer.name(), "reset gate recursion matrix");
    if (!r.good()) return r;
    r = validateGeneralWeightParams(params.outputgaterecursionmatrix(), recurrent_matrix_size,
                                    outSize, "GRU", layer.name(), "output gate recursion matrix");
    if (!r.good()) return r;
    
    if (hasBiasVectors){
        const uint64_t bias_size = params.outputvectorsize();
        r = validateGeneralWeightParams(params.updategatebiasvector(), bias_size, 1,
                                        "GRU", layer.name(), "update gate bias vector");
        if (!r.good()) return r;
        r = validateGeneralWeightParams(params.resetgatebiasvector(), bias_size, 1,
                                        "GRU", layer.name(), "reset gate bias vector");
        if (!r.good()) return r;
        r = validateGeneralWeightParams(params.outputgatebiasvector(), bias_size, 1,
                                        "GRU", layer.name(), "output gate bias vector");
        if (!r.good()) return r;
    }
    
    // Now check the activations
    for (const auto& activation : params.activations()) {
        r = validateRecurrentActivationParams(activation);
        if (!r.good()) {
            break;
        }
    }
    return r;
}

//    UniDirectionalLSTMLayerParams uniDirectionalLSTM = 28;
Result NeuralNetworkSpecValidator::validateUniDirectionalLSTMLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    // Must specify hidden states
    r = validateInputCount(layer, 1, 3);
    if (r.good()) {
        r = validateOutputCount(layer, 3, 3);
    }
    if (!r.good()) {
        return r;
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "UniDirectionalLSTM", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "UniDirectionalLSTM", 5, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    for (const auto& activation : layer.unidirectionallstm().activations()) {
        r = validateRecurrentActivationParams(activation);
        if (!r.good()) {
            break;
        }
    }
    
    // Validate common LSTM params and ensure that all weight field types are consistent
    r = validateLSTMWeightParams(layer.unidirectionallstm().weightparams(), layer.unidirectionallstm().params());
    if (!r.good()) {
        return r;
    }
    
    Specification::UniDirectionalLSTMLayerParams recurrent = layer.unidirectionallstm();
    uint64_t x = recurrent.inputvectorsize();
    uint64_t h = recurrent.outputvectorsize();
    
    if (recurrent.activations_size() != 3){
        const std::string err = std::string("Unidirectional LSTM layer:" + layer.name() + " must provide 3 activations");
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    // Check weight matrices' sizes
    r = validateGeneralWeightParams(recurrent.weightparams().inputgateweightmatrix(), h*x, h,
                                    "Unidirectional LSTM", layer.name(), "input gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(recurrent.weightparams().forgetgateweightmatrix(), h*x, h,
                                    "Unidirectional LSTM", layer.name(), "forget gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(recurrent.weightparams().blockinputweightmatrix(), h*x, h,
                                    "Unidirectional LSTM", layer.name(), "block input gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(recurrent.weightparams().outputgateweightmatrix(), h*x, h,
                                    "Unidirectional LSTM", layer.name(), "output gate weight matrix");
    if(!r.good()) return r;
    // Check recursion matrices' sizes
    r = validateGeneralWeightParams(recurrent.weightparams().inputgaterecursionmatrix(), h*h, h,
                                    "Unidirectional LSTM", layer.name(), "input gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(recurrent.weightparams().forgetgaterecursionmatrix(), h*h, h,
                                    "Unidirectional LSTM", layer.name(), "forget gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(recurrent.weightparams().blockinputrecursionmatrix(), h*h, h,
                                    "Unidirectional LSTM", layer.name(), "block input gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(recurrent.weightparams().outputgaterecursionmatrix(), h*h, h,
                                    "Unidirectional LSTM", layer.name(), "output gate recursion matrix");
    if(!r.good()) return r;
    // Check bias vectors
    if (recurrent.params().hasbiasvectors()) {
        r = validateGeneralWeightParams(recurrent.weightparams().inputgatebiasvector(), h, 1,
                                        "Unidirectional LSTM", layer.name(), "input gate bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(recurrent.weightparams().forgetgatebiasvector(), h, 1,
                                        "Unidirectional LSTM", layer.name(), "forget gate bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(recurrent.weightparams().blockinputbiasvector(), h, 1,
                                        "Unidirectional LSTM", layer.name(), "block input bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(recurrent.weightparams().outputgatebiasvector(), h, 1,
                                        "Unidirectional LSTM", layer.name(), "output gate bias vector");
        if(!r.good()) return r;
    }
    // Check peephole vectors
    if (recurrent.params().haspeepholevectors()){
        r = validateGeneralWeightParams(recurrent.weightparams().inputgatepeepholevector(), h, 1,
                                        "Unidirectional LSTM", layer.name(), "input gate peep hole vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(recurrent.weightparams().forgetgatepeepholevector(), h, 1,
                                        "Unidirectional LSTM", layer.name(), "forget gate peep hole vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(recurrent.weightparams().outputgatepeepholevector(), h, 1,
                                        "Unidirectional LSTM", layer.name(), "output gate peep hole vector");
        if(!r.good()) return r;
    }
    
    return r;
}

//    BiDirectionalLSTMLayerParams biDirectionalLSTM = 29;
Result NeuralNetworkSpecValidator::validateBiDirectionalLSTMLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    // Must specify hidden states
    r = validateInputCount(layer, 1, 5);
    if (r.good()) {
        r = validateOutputCount(layer, 5, 5);
    }
    if (!r.good()) {
        return r;
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "BiDirectionalLSTM", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "BiDirectionalLSTM", 5, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    for (const auto& activation : layer.bidirectionallstm().activationsforwardlstm()) {
        r = validateRecurrentActivationParams(activation);
        if (!r.good()) {
            break;
        }
    }
    for (const auto& activation : layer.bidirectionallstm().activationsbackwardlstm()) {
        r = validateRecurrentActivationParams(activation);
        if (!r.good()) {
            break;
        }
    }
    
    // Validate common LSTM params and ensure that all weight field types are consistent
    r = validateLSTMWeightParams(layer.unidirectionallstm().weightparams(), layer.unidirectionallstm().params());
    if (!r.good()) {
        return r;
    }
    
    Specification::BiDirectionalLSTMLayerParams recurrent = layer.bidirectionallstm();
    Specification::LSTMParams lstmParams = recurrent.params();
    std::string err;
    
    if (recurrent.activationsforwardlstm_size() != 3) {
        err = std::string("Bidirectional LSTM layer:" + layer.name() + " forward lstm must provide 3 activations");
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    if (recurrent.activationsbackwardlstm_size() != 3){
        err = std::string("Bidirectional LSTM layer:" + layer.name() + " backward lstm must provide 3 activations");
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    // Verify weights and biases sizes
    uint64_t h = recurrent.outputvectorsize();
    uint64_t x = recurrent.inputvectorsize();
    const Specification::LSTMWeightParams& weightParamsF = recurrent.weightparams(0);
    const Specification::LSTMWeightParams& weightParamsB = recurrent.weightparams(1);
    
    // Check forward weight matrices' sizes
    r = validateGeneralWeightParams(weightParamsF.inputgateweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "forward input gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsF.forgetgateweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "forward forget gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsF.blockinputweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "forward block input gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsF.outputgateweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "forward output gate weight matrix");
    if(!r.good()) return r;
    // Check forward recursion matrices' sizes
    r = validateGeneralWeightParams(weightParamsF.inputgaterecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "forward input gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsF.forgetgaterecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "forward forget gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsF.blockinputrecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "forward block input gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsF.outputgaterecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "forward output gate recursion matrix");
    if(!r.good()) return r;
    // Check backward weight matrices' sizes
    r = validateGeneralWeightParams(weightParamsB.inputgateweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "backward input gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsB.forgetgateweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "backward forget gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsB.blockinputweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "backward block input gate weight matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsB.outputgateweightmatrix(), h*x, h,
                                    "Bidirectional LSTM", layer.name(), "backward output gate weight matrix");
    if(!r.good()) return r;
    // Check backward recursion matrices' sizes
    r = validateGeneralWeightParams(weightParamsB.inputgaterecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "backward input gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsB.forgetgaterecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "backward forget gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsB.blockinputrecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "backward block input gate recursion matrix");
    if(!r.good()) return r;
    r = validateGeneralWeightParams(weightParamsB.outputgaterecursionmatrix(), h*h, h,
                                    "Bidirectional LSTM", layer.name(), "backward output gate recursion matrix");
    if(!r.good()) return r;
    
    // Check bias vectors
    if (recurrent.params().hasbiasvectors()) {
        r = validateGeneralWeightParams(weightParamsF.inputgatebiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "forward input gate bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsF.forgetgatebiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "forward forget gate bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsF.blockinputbiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "forward block input bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsF.outputgatebiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "forward output gate bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsB.inputgatebiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "backward input gate bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsB.forgetgatebiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "backward forget gate bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsB.blockinputbiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "backward block input bias vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsB.outputgatebiasvector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "backward output gate bias vector");
        if(!r.good()) return r;
    }
    // Check peephole vectors
    if (recurrent.params().haspeepholevectors()){
        r = validateGeneralWeightParams(weightParamsF.inputgatepeepholevector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "forward input gate peephole vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsF.forgetgatepeepholevector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "forward forget gate peephole vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsF.outputgatepeepholevector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "forward output gate peephole vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsB.inputgatepeepholevector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "backward input gate peephole vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsB.forgetgatepeepholevector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "backward forget gate peephole vector");
        if(!r.good()) return r;
        r = validateGeneralWeightParams(weightParamsB.outputgatepeepholevector(), h, 1,
                                        "Bidirectional LSTM", layer.name(), "backward output gate peephole vector");
        if(!r.good()) return r;
    }
    return r;
}

//    CropLayerParams crop = 30;
Result NeuralNetworkSpecValidator::validateCropLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Crop", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Crop", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
        
        if (layer.input_size() > 1) {
            if (blobNameToRank.find(layer.input(0)) != blobNameToRank.end() &&
                blobNameToRank.find(layer.input(1)) != blobNameToRank.end()) {
                if (blobNameToRank.at(layer.input(0)) != blobNameToRank.at(layer.input(1))) {
                    std::string err;
                    err = "Layer '" + std::string(layer.name()) + "' of type 'Crop' expects equal ranks for its inputs, but they are not equal.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
    }
    
    if (layer.input_size() == 1) {
        // check the border amounts
        if (layer.crop().cropamounts().borderamounts_size() != 2) {
            std::string err = "cropAmounts parameter for the crop layer '" + layer.name() + "' is of length " + std::to_string(layer.crop().cropamounts().borderamounts_size()) + " but requires exactly two crop constraints (for X,Y axes).";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }
    }
    else { // size == 2 checked above
        // offset must be size 2
        if (layer.crop().offset_size() != 2)  {
            std::string err = "Offset parameter for the crop layer '" + layer.name() + "' is of length " + std::to_string(layer.crop().offset_size()) + " but requires exactly two offsets (for X,Y axes).";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }
    }
    
    return r;
}

//    DotProductLayerParams dot = 34;
Result NeuralNetworkSpecValidator::validateDotLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    // 2 inputs, 1 output
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "DotProduct", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "DotProduct", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
        
        if (blobNameToRank.find(layer.input(0)) != blobNameToRank.end() &&
            blobNameToRank.find(layer.input(1)) != blobNameToRank.end()) {
            if (blobNameToRank.at(layer.input(0)) != blobNameToRank.at(layer.input(1))) {
                std::string err;
                err = "Layer '" + std::string(layer.name()) + "' of type 'DotProduct' expects equal ranks for its inputs, but they are not equal.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
    }
    
    return r;
}

//    MeanVarianceNormalizeLayerParams mvn = 35;
Result NeuralNetworkSpecValidator::validateMvnLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "MeanVarianceNormalize", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "MeanVarianceNormalize", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    return r;
}


static Result validateEmbeddingWeightsBias(const Specification::NeuralNetworkLayer& layer,
                                              const Specification::WeightParams& weights,
                                              const Specification::WeightParams& bias) {
    
    Result r;
    
    bool has_bias;
    uint64_t input_dim;
    uint64_t output_channels;
    std::string layer_type;
    
    switch(layer.layer_case()) {
        case Specification::NeuralNetworkLayer::LayerCase::kEmbedding: {
            const auto& params = layer.embedding();
            input_dim = params.inputdim();
            output_channels = params.outputchannels();
            has_bias = params.hasbias();
            layer_type = "Embedding";
            break;
        }
        case Specification::NeuralNetworkLayer::LayerCase::kEmbeddingND: {
            const auto& params = layer.embeddingnd();
            input_dim = params.vocabsize();
            output_channels = params.embeddingsize();
            has_bias = params.hasbias();
            layer_type = "EmbeddingND";
            break;
        }
        default:
            return r;
    }
    
    WeightParamType weightsValueType, biasValueType;
    weightsValueType = valueType(weights);
    biasValueType = valueType(bias);
    
    // Only float32 or float16 parameters can be populated at any time
    if ((weightsValueType == UNSPECIFIED) || (has_bias && biasValueType == UNSPECIFIED)){
        std::string err = layer_type + " '" + layer.name() + "' has invalid weights/bias fields. Field value types should match and should either be half or full precision.";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    if (has_bias){
        if ((weightsValueType == CoreML::FLOAT16 && biasValueType == CoreML::FLOAT32) ||
            (weightsValueType == CoreML::FLOAT32 && biasValueType == CoreML::FLOAT16)){
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, layer_type + " layer '" + layer.name() +
                       "has unmatched precisions of weights/bias They should either be half or full precision.");
            return r;
        }
    }
    // Validate weight and bias sizes
    r = validateGeneralWeightParams(weights, input_dim * output_channels, output_channels,
                                    layer_type, layer.name(), "weight");
    if (!r.good()) return r;
    if (has_bias){
        r = validateGeneralWeightParams(bias, output_channels, 1,
                                        layer_type, layer.name(), "bias");
        if (!r.good()) return r;
    }
    
    return r;
}

//    EmbeddingLayerParams embedding = 36;
Result NeuralNetworkSpecValidator::validateEmbeddingLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    if (!r.good()) {
        return r;
    }
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Embedding", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Embedding", 4, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.embedding();
    r = validateEmbeddingWeightsBias(layer, params.weights(), params.bias());
    
    return r;
}

Result NeuralNetworkSpecValidator::validateEmbeddingNDLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    if (!r.good()) {
        return r;
    }
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "EmbeddingND", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "EmbeddingND", 2, 5, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.embeddingnd();
    r = validateEmbeddingWeightsBias(layer, params.weights(), params.bias());
    
    return r;
}

Result NeuralNetworkSpecValidator::validateAverageLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateMaxLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateMinLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

//    SequenceRepeatLayerParams sequenceRepeat = 37;
Result NeuralNetworkSpecValidator::validateSequenceRepeatLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "SequenceRepeat", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "SequenceRepeat", 5, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateSoftmaxLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (!r.good()) {return r;}
    
    r = validateOutputCount(layer, 1, 1);
    if (!r.good()) {return r;}
    
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Softmax", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "Softmax", 3, -1, blobNameToRank);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateConcatLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "Concat", blobNameToRank);
        if (!r.good()) {return r;}
        if (layer.concat().sequenceconcat()) {
            r = validateRankCount(layer, "Concat", 5, -1, blobNameToRank);
            if (!r.good()) {return r;}
        } else {
            r = validateRankCount(layer, "Concat", 3, -1, blobNameToRank);
            if (!r.good()) {return r;}
        }
       
        // check that all inputs have same rank
        int rank = 0;
        if (blobNameToRank.find(layer.input(0)) != blobNameToRank.end()) {
            rank = blobNameToRank.at(layer.input(0));
        } else {
            return r;
        }
        
        for (const auto& input : layer.input()) {
            if (blobNameToRank.find(input) != blobNameToRank.end()) {
                if (rank != blobNameToRank.at(input)) {
                    std::string err;
                    err = "Layer '" + std::string(layer.name()) + "' of type 'Concat' expects equal ranks for its inputs, but they are not equal.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateCustomLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, -1);
    }
    
    if (layer.custom().classname().size() == 0) {
        std::string err = "Custom layer " + layer.name() + " has an empty 'className' field. This field is required in order for Core ML to link to the implementation for this custom class.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    for (const auto& param: layer.custom().weights()) {
        if (!checkSingleWeightType(param)) {
            std::string err = "Custom layer " + layer.name() + " has a weights parameter with multiple types filled in.  The WeightParams message should be treated as a oneof.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    
    return r;
}

//    ResizeBilinear Layer
Result NeuralNetworkSpecValidator::validateResizeBilinearLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "ResizeBilinear", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "ResizeBilinear", 3, -1, blobNameToRank);
        if (!r.good()) {return r;}
    }
    
    const auto& params = layer.resizebilinear();
    // target Size must be 2D if provided
    if (!(params.targetsize_size() == 0 || params.targetsize_size() == 2)) {
        std::string err = "Target Size in the resize bilinear layer '" + layer.name() + "' must be a vector of size 2 (i.e height, width) but is a vector of size " + std::to_string(params.targetsize_size()) + ".";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    return r;
}

//    CropResize Layer
Result NeuralNetworkSpecValidator::validateCropResizeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!r.good()){ return r;}
    if (ndArrayInterpretation) {
        r = validateInputOutputRankEquality(layer, "CropResize", blobNameToRank);
        if (!r.good()) {return r;}
        r = validateRankCount(layer, "CropResize", 5, -1, blobNameToRank);
        if (!r.good()) {return r;}
        
        if (blobNameToRank.find(layer.input(0)) != blobNameToRank.end() &&
            blobNameToRank.find(layer.input(1)) != blobNameToRank.end()) {
            if (blobNameToRank.at(layer.input(0)) != blobNameToRank.at(layer.input(1))) {
                std::string err;
                err = "Layer '" + std::string(layer.name()) + "' of type 'CropResize' expects equal ranks for its inputs, but they are not equal.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
        
    }
    
    const auto& params = layer.cropresize();
    // target Size must be 2D if provided
    if (!(params.targetsize_size() == 0 || params.targetsize_size() == 2)) {
        std::string err = "Target Size in the crop resize layer '" + layer.name() + "' must be a vector of size 2 (i.e height, width) but is a vector of size " + std::to_string(params.targetsize_size()) + ".";
        r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        return r;
    }
    
    return r;
}

// Branch Layer
Result NeuralNetworkSpecValidator::validateBranchLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 0, 0);
    }
    if (!r.good()) {
        return r;
    }
    
    if (layer.inputtensor_size()) {
        auto &in_tensor = layer.inputtensor(0);
        if (in_tensor.dimvalue_size()) {
            for (int i=0; i < in_tensor.dimvalue_size(); i++) {
                if (in_tensor.dimvalue(i) > 1) {
                    std::string err = "Branch Layer '" + std::string(layer.name()) + "' input's length cannot be more than 1";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
    }
    
    std::string condition = layer.input(0);
    
    const auto& params = layer.branch();
    // check that condition is already present in the network
    if (blobs.find(condition) == blobs.end()) {
        std::string err = "Branch Layer '" + std::string(layer.name()) + "' requires the condition blob '"
        + condition + "' which is not present in the network prior to this layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    // get the NN spec for If and Else branches
    const auto& ifNNSpec = params.ifbranch();
    const auto& elseNNSpec = params.elsebranch();
    if (ifNNSpec.layers_size() == 0) {
        std::string err = "Branch Layer '" + std::string(layer.name()) + "' has an empty If branch";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    bool isElseBranch = (elseNNSpec.layers_size() > 0) ? true : false;
    
    // validate both If and Else branches
    NeuralNetworkSpecValidator ifNNValidator(blobs, ModelIOBlobNameToRank, ndArrayInterpretation, loopStackDepth, blobNameToRank);
    r = ifNNValidator.validateNeuralNetwork(ifNNSpec);
    if (!r.good()) {
        return r;
    }
    
    if (isElseBranch) {
        NeuralNetworkSpecValidator elseNNValidator(blobs, ModelIOBlobNameToRank, ndArrayInterpretation, loopStackDepth, blobNameToRank);
        r = elseNNValidator.validateNeuralNetwork(elseNNSpec);
        if (!r.good()) {
            return r;
        }
        // update set of "blobs" of the current Neural Network: the ones that are in both if and else branches
        for (auto& blob_map: ifNNValidator.blobs){
            std::string current_blob_name = blob_map.first;
            if (elseNNValidator.blobs.find(current_blob_name) != elseNNValidator.blobs.end()){
                // if we are here, this means "current_blob_name" is present in both if and else branch
                blobs[current_blob_name].insert(ifNNValidator.blobs[current_blob_name].begin(),
                                                ifNNValidator.blobs[current_blob_name].end());
                blobs[current_blob_name].insert(elseNNValidator.blobs[current_blob_name].begin(),
                                                elseNNValidator.blobs[current_blob_name].end());
            }
        }
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateTransposeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    // axes are required parameters
    const auto& params = layer.transpose();
    if (params.axes_size() == 0) {
        const std::string err = "Axes are required parameters for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateCopyLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    if (layer.input(0) == layer.output(0)) {
        const std::string err = "Copy layer '" + layer.name() + "' has identical input and output names.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateBatchedMatmulLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {return r;}
    }
    
    // validate rank, if present
    if (layer.input_size() == 2 && layer.inputtensor_size() == 2 && layer.outputtensor_size() == 1) {
        int in1_rank = static_cast<int>(layer.inputtensor(0).rank());
        int in2_rank = static_cast<int>(layer.inputtensor(1).rank());
        int out_rank = static_cast<int>(layer.outputtensor(0).rank());
        if (out_rank != std::max(2, std::max(in1_rank, in2_rank))) {
            std::string err = "BatchedMatMul layer '" + layer.name() + "': given ranks of the two inputs, rank of the output is incorrect.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    
    if (layer.input_size() == 1 && layer.inputtensor_size() == 1 && layer.outputtensor_size() == 1) {
        int in_rank = static_cast<int>(layer.inputtensor(0).rank());
        int out_rank = static_cast<int>(layer.outputtensor(0).rank());
        if (out_rank != in_rank) {
            std::string err = "BatchedMatMul layer '" + layer.name() + "': has one input, in this case, output and input ranks must be equal but they are not.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    
    if (layer.input_size() > 1 && layer.batchedmatmul().hasbias()) {
        std::string err = "BatchedMatMul layer '" + layer.name() + "': has two inputs and 'hasBias' flag is set to True."
                                "However, bias is only supported when the layer has 1 input.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    if (layer.input_size() == 1) {
        const auto& params = layer.batchedmatmul();
        r = validateInnerProductWeightsBias(layer, params.weights(), params.bias());
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateConcatNDLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.concatnd();
    if (layer.inputtensor_size() > 0) {
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSoftmaxNDLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.softmaxnd();
    if (layer.inputtensor_size() > 0) {
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateReverseLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.reverse();
    // requires len(reverseDim) == rank(inputTensor)
    if (layer.inputtensor_size() > 0) {
        if (params.reversedim_size() != static_cast<int>(layer.inputtensor(0).rank())) {
            const std::string err = "Invalid size of reverse_dim for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateBooleanElementwiseLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    if (layer.layer_case() == CoreML::Specification::NeuralNetworkLayer::kLogicalNot) {
        r = validateInputCount(layer, 1, 1);
    } else if (layer.layer_case() == CoreML::Specification::NeuralNetworkLayer::kLogicalAnd ||
               layer.layer_case() == CoreML::Specification::NeuralNetworkLayer::kLogicalOr ||
               layer.layer_case() == CoreML::Specification::NeuralNetworkLayer::kLogicalXor) {
        r = validateInputCount(layer, 2, 2);
    } else {
        r = validateInputCount(layer, 1, 2);
    }
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateFillLikeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateFillStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 0);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.fillstatic();
    if (params.targetshape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateFillDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateReshapeLikeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateReshapeStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.reshapestatic();
    if (params.targetshape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateReshapeDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateWhereLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 3, 3);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateTrigonometryLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validatePowBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateExp2Layer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateUpperTriangularLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}


Result NeuralNetworkSpecValidator::validateLowerTriangularLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateMatrixBandPartLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateBroadcastToLikeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateBroadcastToStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.broadcasttostatic();
    if (params.targetshape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateBroadcastToDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateAddBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSubtractBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateMultiplyBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateDivideBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateMaxBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateMinBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateFloorDivBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateModBroadcastableLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateGatherLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateScatterLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 3, 3);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }

    if (ndArrayInterpretation) {
        std::string err;
        if (layer.inputtensor_size() > 0) {
            if (layer.inputtensor_size() != 3) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Scatter layer must have 3 input tensor fields filled");
            }
            if (!(layer.inputtensor(0).rank() == layer.inputtensor(2).rank() &&
                  layer.inputtensor(1).rank() == 1)) {
                err = "Input ranks of Scatter layer '" + std::string(layer.name()) + "' are invalid.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
        if (layer.inputtensor_size() > 0 && layer.outputtensor_size() > 0) {
            if (layer.outputtensor_size() != 1) {
                return Result(ResultType::INVALID_MODEL_PARAMETERS, "Scatter layer must have 1 output tensor fields filled");
            }
            if (!(layer.inputtensor(0).rank() == layer.outputtensor(0).rank())) {
                err = "Output rank of Scatter layer '" + std::string(layer.name()) + "' does not match container input.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
    }

    return r;
}

Result NeuralNetworkSpecValidator::validateGatherNDLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateScatterNDLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 3, 3);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateGatherAlongAxisLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateScatterAlongAxisLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 3, 3);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateWhereNonZeroLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateStackLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, -1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.stack();
    if (layer.inputtensor_size() > 0) {
        const int rank0 = static_cast<int>(layer.inputtensor(0).rank());
        const int rank1 = static_cast<int>(layer.inputtensor(1).rank());
        if (rank0 != rank1) {
            const std::string err = "Shapes of all inputs must match for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (!(params.axis() >= -(rank0 + 1) && params.axis() < rank0 + 1)) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)] for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSplitNDLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 2, -1);
    }
    size_t numSplits = 0;
    const auto& params = layer.splitnd();

    if (params.splitsizes_size() > 0) {
        numSplits = static_cast<size_t>(params.splitsizes_size());
    } else {
        numSplits = params.numsplits();
    }
    if (numSplits == 0) {
        const std::string err = "Either split_sizes or num_splits should be provided for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (layer.inputtensor_size() > 0) {
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    if (numSplits != static_cast<size_t>(layer.output_size())) {
        const std::string err = "Value of num_splits should match size of output names for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }

    return r;
}

Result NeuralNetworkSpecValidator::validateCeilLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateFloorLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRoundLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSignLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateClipLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.clip();
    if (params.minval() > params.maxval()) {
        const std::string err = "Value of minval should be smaller than maxval for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSliceStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.slicestatic();
    // check for required parameters
    if (params.beginids_size() == 0) {
        const std::string err = "Begin IDs are required parameters for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (params.endids_size() == 0) {
        const std::string err = "End IDs are required parameters for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (params.strides_size() == 0) {
        const std::string err = "Strides are required parameters for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (params.beginmasks_size() == 0) {
        const std::string err = "Begin masks are required parameters for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (params.endmasks_size() == 0) {
        const std::string err = "End masks are required parameters for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSliceDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 6);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateTileLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateSlidingWindowsLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.slidingwindows();
    // axis should be in range [-inputRank : inputRank)
    if (layer.inputtensor_size() > 0) {
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateReverseSeqLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 2);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateReductionTypeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    // all given axes should be in range [-inputRank : inputRank)
    if (layer.inputtensor_size() > 0) {
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";

        switch (layer.layer_case()) {
            case Specification::NeuralNetworkLayer::LayerCase::kReduceL1:
                for (const auto& axis : layer.reducel1().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceL2:
                for (const auto& axis : layer.reducel2().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceMax:
                for (const auto& axis : layer.reducemax().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceMin:
                for (const auto& axis : layer.reducemin().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceSum:
                for (const auto& axis : layer.reducesum().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceProd:
                for (const auto& axis : layer.reduceprod().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceMean:
                for (const auto& axis : layer.reducemean().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceLogSum:
                for (const auto& axis : layer.reducelogsum().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceSumSquare:
                for (const auto& axis : layer.reducesumsquare().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;
            case Specification::NeuralNetworkLayer::LayerCase::kReduceLogSumExp:
                for (const auto& axis : layer.reducelogsumexp().axes()) {
                    if (axis < -rank || axis >= rank) {
                        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    }
                }
                break;

            default:
                break;
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateLoopLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 0, 0);
    }
    if (!r.good()) {
        return r;
    }
    
    // check that if input exists, and if its shape exists, its scalar
    if (layer.inputtensor_size()) {
        auto &in_tensor = layer.inputtensor(0);
        if (in_tensor.dimvalue_size()) {
            for (int i=0; i < in_tensor.dimvalue_size(); i++) {
                if (in_tensor.dimvalue(i) > 1) {
                    std::string err = "Loop Layer '" + std::string(layer.name()) + "' input's length cannot be more than 1";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
    }
    
    const auto& params = layer.loop();
    const auto &conditionNNSpec = params.conditionnetwork();
    std::string conditionVar = params.conditionvar();
    const auto &bodyNNSpec = params.bodynetwork();
    bool isConditionNet = (conditionNNSpec.layers_size() > 0) ? true : false;
    
    // validate some generic requirements for the existense of fields
    if (bodyNNSpec.layers_size() == 0) {
        std::string err = "Loop Layer '" + std::string(layer.name()) + "' has an empty body network";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if ((conditionVar == "" && isConditionNet) || (conditionVar != "" && !isConditionNet)) {
        std::string err = "Loop Layer '" + std::string(layer.name()) + "': condition variable must be provided if condition network exists and vice versa.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (!isConditionNet && params.maxloopiterations() == 0 && layer.input_size() == 0) {
        std::string err = "Loop Layer '" + std::string(layer.name()) + "': has no input, no condition network and max loop iterations is 0.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    
    NeuralNetworkSpecValidator conditionNNValidator(blobs, ModelIOBlobNameToRank, ndArrayInterpretation, loopStackDepth, blobNameToRank);
    
    // validate the condition network if it exists
    if (isConditionNet) {
        r = conditionNNValidator.validateNeuralNetwork(conditionNNSpec);
        if (!r.good()) {
            return r;
        }
        
        // conditionVar must be produced by the condition network
        if (blobs.find(conditionVar) == blobs.end()) { // conditionVar not in the parent NN
            if (conditionNNValidator.blobs.find(conditionVar) == conditionNNValidator.blobs.end()) {
                std::string err = "Loop Layer '" + std::string(layer.name()) + "': has conditionVar named '" + conditionVar + "' which is not produced by the condition network";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        } else { // conditionVar in the parent NN: must be regenerated by the condition network
            std::set<std::string> parent_set = blobs[conditionVar];
            std::set<std::string> condition_set = conditionNNValidator.blobs[conditionVar];
            if (parent_set == condition_set) {
                std::string err = "Loop Layer '" + std::string(layer.name()) + "': has conditionVar named '" + conditionVar + "' which is not produced by the condition network";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
        
        // add the blobs generated by the condition network to the scope of the parent network
        for (auto& blob_map: conditionNNValidator.blobs) {
            std::string current_blob_name = blob_map.first;
            blobs[current_blob_name].insert(conditionNNValidator.blobs[current_blob_name].begin(),
                                            conditionNNValidator.blobs[current_blob_name].end());
        }
    }
    
    // validate the body network
    NeuralNetworkSpecValidator bodyNNValidator(blobs, ModelIOBlobNameToRank, ndArrayInterpretation, loopStackDepth + 1, blobNameToRank);
    r = bodyNNValidator.validateNeuralNetwork(bodyNNSpec);
    if (!r.good()) {
        return r;
    }
    
    // update the set of "blobs" of the current Neural Network:
    // - if there is no condition network, all the blobs generated in the body network gets added to the scope of the parent network
    // - if there is a condition network, all its blobs gets added to the overall scope (already done above), as well the ones from the bodynetwork that are present in the condition net.
    
    if (!isConditionNet) {
        for (auto& blob_map: bodyNNValidator.blobs) {
            std::string current_blob_name = blob_map.first;
            blobs[current_blob_name].insert(bodyNNValidator.blobs[current_blob_name].begin(),
                                            bodyNNValidator.blobs[current_blob_name].end());
        }
    } else {
        for (auto& blob_map: conditionNNValidator.blobs) {
            std::string current_blob_name = blob_map.first;
            if (bodyNNValidator.blobs.find(current_blob_name) != bodyNNValidator.blobs.end()) {
                blobs[current_blob_name].insert(bodyNNValidator.blobs[current_blob_name].begin(),
                                                bodyNNValidator.blobs[current_blob_name].end());
            }
        }
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateLoopContinueBreakLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 0);
    if (r.good()) {
        r = validateOutputCount(layer, 0, 0);
    }
    if (!r.good()) {
        return r;
    }
    
    if (loopStackDepth == 0) {
        std::string err;
        if (layer.layer_case() == Specification::NeuralNetworkLayer::LayerCase::kLoopBreak) {
            err = "Loop Break Layer '" + std::string(layer.name()) + "' must be inside the bodyNetwork of a loop layer.";
        } else {
            err = "Loop Continue Layer '" + std::string(layer.name()) + "' must be inside the bodyNetwork of a loop layer.";
        }
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateActivationLayers(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRankPreservingReshapeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
        if (!r.good()) {
            return r;
        }
    }
    
    r = validateRankExists(layer);
    if (!r.good()) {
        return r;
    }

    const auto& params = layer.rankpreservingreshape();
    if (params.targetshape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }

    int input_rank = static_cast<int>(layer.inputtensor(0).rank());
    int output_rank = static_cast<int>(layer.outputtensor(0).rank());
    int shape_length = params.targetshape_size();
    if (input_rank != output_rank) {
        std::string err = "RankPreservingReshape Layer '" + std::string(layer.name()) + "': input and output rank must be equal.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (input_rank != shape_length) {
        std::string err = "RankPreservingReshape Layer '" + std::string(layer.name()) + "': input rank must be same as the length of the target shape property.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateFlattenTo2DLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.flattento2d();
    // axis should be in range [-inputRank : inputRank)
    if (layer.inputtensor_size() > 0) {
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateExpandDimsLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (layer.expanddims().axes_size() == 0) {
        std::string err = "ExpandDims Layer '" + std::string(layer.name()) + "': length of the 'axes' parameter cannot be 0.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    
    // either all values in axes must be positive or all negative
    std::set<int> axes_set;
    for (int i=0; i<layer.expanddims().axes_size(); i++) {
        int a = (int)layer.expanddims().axes(i);
        if (axes_set.find(a) != axes_set.end()) {
            std::string err = "ExpandDims Layer '" + std::string(layer.name()) + "': all the values in the 'axes' parameter must be unique.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        } else {
            axes_set.insert(a);
        }
    }
    
    if (layer.inputtensor_size() > 0 && layer.outputtensor_size() > 0) {
        int input_rank = static_cast<int>(layer.inputtensor(0).rank());
        int output_rank = static_cast<int>(layer.outputtensor(0).rank());
        int axes_length = layer.expanddims().axes_size();
        
        if (input_rank + axes_length != output_rank) {
            std::string err = "ExpandDims Layer '" + std::string(layer.name()) + "': input rank plus the length of the axes parameter must equal output rank.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        
        std::vector<int> axes;
        for (int i=0; i<axes_length; i++) {
            int a = (int)layer.expanddims().axes(i);
            if (a < 0) {
                a = output_rank + a;
            }
            if (std::find(axes.begin(), axes.end(), a) != axes.end()) {
                std::string err = "ExpandDims Layer '" + std::string(layer.name()) + "': axes parameter list cannot have the same value more than once.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
            axes.push_back(a);
        }
        
        int max_value = *std::max_element(axes.begin(), axes.end());
        int min_value = *std::min_element(axes.begin(), axes.end());
        if (max_value > output_rank - 1 || min_value < 0) {
            std::string err = "ExpandDims Layer '" + std::string(layer.name()) + "': axes refers to a dimension that exceeds the output rank.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }

    return r;
}

Result NeuralNetworkSpecValidator::validateSqueezeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (!layer.squeeze().squeezeall()) {
        
        if (layer.squeeze().axes_size() == 0) {
            std::string err = "Squeeze Layer '" + std::string(layer.name()) + "': length of the 'axes' parameter cannot be 0.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        
        // either all values in axes must be positive or all negative
        std::set<int> axes_set;
        for (int i=0; i<layer.squeeze().axes_size(); i++) {
            int a = (int)layer.squeeze().axes(i);
            if (axes_set.find(a) != axes_set.end()) {
                std::string err = "Squeeze Layer '" + std::string(layer.name()) + "': all the values in the 'axes' parameter must be unique.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            } else {
                axes_set.insert(a);
            }
        }
        
        if (layer.inputtensor_size() > 0 && layer.outputtensor_size() > 0) {
            int input_rank = static_cast<int>(layer.inputtensor(0).rank());
            int output_rank = static_cast<int>(layer.outputtensor(0).rank());
            int axes_length = layer.squeeze().axes_size();
            
            if (input_rank != 1) {
                if (output_rank + axes_length != input_rank) {
                    std::string err = "Squeeze Layer '" + std::string(layer.name()) + "': output rank plus the length of the axes parameter must equal input rank.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
            
            std::vector<int> axes;
            for (int i=0; i<axes_length; i++) {
                int a = (int)layer.squeeze().axes(i);
                if (a < 0) {
                    a = input_rank + a;
                }
                if (std::find(axes.begin(), axes.end(), a) != axes.end()) {
                    std::string err = "Squeeze Layer '" + std::string(layer.name()) + "': axes parameter list cannot have the same value more than once.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
                axes.push_back(a);
            }
            
            int max_value = *std::max_element(axes.begin(), axes.end());
            int min_value = *std::min_element(axes.begin(), axes.end());
            if (max_value > input_rank - 1 || min_value < 0) {
                std::string err = "Squeeze Layer '" + std::string(layer.name()) + "': axes refers to a dimension that exceeds the input rank.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        }
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateRangeStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 0);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    
    if (layer.outputtensor_size() > 0) {
        int rank = static_cast<int>(layer.outputtensor(0).rank());
        r =  checkRank(layer, "Range", 1, 1, "output", rank);
    }
    
    return r;
}

Result NeuralNetworkSpecValidator::validateRangeDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    if (layer.input_size() > 0) {
        r = validateInputCount(layer, 1, 3);
    } else {
        r = validateInputCount(layer, 0, 0);
    }
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }

    if (layer.outputtensor_size() > 0) {
        int rank = static_cast<int>(layer.outputtensor(0).rank());
        r =  checkRank(layer, "Range", 1, 1, "output", rank);
    }

    return r;
}

Result NeuralNetworkSpecValidator::validateLoadConstantNDLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    if (!(r = validateInputCount(layer, 0, 0)).good()) return r;
    if (!(r = validateOutputCount(layer, 1, 1)).good()) return r;

    const auto& params = layer.loadconstantnd();
    WeightParamType paramType = valueType(params.data());

    if (params.shape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }

    // Only float32 or float16 parameters can be populated at any time
    if (paramType == UNSPECIFIED) {
        return Result(ResultType::INVALID_MODEL_PARAMETERS,
                      "LoadConstantND layer '" + layer.name() + "' has both full precision and half precision weight fields populated");
    }
    if (!(params.shape_size() >= 1 && params.shape_size() <= 5)) {
        return Result(ResultType::INVALID_MODEL_PARAMETERS,
                      "LoadConstantND layer '" + layer.name() + "'can only accept shape of length 1 to 5");
    }
    size_t total_shape = 1;
    for (int i = 0; i < params.shape_size(); i++) {
        total_shape *= params.shape(i);
    }

    r = validateGeneralWeightParams(params.data(), total_shape, 1, "LoadConstantND", layer.name(), "constants");
    if (!r.good()) return r;

    return Result();
}

Result NeuralNetworkSpecValidator::validateGetShapeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    if (!(r = validateInputCount(layer, 1, 1)).good()) return r;
    if (!(r = validateOutputCount(layer, 1, 1)).good()) return r;
    return Result();
}

Result NeuralNetworkSpecValidator::validateRandomNormalLikeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomNormalStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 0);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.randomnormalstatic();
    if (params.outputshape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomNormalDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomUniformLikeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.randomuniformlike();
    if (params.minval() > params.maxval()) {
        const std::string err = "Value of minval should be smaller than maxval for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomUniformStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 0);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.randomuniformstatic();
    if (params.outputshape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (params.minval() > params.maxval()) {
        const std::string err = "Value of minval should be smaller than maxval for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomUniformDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.randomuniformdynamic();
    if (params.minval() > params.maxval()) {
        const std::string err = "Value of minval should be smaller than maxval for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomBernoulliLikeLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.randombernoullilike();
    if (params.prob() < 0.0f || params.prob() > 1.0f) {
        const std::string err = "Value of prob should be in range [0: 1] for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomBernoulliStaticLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 0, 0);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.randombernoullistatic();
    if (params.outputshape_size() == 0) {
        const std::string err = "Target shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (params.prob() < 0.0f || params.prob() > 1.0f) {
        const std::string err = "Value of prob should be in range [0: 1] for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateRandomBernoulliDynamicLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    const auto& params = layer.randombernoullidynamic();
    if (params.prob() < 0.0f || params.prob() > 1.0f) {
        const std::string err = "Value of prob should be in range [0: 1] for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateTopKLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    if (!(r = validateInputCount(layer, 1, 2)).good()) return r;
    if (!(r = validateOutputCount(layer, 2, 2)).good()) return r;
    if (!(r = validateInputOutputRankEquality(layer, "TopK", blobNameToRank)).good()) return r;
    
    if (blobNameToRank.find(layer.input(0)) != blobNameToRank.end() &&
        blobNameToRank.find(layer.output(1)) != blobNameToRank.end()) {
        if (blobNameToRank.at(layer.input(0)) != blobNameToRank.at(layer.output(1))) {
            std::string err;
            err = "Layer '" + std::string(layer.name()) + "' of type 'TopK' expects equal ranks for its input and second output, but they are not equal.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }

    if (layer.inputtensor_size() > 0) {
        const auto& params = layer.topk();
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return Result();
}

Result NeuralNetworkSpecValidator::validateArgMaxLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    if (!(r = validateInputCount(layer, 1, 1)).good()) return r;
    if (!(r = validateOutputCount(layer, 1, 1)).good()) return r;
    if (!layer.argmax().removedim()) {
        if (!(r = validateInputOutputRankEquality(layer, "ArgMax", blobNameToRank)).good()) return r;
    }
    if (layer.inputtensor_size() > 0) {
        const auto& params = layer.argmax();
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return Result();
}

Result NeuralNetworkSpecValidator::validateArgMinLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    if (!(r = validateInputCount(layer, 1, 1)).good()) return r;
    if (!(r = validateOutputCount(layer, 1, 1)).good()) return r;
    if (!layer.argmin().removedim()) {
        if (!(r = validateInputOutputRankEquality(layer, "ArgMin", blobNameToRank)).good()) return r;
    }
    if (layer.inputtensor_size() > 0) {
        const auto& params = layer.argmin();
        const int rank = static_cast<int>(layer.inputtensor(0).rank());
        if (params.axis() < -rank || params.axis() >= rank) {
            const std::string err = "Value of axis must be in the range [-rank(tensor), rank(tensor)) for '" + layer.name() + "' layer.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return Result();
}

Result NeuralNetworkSpecValidator::validateCategoricalDistributionLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateLayerNormalizationLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 1);
    if (r.good()) {
        r = validateOutputCount(layer, 1, 1);
    }
    // check required parameters
    std::string err = "";
    const auto& params = layer.layernormalization();
    if (params.normalizedshape_size() == 0) {
        err = "Normalized shape is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (!params.has_gamma()) {
        err = "Gamma is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (!params.has_beta()) {
        err = "Beta is required parameter for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    // value of gamma and beta should be unquantized
    if (params.gamma().has_quantization() || params.gamma().has_quantization()) {
        err = "Gamma and Beta should not be quantized for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    // shape of gamma and beta should match normalized shape
    const int totalShape = std::accumulate(params.normalizedshape().begin(),
                                           params.normalizedshape().end(),
                                           1, std::multiplies<int>());
    if (params.gamma().floatvalue_size() != totalShape) {
        err = "Shape of gamma should match normalized_shape for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    if (params.beta().floatvalue_size() != totalShape) {
        err = "Shape of beta should match normalized_shape for '" + layer.name() + "' layer.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return r;
}

Result NeuralNetworkSpecValidator::validateConstantPadLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 1, 2);
    if (!r.good()) {return r;}
    r = validateOutputCount(layer, 1, 1);
    if (!r.good()) {return r;}
    r = validateInputOutputRankEquality(layer, "ConstantPad", blobNameToRank);
    if (!r.good()) {return r;}

    const auto& params = layer.constantpad();

    if (layer.input_size() == 1) {
        int len = params.padamounts_size();
        if (len % 2 != 0) {
            const std::string err = "In 'ConstantPad' layer '" + layer.name() + "', length of 'padAmounts' parameter is " + std::to_string(len) + ", an odd value, which is not allowed.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (len == 0) {
            const std::string err = "In 'ConstantPad' layer '" + layer.name() + "', length of 'padAmounts' cannot be zero when only 1 input is provided.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (params.padtogivenoutputsizemode()) {
            for (int i=0; i < len/2; i++) {
                int pad_a = (int)params.padamounts(2*i);
                int pad_b = (int)params.padamounts(2*i+1);
                if (pad_a > 0 && pad_b > 0) {
                    const std::string err = "In 'ConstantPad' layer '" + layer.name() + "', 'padToGivenOutputSizeMode' is true, and both padding values corresponding to dimension " + std::to_string(i) + " are non zero, which is invalid. Only one value can be non-zero.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
    }

    return r;
}

Result NeuralNetworkSpecValidator::validateNMSLayer(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    r = validateInputCount(layer, 2, 5);
    if (!r.good()) {return r;}
    r = validateOutputCount(layer, 4, 4);

    return r;
}

Result NeuralNetworkSpecValidator::validateFailUnknownType(const Specification::NeuralNetworkLayer& layer) {
    return Result(ResultType::INVALID_MODEL_PARAMETERS, "Unsupported layer type (" + layer.GetTypeName() + ") for layer '" + layer.name() + "'.");
}
