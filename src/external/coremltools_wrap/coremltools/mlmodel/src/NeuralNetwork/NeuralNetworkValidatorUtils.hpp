//
//  NeuralNetworkValidatorUtils.cpp
//  mlmodel
//
//  Created by aseem wadhwa on 10/27/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#include "NeuralNetworkValidator.hpp"
#include "../Utils.hpp"

#include <algorithm>
#include <sstream>

using namespace CoreML;

inline Result validateTensorMessage(const Specification::Tensor& tensor, const Specification::NeuralNetworkLayer& layer) {
    std::string err;
    int rank = static_cast<int>(tensor.rank());
    if (tensor.dimvalue_size() > 0) {
        if (rank != tensor.dimvalue_size()) {
            err = "Tensor in layer '" + std::string(layer.name()) + "': rank must match the length of dimValue";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (rank < 1) {
            err = "Tensor in layer '" + std::string(layer.name()) + "': rank must be positive";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return Result();
}

inline Result checkRank(const Specification::NeuralNetworkLayer& layer,
                        const std::string &layerType, int min, int max,
                        const std::string &blobType, int rank) {
    
    // blobType: "input" or "output"
    
    assert( min <= max || max < 0 );
    std::string err;
    
    if (max > 0 && max == min && rank != max) {
        err = "Layer '" + layer.name() + "' of type '" + layerType + + "' has " + blobType  + " rank " + std::to_string(rank) + " but expects rank exactly " + std::to_string(min) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else if (min > 0 && rank < min) {
        err = "Layer '" + layer.name() + "' of type '" + layerType + + "' has " + blobType + " rank " + std::to_string(rank) + " but expects rank at least " + std::to_string(min) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else if (max > 0 && rank > max) {
        err = "Layer '" + layer.name() + "' of type '" + layerType + "' has " + blobType + " rank " + std::to_string(rank) + " but expects rank at most " + std::to_string(max) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    } else {
        return Result();
    }
}

inline Result validateRankCount(const Specification::NeuralNetworkLayer& layer,
                                const std::string &layerType, int min, int max,
                                std::map<std::string, int>& blobNameToRank) {
    
    Result r;
    
    // check that 1st input's rank is within permissible limits
    if (blobNameToRank.find(layer.input(0)) != blobNameToRank.end()) {
        int rank = blobNameToRank.at(layer.input(0));
        r = checkRank(layer, layerType, min, max, "input", rank);
    }
    
    if (!r.good()) {return r;}
    
    // check that 2nd input's rank is within permissible limits
    if (blobNameToRank.find(layer.output(0)) != blobNameToRank.end()) {
        int rank = blobNameToRank.at(layer.output(0));
        r = checkRank(layer, layerType, min, max, "output", rank);
    }
    return r;
}

inline Result validateInputOutputRankEquality(const Specification::NeuralNetworkLayer& layer, std::string layerType,
                                              std::map<std::string, int>& blobNameToRank) {
    
    if (blobNameToRank.find(layer.input(0)) != blobNameToRank.end() &&
        blobNameToRank.find(layer.output(0)) != blobNameToRank.end()) {
        if (blobNameToRank.at(layer.input(0)) != blobNameToRank.at(layer.output(0))) {
            std::string err;
            err = "Layer '" + std::string(layer.name()) + "' of type '" + layerType + "' expects equal ranks for its input and output, but they are not equal.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
    }
    return Result();
}

// Min and max are the minimum and maximum number of possible inputs.
// negative values are interpreted as no bound
inline Result validateInputCount(const Specification::NeuralNetworkLayer& layer, int min, int max) {
    
    assert( min <= max || max < 0 );
    std::string err;
    
    if (max > 0 && max == min && layer.input_size() != max) {
        err = "Layer '" + std::string(layer.name()) + "' of type " + std::to_string(layer.layer_case()) + " has " + std::to_string(layer.input_size()) + " inputs but expects exactly " + std::to_string(min) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else if (min > 0 && layer.input_size() < min) {
        err = "Layer '" + std::string(layer.name()) + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " inputs but expects at least " + std::to_string(min) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else if (max > 0 && layer.input_size() > max) {
        err = "Layer '" + std::string(layer.name()) + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " inputs but expects at most " + std::to_string(max) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else {
        return Result();
    }
}

inline Result validateOutputCount(const Specification::NeuralNetworkLayer& layer, int min, int max) {
    
    assert( min <= max || max < 0 );
    std::string err;
    
    if (max > 0 && max == min && layer.output_size() != max) {
        err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.output_size()) + " outputs but expects exactly " + std::to_string(min) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else if (min > 0 && layer.output_size() < min) {
        err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.output_size()) + " outputs but expects at least " + std::to_string(min) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else if (max > 0 && layer.output_size() > max) {
        err = "Layer '" + layer.name() + "' of type " + std::to_string(layer.layer_case()) + " has " + std::to_string(layer.output_size()) + " outputs but expects at most " + std::to_string(max) + ".";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    else {
        return Result();
    }
}

inline Result validateRankExists(const Specification::NeuralNetworkLayer& layer) {
    
    if (layer.inputtensor_size() == 0 || layer.outputtensor_size() == 0) {
        std::string err = "Layer '" + std::string(layer.name()) + "' must have rank specified for its input and output.";
        return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
    }
    return Result();
}

inline Result validateGeneralWeightParams(const CoreML::Specification::WeightParams& weight,
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

inline bool isWeightParamTypeCompatible(const std::vector<WeightParamType>& weightTypes){
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

inline Result validateLSTMWeightParams(const Specification::LSTMWeightParams& lstmWeightParams, const Specification::LSTMParams lstmParams) {
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

inline Result validateActivationParams(const Specification::ActivationParams& params) {
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

inline Result validateRecurrentActivationParams(const Specification::ActivationParams& params) {
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


