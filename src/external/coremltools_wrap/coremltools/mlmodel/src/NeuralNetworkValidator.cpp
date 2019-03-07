//
//  NeuralNetworkValidator.cpp
//  libmlmodelspec
//
//  Created by Zach Nation on 12/21/16.
//  Copyright © 2016 Apple. All rights reserved.
//

#include "../build/format/NeuralNetwork_enums.h"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "transforms/NeuralNetwork.hpp"
#include "NeuralNetworkShapes.hpp"
#include "QuantizationValidationUtils.hpp"

#include <algorithm>
#include <functional>
#include <sstream>

namespace CoreML {

#pragma mark Layer Specific Functions

    // Min and max are the minimum and maximum number of possible inputs.
    // negative values are interpreted as no bound
    static Result validateInputCount(const Specification::NeuralNetworkLayer& layer, int min, int max) {

        assert( min <= max || max < 0 );
        std::string err;

        if (max > 0 && max == min && layer.input_size() != max) {
            err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + " has " + std::to_string(layer.input_size()) + " inputs but expects exactly " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (min > 0 && layer.input_size() < min) {
            err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " inputs but expects at least " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (max > 0 && layer.input_size() > max) {
            err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " inputs but expects at most " + std::to_string(max) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else {
            return Result();
        }
    }

    static Result validateOutputCount(const Specification::NeuralNetworkLayer& layer, int min, int max) {

        assert( min <= max || max < 0 );
        std::string err;

        if (max > 0 && max == min && layer.output_size() != max) {
            err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " outputs but expects exactly " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (min > 0 && layer.output_size() < min) {
            err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " outputs but expects at least " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (max > 0 && layer.output_size() > max) {
            err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + " has " + std::to_string(layer.input_size()) + " outputs but expects at most " + std::to_string(max) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else {
            return Result();
        }
    }

    static Result validateGeneralWeightParams(const CoreML::Specification::WeightParams& weight,
                                                const uint64_t expectedUnits,
                                                const uint64_t outChannels,
                                                const std::string& layerClassName,
                                                const std::string& layerName,
                                                const std::string& weightName){
        // Validate a 2D WeightParam
        Result r;
        WeightParamType wType = valueType(weight);
        uint64_t size = 0;
        if (wType == FLOAT32 || wType == FLOAT16){
            if (wType == FLOAT32){
                size = static_cast<uint64_t>(weight.floatvalue().size());
            } else {
                size = static_cast<uint64_t>(weight.float16value().size()/2);
            }
            if (size != expectedUnits){
                const std::string err = layerClassName + "Layer '" + layerName + "' has incorrect " +
                        weightName + " size " + std::to_string(size) + " (expected " +
                        std::to_string(expectedUnits) + ").";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        } else if (wType == QUINT){
            if (!CoreML::hasSufficientBytesInQuantizedWeightParam(weight, expectedUnits)){
                const std::string err = layerClassName + "Layer '" + layerName +
                        "' has insufficient bytes for quantized " + weightName + " with " +
                        std::to_string(expectedUnits) + "units.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
            if (!CoreML::hasValidQuantizationParams(weight, (int) outChannels)){
                const std::string err = layerClassName + "Layer '" + layerName +
                "' has invalid quantization parameters for quantized " + weightName + ".";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
        } else if (wType == UNSPECIFIED){
            const std::string err = layerClassName + "Layer '" + layerName + "' has unspecified " + weightName + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        } else { // EMPTY
            const std::string err = layerClassName + "Layer '" + layerName + "' has empty " + weightName + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        return r;
    }

    static bool isWeightParamTypeCompatible(const std::vector<WeightParamType>& weightTypes){
        int nfp32 = 0, nfp16 = 0; // number of FP32/16 weight blobs
        for (auto& wt : weightTypes){
            if (wt == FLOAT32){
                nfp32++;
            } else if (wt == FLOAT16){
                nfp16++;
            } // else do nothing - quantization assumed to be always compatible
            if (nfp32 * nfp16 > 0){
                return false;
            }
        }
        return true;
    }

    static Result validateLSTMWeightParams(const Specification::LSTMWeightParams& lstmWeightParams, const Specification::LSTMParams lstmParams) {
        bool has_peephole_vector = lstmParams.haspeepholevectors();
        bool has_bias_vector = lstmParams.hasbiasvectors();

        // Validate all weightParam types match
        std::vector<CoreML::WeightParamType> weightTypes;
        weightTypes.push_back(valueType(lstmWeightParams.inputgateweightmatrix()));
        weightTypes.push_back(valueType(lstmWeightParams.forgetgateweightmatrix()));
        weightTypes.push_back(valueType(lstmWeightParams.blockinputweightmatrix()));
        weightTypes.push_back(valueType(lstmWeightParams.outputgateweightmatrix()));
        weightTypes.push_back(valueType(lstmWeightParams.inputgaterecursionmatrix()));
        weightTypes.push_back(valueType(lstmWeightParams.forgetgaterecursionmatrix()));
        weightTypes.push_back(valueType(lstmWeightParams.blockinputrecursionmatrix()));
        weightTypes.push_back(valueType(lstmWeightParams.outputgaterecursionmatrix()));
        if(has_bias_vector){
            weightTypes.push_back(valueType(lstmWeightParams.inputgatebiasvector()));
            weightTypes.push_back(valueType(lstmWeightParams.forgetgatebiasvector()));
            weightTypes.push_back(valueType(lstmWeightParams.blockinputbiasvector()));
            weightTypes.push_back(valueType(lstmWeightParams.outputgatebiasvector()));
        }
        if(has_peephole_vector){
            weightTypes.push_back(valueType(lstmWeightParams.inputgatepeepholevector()));
            weightTypes.push_back(valueType(lstmWeightParams.forgetgatepeepholevector()));
            weightTypes.push_back(valueType(lstmWeightParams.outputgatepeepholevector()));
        }
        if(!isWeightParamTypeCompatible(weightTypes)){
            const std::string err = "LSTM weight parameters have inconsistent field value types. "
                    "Types should match and should be either half or full precision";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        return Result();
    }


    //    ActivationParams activation = 7;
    static Result validateActivationParams(const Specification::ActivationParams& params) {
        // make sure params fall into expected values for non-recurrent activations
        switch (params.NonlinearityType_case()) {
            case Specification::ActivationParams::NonlinearityTypeCase::kReLU:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kLeakyReLU:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kTanh:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kScaledTanh:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kSigmoid:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kSigmoidHard:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kLinear:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kELU:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kSoftplus:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kPReLU:
            {
                if (valueType(params.prelu().alpha()) == UNSPECIFIED) {
                    std::stringstream ss;
                    ss << "Nonlinearity type ";
                    ss << MLActivationParamsNonlinearityType_Name(static_cast<MLActivationParamsNonlinearityType>(params.NonlinearityType_case()));
                    ss << " has inconsistent weight parameter types.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, ss.str());
                }
                break;
            }
            case Specification::ActivationParams::NonlinearityTypeCase::kParametricSoftplus:
            {
                if (valueType(params.parametricsoftplus().alpha()) != valueType(params.parametricsoftplus().beta()) ||
                    valueType(params.parametricsoftplus().alpha()) == UNSPECIFIED ||
                    valueType(params.parametricsoftplus().beta()) == UNSPECIFIED) {
                    std::stringstream ss;
                    ss << "Nonlinearity type ";
                    ss << MLActivationParamsNonlinearityType_Name(static_cast<MLActivationParamsNonlinearityType>(params.NonlinearityType_case()));
                    ss << " has inconsistent weight parameter types.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, ss.str());
                }
                break;
            }
            case Specification::ActivationParams::NonlinearityTypeCase::kThresholdedReLU:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kSoftsign:
                break;
            default:
            {
                std::stringstream ss;
                ss << "Nonlinearity type ";
                ss << MLActivationParamsNonlinearityType_Name(static_cast<MLActivationParamsNonlinearityType>(params.NonlinearityType_case()));
                ss << " is not supported in this version of CoreML.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, ss.str());
            }
        }
        return Result();
    }


    static Result validateRecurrentActivationParams(const Specification::ActivationParams& params) {
        // make sure params fall into expected values for recurrent activations
        switch (params.NonlinearityType_case()) {
            case Specification::ActivationParams::NonlinearityTypeCase::kLinear:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kSigmoid:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kTanh:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kScaledTanh:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kSigmoidHard:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kReLU:
                break;
            default:
            {
                std::stringstream ss;
                ss << "Recurrent non-linearity type ";
                ss << MLActivationParamsNonlinearityType_Name(static_cast<MLActivationParamsNonlinearityType>(params.NonlinearityType_case()));
                ss << " is not supported in this version of CoreML.";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, ss.str());
            }
        }
        return Result();
    }

    //    ConvolutionLayerParams convolution = 4;
    static Result validateConvolutionLayer(const Specification::NeuralNetworkLayer& layer) {

        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }

        // We need to check if the ConvolutionPaddingType is set
        if (layer.convolution().ConvolutionPaddingType_case() == Specification::ConvolutionLayerParams::ConvolutionPaddingTypeCase::CONVOLUTIONPADDINGTYPE_NOT_SET) {
            std::string err = "Padding type for convolution layer '" + layer.name() + "' is not set.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }

        const auto& params = layer.convolution();
        bool is_deconv = params.isdeconvolution();

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

        bool has_bias = params.hasbias();
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

    //    InnerProductLayerParams innerProduct = 5;
    static Result validateInnerProductLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        if (!r.good()) {
            return r;
        }

        const auto& params = layer.innerproduct();

        uint64_t num_inputs = params.inputchannels();
        uint64_t num_outputs = params.outputchannels();

        bool has_bias = params.hasbias();
        WeightParamType weightsValueType, biasValueType;
        weightsValueType = valueType(params.weights());
        biasValueType = valueType(params.bias());

        // Check for weight and bias value type
        if ((weightsValueType == UNSPECIFIED) || (has_bias && biasValueType == UNSPECIFIED)) {
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Inner product layer '" + layer.name() + "' has invalid weights/bias fields.");
            return r;
        }
        // Check that weight and bias should both be FP16/32, or quantized
        if (has_bias){
            if ((weightsValueType == CoreML::FLOAT16 && biasValueType == CoreML::FLOAT32) ||
                (weightsValueType == CoreML::FLOAT32 && biasValueType == CoreML::FLOAT16)){
                r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Inner product layer '" + layer.name() +
                        "has unmatched precisions of weights/bias They should either be half or full precision.");
                return r;
            }
        }

        // Check weights
        uint64_t weight_size = 0;
        if (weightsValueType == FLOAT32 || weightsValueType == FLOAT16){
            if (weightsValueType == FLOAT32) {
                weight_size = static_cast<uint64_t>(params.weights().floatvalue().size());
            } else {
                weight_size = static_cast<uint64_t>(params.weights().float16value().size() / 2);
            }
            if (num_inputs * num_outputs != weight_size) {
                r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Layer '" + layer.name() +
                        " has incorrect weight matrix size " + std::to_string(weight_size) +
                        " to encode a " + std::to_string(num_inputs) + " x " +
                        std::to_string(num_outputs) + " inner product.");
                return r;
            }
        } else if (weightsValueType == QUINT){
            r = validateGeneralWeightParams(params.weights(), num_inputs * num_outputs, num_outputs,
                                            "Inner Product", layer.name(), "weight");
            if (!r.good()) return r;
        }

        // Check the bias
        uint64_t bias_size = 0;
        if (has_bias){
            if (biasValueType == FLOAT32 || biasValueType == FLOAT16){
                if (biasValueType == FLOAT32){
                    bias_size = static_cast<uint64_t>(params.bias().floatvalue().size());
                } else {
                    bias_size = static_cast<uint64_t>(params.bias().float16value().size() / 2);
                }
                if (bias_size != num_outputs) {
                    std::string err = "Layer '" + layer.name() + "' has incorrect bias vector size " +
                            std::to_string(bias_size) + " (expected " + std::to_string(num_outputs) + ").";
                    r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                    return r;
                }
            } else if (biasValueType == QUINT){
                r = validateGeneralWeightParams(params.bias(), num_outputs, 1, "Inner Product",
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

    //    BatchnormLayerParams batchnorm = 6;
    static Result validateBatchnormLayer(const Specification::NeuralNetworkLayer& layer) {

        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        if (!r.good()){
            return r;
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
    static Result validateActivation(const Specification::NeuralNetworkLayer& layer) {
        // TODO: is there a bound on the number of inputs and outputs here?
        return validateActivationParams(layer.activation());
    }

    //    PoolingLayerParams pooling = 8;
    static Result validatePoolingLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }

        // We need to check if the PoolingPaddingType is set
        if (layer.pooling().PoolingPaddingType_case() == Specification::PoolingLayerParams::PoolingPaddingTypeCase::POOLINGPADDINGTYPE_NOT_SET) {
            std::string err = "Padding type for the pooling layer '" + layer.name() + "' is not set.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        return r;
    }
    //    PaddingLayerParams padding = 9;
    static Result validatePaddingLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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
    static Result validateLRNLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }

        if (layer.lrn().k() < 0.0) {
            std::string err = "Parameter 'K' for the LRN layer '" + layer.name() + "' must be positive.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }

        return r;
    }
    //    SplitLayerParams split = 13;
    static Result validateSplitLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            // between 2 and any number of outputs
            r = validateOutputCount(layer, 2, -1);
        }
        return r;
    }
    //    AddLayerParams add = 14;
    static Result validateAddLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        // 1 or more inputs
        r = validateInputCount(layer, 1, -1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }
    //    MultiplyLayerParams multiply = 15;
    static Result validateMultiplyLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        // 1 or more inputs
        r = validateInputCount(layer, 1, -1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }
    //    UnaryFunctionLayerParams unary = 16;
    static Result validateUnaryFunctionLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }
    //    UpsampleLayerParams upsample = 17;
    static Result validateUpsampleLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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
    static Result validateBiasLayer(const Specification::NeuralNetworkLayer& layer) {

        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        if (!r.good()) {
            return r;
        }

        const auto& params = layer.bias();
        WeightParamType paramType = valueType(params.bias());

        // Only float32 or float16 parameters can be populated at any time
        if (paramType == UNSPECIFIED) {
            std::string err = "Bias product layer '" + layer.name() + "' has both full precision and half precision weights and/or bias fields populated";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }

        size_t total_shape = 1;
        for (int i = 0; i < params.shape_size(); i++) {
            total_shape *= params.shape(i);
        }
        // shape can be ``[1]``, ``[C]``, ``[1, H, W]`` or ``[C, H, W]``
        if (params.shape_size() == 3 && params.shape(0) > 1){
            r = validateGeneralWeightParams(params.bias(), total_shape, params.shape(0), "Bias", layer.name(), "bias");
        } else {
            r = validateGeneralWeightParams(params.bias(), total_shape, 1, "Bias", layer.name(), "bias");
        }
        if (!r.good()) return r;

        if (params.shape_size() < 1 || params.shape_size() > 3) {
            std::string err = "Bias layer '" + layer.name() + "' cannot be " + std::to_string(params.shape_size()) + " dimensional. Must be 1D, 2D, or 3D.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        return r;
    }

    //    L2NormLayerParams l2norm = 19;
    static Result validateL2NormLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }
    //    ReshapeLayerParams reshape = 20;
    static Result validateReshapeLayer(const Specification::NeuralNetworkLayer& layer) {

        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }

        const auto& params = layer.reshape();
        if (params.targetshape_size() != 3 && params.targetshape_size() != 4) {
            std::string err = "Reshape layer '" + layer.name() + "' target shape must be 3D or 4D.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        return r;
    }

    //    FlattenLayerParams flatten = 21;
    static Result validateFlattenLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }

    //    PermuteLayerParams permute = 22;
    static Result validatePermuteLayer(const Specification::NeuralNetworkLayer& layer) {

        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }

        const auto& params = layer.permute();
        if (params.axis_size() != 4) {
            std::string err = "Permute layer '" + layer.name() + "' must have 4D axis parameters.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        return r;
    }

    //    ReduceLayerParams reduce = 23;
    static Result validateReduceLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }

    static Result validateReorganizeDataLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        const auto& reorg = layer.reorganizedata();
        if (static_cast<int>(reorg.blocksize()) < 2) {
            std::string err = "Block size for layer '" + layer.name() + "' must be > 1.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }
        return r;
    }

    static Result validateSliceLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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
    static Result validateLoadConstantLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 0, 0);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        if (!r.good()) {
            return r;
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
    static Result validateScaleLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        if (!r.good()) {
            return r;
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
    static Result validateSimpleRecurrentLayer(const Specification::NeuralNetworkLayer& layer) {

        Result r;
        // Must specify hidden state
        r = validateInputCount(layer, 2, 2);
        if (r.good()) {
            r = validateOutputCount(layer, 2, 2);
        }
        if (!r.good()) {
            return r;
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
    static Result validateGRULayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;

        // Must specify hidden states
        r = validateInputCount(layer, 2, 2);
        if (r.good()) {
            r = validateOutputCount(layer, 2, 2);
        }
        if (!r.good()) {
            return r;
        }

        const auto& params = layer.gru();
        bool hasBiasVectors = params.hasbiasvectors();

        // Validate that all weightParam types match
        WeightParamType gateWeightMatrixValueType, gateRecursionMatrixValueType, gateBiasVectorValueType;
        gateWeightMatrixValueType = valueType(params.updategateweightmatrix());
        gateRecursionMatrixValueType = valueType(params.updategaterecursionmatrix());
        gateBiasVectorValueType = valueType(params.updategatebiasvector());

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
    static Result validateUniDirectionalLSTMLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        // Must specify hidden states
        r = validateInputCount(layer, 3, 3);
        if (r.good()) {
            r = validateOutputCount(layer, 3, 3);
        }
        if (!r.good()) {
            return r;
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
    static Result validateBiDirectionalLSTMLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        // Must specify hidden states
        r = validateInputCount(layer, 5, 5);
        if (r.good()) {
            r = validateOutputCount(layer, 5, 5);
        }
        if (!r.good()) {
            return r;
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
    static Result validateCropLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 2);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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
    static Result validateDotLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        // 2 inputs, 1 output
        r = validateInputCount(layer, 2, 2);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }
//    MeanVarianceNormalizeLayerParams mvn = 35;
    static Result validateMvnLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }
//    EmbeddingLayerParams embedding = 36;
    static Result validateEmbeddingLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }

        const auto& params = layer.embedding();
        uint64_t input_dim = params.inputdim();
        uint64_t output_channels = params.outputchannels();

        bool has_bias = params.hasbias();
        WeightParamType weightsValueType, biasValueType;
        weightsValueType = valueType(params.weights());
        biasValueType = valueType(params.bias());

        // Only float32 or float16 parameters can be populated at any time
        if ((weightsValueType == UNSPECIFIED) || (has_bias && biasValueType == UNSPECIFIED)){
            std::string err = "Embedding layer '" + layer.name() + "' has invalid weights/bias fields. Field value types should match and should either be half or full precision.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }

        if (has_bias){
            if ((weightsValueType == CoreML::FLOAT16 && biasValueType == CoreML::FLOAT32) ||
                (weightsValueType == CoreML::FLOAT32 && biasValueType == CoreML::FLOAT16)){
                r = Result(ResultType::INVALID_MODEL_PARAMETERS, "Embedding layer '" + layer.name() +
                           "has unmatched precisions of weights/bias They should either be half or full precision.");
                return r;
            }
        }
        // Validate weight and bias sizes
        r = validateGeneralWeightParams(params.weights(), input_dim * output_channels, output_channels,
                                        "Embedding", layer.name(), "weight");
        if (!r.good()) return r;
        if (has_bias){
            r = validateGeneralWeightParams(params.bias(), output_channels, 1,
                                            "Embedding", layer.name(), "bias");
            if (!r.good()) return r;
        }

        return r;
    }

    static Result validateAverageLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, -1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }

    static Result validateMaxLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, -1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }
    static Result validateMinLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, -1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }

//    SequenceRepeatLayerParams sequenceRepeat = 37;
    static Result validateSequenceRepeatLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }

    static Result validateSoftmaxLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }

    static Result validateConcatLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 2, -1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
        }
        return r;
    }


    static Result validateCustomLayer(const Specification::NeuralNetworkLayer& layer) {
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
    static Result validateResizeBilinearLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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
    static Result validateCropResizeLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 2, 2);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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

    static Result validateFailUnknownType(const Specification::NeuralNetworkLayer& layer) {
        return Result(ResultType::INVALID_MODEL_PARAMETERS, "Unsupported layer type (" + layer.GetTypeName() + ") for layer '" + layer.name() + "'.");
    }


    typedef std::function<Result(const CoreML::Specification::NeuralNetworkLayer& specLayer)> validateSpecLayerFn;

    validateSpecLayerFn
    static getValidateFunctionFromTag(const Specification::NeuralNetworkLayer::LayerCase& layerType) {
        switch (layerType) {
                //                    Convolution2DLayerParams convolution = 4;
            case Specification::NeuralNetworkLayer::LayerCase::kConvolution:
                return validateConvolutionLayer;
                //                    InnerProductLayerParams innerProduct = 5;
            case Specification::NeuralNetworkLayer::LayerCase::kInnerProduct:
                return validateInnerProductLayer;
                //                    BatchnormLayerParams batchnorm = 6;
            case Specification::NeuralNetworkLayer::LayerCase::kBatchnorm:
                return validateBatchnormLayer;
                //                    ActivationParams activation = 7;
            case Specification::NeuralNetworkLayer::LayerCase::kActivation:
                return validateActivation;
                //                    PoolingLayerParams pooling = 8;
            case Specification::NeuralNetworkLayer::LayerCase::kPooling:
                return validatePoolingLayer;
                //                    PaddingLayerParams padding = 9;
            case Specification::NeuralNetworkLayer::LayerCase::kPadding:
                return validatePaddingLayer;
                //                    ConcatLayerParams concat = 10;
            case Specification::NeuralNetworkLayer::LayerCase::kConcat:
                return validateConcatLayer;
                //                    LRNLayerParams lrn = 11;
            case Specification::NeuralNetworkLayer::LayerCase::kLrn:
                return validateLRNLayer;
                //                    SoftmaxLayerParams softmax = 12;
            case Specification::NeuralNetworkLayer::LayerCase::kSoftmax:
                return validateSoftmaxLayer;
                //                    SplitLayerParams split = 13;
            case Specification::NeuralNetworkLayer::LayerCase::kSplit:
                return validateSplitLayer;
                //                    AddLayerParams add = 14;
            case Specification::NeuralNetworkLayer::LayerCase::kAdd:
                return validateAddLayer;
                //                    MultiplyLayerParams multiply = 15;
            case Specification::NeuralNetworkLayer::LayerCase::kMultiply:
                return validateMultiplyLayer;
                //                    UnaryFunctionLayerParams unary = 16;
            case Specification::NeuralNetworkLayer::LayerCase::kUnary:
                return validateUnaryFunctionLayer;
                //                    UpsampleLayerParams upsample = 17;
            case Specification::NeuralNetworkLayer::LayerCase::kUpsample:
                return validateUpsampleLayer;
                //                    BiasLayerParams bias = 18;
            case Specification::NeuralNetworkLayer::LayerCase::kBias:
                return validateBiasLayer;
                //                    L2NormLayerParams l2norm = 19;
            case Specification::NeuralNetworkLayer::LayerCase::kL2Normalize:
                return validateL2NormLayer;
                //                    ReshapeLayerParams reshape = 20;
            case Specification::NeuralNetworkLayer::LayerCase::kReshape:
                return validateReshapeLayer;
                //                    FlattenLayerParams flatten = 21;
            case Specification::NeuralNetworkLayer::LayerCase::kFlatten:
                return validateFlattenLayer;
                //                    PermuteLayerParams permute = 22;
            case Specification::NeuralNetworkLayer::LayerCase::kPermute:
                return validatePermuteLayer;
                //                    ReduceLayerParams reduce = 23;
            case Specification::NeuralNetworkLayer::LayerCase::kReduce:
                return validateReduceLayer;
                //                    LoadConstantLayerParams loadConstant = 24;
            case Specification::NeuralNetworkLayer::LayerCase::kLoadConstant:
                return validateLoadConstantLayer;
                //                    ScaleLayerParams scale = 25;
            case Specification::NeuralNetworkLayer::LayerCase::kScale:
                return validateScaleLayer;
                //                    SimpleRecurrentLayerParams simpleRecurrent = 26;
            case Specification::NeuralNetworkLayer::LayerCase::kSimpleRecurrent:
                return validateSimpleRecurrentLayer;
                //                    GRULayerParams gru = 27;
            case Specification::NeuralNetworkLayer::LayerCase::kGru:
                return validateGRULayer;
                //                    UniDirectionalLSTMLayerParams uniDirectionalLSTM = 28;
            case Specification::NeuralNetworkLayer::LayerCase::kUniDirectionalLSTM:
                return validateUniDirectionalLSTMLayer;
                //                    BiDirectionalLSTMLayerParams biDirectionalLSTM = 29;
            case Specification::NeuralNetworkLayer::LayerCase::kBiDirectionalLSTM:
                return validateBiDirectionalLSTMLayer;
                //    CropLayerParams crop = 30;
            case Specification::NeuralNetworkLayer::LayerCase::kCrop:
                return validateCropLayer;
                //    AverageLayerParams average = 31;
            case Specification::NeuralNetworkLayer::LayerCase::kAverage:
                return validateAverageLayer;
                //    MaxLayerParams max = 32;
            case Specification::NeuralNetworkLayer::LayerCase::kMax:
                return validateMaxLayer;
                //    MinLayerParams min = 33;
            case Specification::NeuralNetworkLayer::LayerCase::kMin:
                return validateMinLayer;
                //    DotProductLayerParams dot = 34;
            case Specification::NeuralNetworkLayer::LayerCase::kDot:
                return validateDotLayer;
                //    MeanVarianceNormalizeLayerParams mvn = 35;
            case Specification::NeuralNetworkLayer::LayerCase::kMvn:
                return validateMvnLayer;
                //    EmbeddingLayerParams embedding = 36;
            case Specification::NeuralNetworkLayer::LayerCase::kEmbedding:
                return validateEmbeddingLayer;
                //    SequenceRepeatLayerParams sequenceRepeat = 37;
            case Specification::NeuralNetworkLayer::LayerCase::kSequenceRepeat:
                return validateSequenceRepeatLayer;
            case Specification::NeuralNetworkLayer::LayerCase::kReorganizeData:
                return validateReorganizeDataLayer;
            case Specification::NeuralNetworkLayer::LayerCase::kSlice:
                return validateSliceLayer;
            case Specification::NeuralNetworkLayer::LayerCase::kCustom:
                return validateCustomLayer;
            case Specification::NeuralNetworkLayer::LayerCase::kResizeBilinear:
                return validateResizeBilinearLayer;
            case Specification::NeuralNetworkLayer::LayerCase::kCropResize:
                return validateCropResizeLayer;
            default:
                return validateFailUnknownType;
        }
    }

#pragma mark Network-Wide Validation

    template<typename T>
    static Result validateNeuralNetwork(const Specification::ModelDescription& interface,
                                        const T& nn, std::set<std::string>& outputBlobNames) {
        Result r;

        if (interface.input_size() == 0) {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Neural networks require at least one input.");
        }

        if (interface.output_size() == 0) {
            return Result(ResultType::INVALID_MODEL_INTERFACE,
                          "Neural networks produce at least one output.");
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


        // For each named data blob, the name of the node which produced it
        std::map<std::string, std::string> blobNameToProducingLayerName;

        for (const auto& input: interface.input()) {
            // For input blobs, we'll give them a dummy producing layer name
            blobNameToProducingLayerName[input.name()] = "__input";
            if (input.type().Type_case() == Specification::FeatureType::kMultiArrayType) {
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
            }
        }

        for (const auto& layer : nn.layers()) {

            // First, we check the layer for internal correctness

            validateSpecLayerFn validateConvertFn = getValidateFunctionFromTag(layer.layer_case());
            r = validateConvertFn(layer);

            if (!r.good()) {
                return r;
            }

            // Check for topological defects: the layer's input must have been produced by a blob we have
            // already seen. Also, check that the same output isn't being produced in two different places.
            for (const auto& input: layer.input()) {
                if (blobNameToProducingLayerName.find(input) == blobNameToProducingLayerName.end()) {
                    std::string err = "Layer '" + std::string(layer.name()) + "' consumes an input named '"
                        + std::string(input) + "' which is not present in this network.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
            for (const auto& output: layer.output()) {
                if (blobNameToProducingLayerName.find(output) != blobNameToProducingLayerName.end()) {
                    std::string err = "Layer '" + std::string(layer.name()) + "' produces an output named '"
                        + std::string(output) + "' which is also an output produced by the layer '"
                        + blobNameToProducingLayerName[output] + "'.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
                blobNameToProducingLayerName[output] = layer.name();
                outputBlobNames.insert(output);
            }

        } // loop over layers

        // Compute the shapes
        try {
            NeuralNetworkShaper shaper(interface, nn.layers());
        }
        catch(std::runtime_error& e) {
            std::string err = std::string("Error determining network blob shapes: ") + std::string(e.what());
            return Result(ResultType::POTENTIALLY_INVALID_NEURAL_NETWORK_SHAPES, err);
        }

        return Result();

    }

    template <>
    Result validate<MLModelType_neuralNetworkClassifier>(const Specification::Model& format) {
        // must have classifier parameters
        Result r = validateClassifierInterface(format, format.neuralnetworkclassifier());
        if (!r.good()) {
            return r;
        }

        std::set<std::string> outputBlobNames;
        r = validateNeuralNetwork(format.description(), format.neuralnetworkclassifier(), outputBlobNames);

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
        return validateNeuralNetwork(format.description(), format.neuralnetworkregressor(), outputBlobNames);
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

        Result r = validateNeuralNetwork(format.description(), format.neuralnetwork(), outputBlobNames);

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
