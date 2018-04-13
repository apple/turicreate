/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "../build/format/NeuralNetwork_enums.h"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "ValidatorClassifiers-impl.hpp"
#include "transforms/NeuralNetwork.hpp"

#include <algorithm>
#include <sstream>
#include <type_traits>

namespace CoreML {

#pragma mark Layer Specific Functions

    // Min and max are the minimum and maximum number of possible inputs.
    // negative values are interpreted as no bound
    static Result validateInputCount(const Specification::NeuralNetworkLayer& layer, int min, int max) {

        assert( min <= max || max < 0 );
        std::string err;

        if (max > 0 && max == min && layer.input_size() != max) {
            err = "Layer " + layer.name() + " of type " + std::to_string(layer.layer_case()) + " has " + std::to_string(layer.input_size()) + " inputs, but needs exactly " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (min > 0 && layer.input_size() < min) {
            err = "Layer " + layer.name() + " of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " inputs, but needs at least " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (max > 0 && layer.input_size() > max) {
            err = "Layer " + layer.name() + " of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " inputs, but requires at least " + std::to_string(max) + ".";
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
            err = "Layer " + layer.name() + " of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " outputs, but needs exactly " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (min > 0 && layer.output_size() < min) {
            err = "Layer " + layer.name() + " of type " + std::to_string(layer.layer_case()) + + " has " + std::to_string(layer.input_size()) + " outputs, but needs at least " + std::to_string(min) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else if (max > 0 && layer.output_size() > max) {
            err = "Layer " + layer.name() + " of type " + std::to_string(layer.layer_case()) + " has " + std::to_string(layer.input_size()) + " outputs, but requires at least " + std::to_string(max) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        else {
            return Result();
        }
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
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kParametricSoftplus:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kThresholdedReLU:
                break;
            case Specification::ActivationParams::NonlinearityTypeCase::kSoftsign:
                break;
            default:
            {
                std::stringstream ss;
                ss << "Unsupported nonlinearity type ";
                ss << MLActivationParamsNonlinearityType_Name(static_cast<MLActivationParamsNonlinearityType>(params.NonlinearityType_case()));
                ss << " provided to layer activation.";
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
                ss << "Unsupported nonlinearity type ";
                ss << MLActivationParamsNonlinearityType_Name(static_cast<MLActivationParamsNonlinearityType>(params.NonlinearityType_case()));
                ss << " provided to recurrent activation.";
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

        if (params.hasbias() && params.bias().floatvalue().size() != num_outputs) {
            std::string err = "Layer " + layer.name() + " has incorrect bias vector size " + std::to_string(params.bias().floatvalue().size()) + " (expected " + std::to_string(num_outputs) + ").";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }
        else if (!params.hasbias() && params.bias().floatvalue().size() > 0) {

            std::string err = "Bias vector being ignored since \"hasBias\" flag not set.";
            r = Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            return r;
        }

        if (num_inputs * num_outputs != params.weights().floatvalue().size()) {
            std::string err = "Incorrect weight matrix size " + std::to_string(params.weights().floatvalue().size()) + " for inner product of size " + std::to_string(num_inputs) + " x " + std::to_string(num_outputs) + ".";
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

        if (r.good()) {
            int num_channels = static_cast<int>(layer.batchnorm().channels());

            if (num_channels != layer.batchnorm().gamma().floatvalue().size()) {
                std::string err = "In layer " + layer.name() + ": incorrect gamma size " + std::to_string(layer.batchnorm().gamma().floatvalue().size()) + ".";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }

            if (num_channels != layer.batchnorm().beta().floatvalue().size()) {
                std::string err = "In layer " + layer.name() + ": incorrect beta size " + std::to_string(layer.batchnorm().beta().floatvalue().size()) + ".";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }

            if (num_channels != layer.batchnorm().mean().floatvalue().size()) {
                std::string err = "In layer " + layer.name() + ": incorrect mean size " + std::to_string(layer.batchnorm().mean().floatvalue().size()) + ".";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }

            if (num_channels != layer.batchnorm().variance().floatvalue().size()) {
                std::string err = "In layer " + layer.name() + ": incorrect variance size " + std::to_string(layer.batchnorm().variance().floatvalue().size()) + ".";
                return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
            }
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
        return r;
    }
    //    PaddingLayerParams padding = 9;
    static Result validatePaddingLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 1);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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

        size_t total_shape = 1;
        for (int i = 0; i < params.shape_size(); i++) {
            total_shape *= params.shape(i);
        }

        if (total_shape != params.bias().floatvalue().size()) {
            std::string err = "Incorrect bias layer (name: " + layer.name() + "). Have bias vector of length " + std::to_string(params.bias().floatvalue().size()) + " but expected length  " + std::to_string(total_shape) + ".";

            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        if (params.shape_size() < 1 || params.shape_size() > 3) {
            std::string err = "Incorrect bias layer (name: " + layer.name() + "). Has shape vector of length " + std::to_string(params.shape_size()) + " but requires 1,2, or 3 elements.";
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
            std::string err = "Reshape layer " + layer.name() + " requires a target shape exactly 3 or 4 elements.";
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
            std::string err = "Permute layer " + layer.name() + " requires axis parameter of length 4.";
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

        if (params.shape_size() != 3) {
            std::string err = "Load constant layer " + layer.name() + " requires a shape array of length 3.";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        size_t total_shape = 1;
        for (int i = 0; i < params.shape_size(); i++) {
            total_shape *= params.shape(i);
        }

        if (total_shape != params.data().floatvalue().size()) {
            std::string err = "Incorrect load constant layer (name: " + layer.name() + "). Have data buffer of length " + std::to_string(params.data().floatvalue().size()) + " but expected length  " + std::to_string(total_shape) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

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

        size_t total_bias_shape = 1;
        for (int i = 0; i < params.shapebias_size(); i++) {
            total_bias_shape *= params.shapebias(i);
        }

        if (total_bias_shape != params.bias().floatvalue().size()) {
            std::string err = "Incorrect scale layer (name: " + layer.name() + "). Have bias vector of length " + std::to_string(params.bias().floatvalue().size()) + " but expected length  " + std::to_string(total_bias_shape) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        size_t total_scale_shape = 1;
        for (int i = 0; i < params.shapescale_size(); i++) {
            total_scale_shape *= params.shapescale(i);
        }

        if (total_scale_shape != params.scale().floatvalue().size()) {
            std::string err = "Incorrect scale layer (name: " + layer.name() + "). Have bias vector of length " + std::to_string(params.scale().floatvalue().size()) + " but expected length  " + std::to_string(total_scale_shape) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
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

//        //size of the input vectors
//        uint64 inputVectorSize = 1;
//        //size of the output
//        uint64 outputVectorSize = 2;
//
//        ActivationParams activation = 3; //typical value used = sigmoid
//        // isOutputASequence: if false output is the result after final state update. If true, output is a sequence, containing outputs at all time steps
//        bool isOutputASequence = 4;
//
//        bool hasBiasVector = 5; //if False no bias is added
//
//        //Weights matrices:
//        repeated float weightMatrix = 6; //W
//        repeated float recursionMatrix = 7; //R
//        //Bias:
//        repeated float biasVector = 8; //b
//
//        bool reverseInput = 100;

        const auto& params = layer.simplerecurrent();

        // Check the size of the input matrices
        uint64_t input_matrix_size = params.inputvectorsize() * params.outputvectorsize();
        if (input_matrix_size != params.weightmatrix().floatvalue().size()) {
            std::string err = "Simple Recurrent layer " + layer.name() + " expects input matrix of size " + std::to_string(params.inputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.weightmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        // Check the size of the recurrent matrices
        uint64_t recurrent_matrix_size = params.outputvectorsize() * params.outputvectorsize();
        if (recurrent_matrix_size != params.recursionmatrix().floatvalue().size()) {
            std::string err = "Simple Recurrent layer " + layer.name() + " expects recursion matrix of size " + std::to_string(params.outputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.recursionmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        if (params.hasbiasvector() && params.biasvector().floatvalue().size() != params.outputvectorsize()) {
            std::string err = "Simple Recurrent layer " + layer.name() + " has bias vector of size " + std::to_string(params.biasvector().floatvalue().size()) + " but expects size " + std::to_string(params.outputvectorsize()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
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

        // Check the size of the input matrices
        uint64_t input_matrix_size = params.inputvectorsize() * params.outputvectorsize();
        if (input_matrix_size != params.updategateweightmatrix().floatvalue().size()) {
            std::string err = "GRU layer " + layer.name() + " expects update gate weight matrix of size " + std::to_string(params.inputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.updategateweightmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (input_matrix_size != params.resetgateweightmatrix().floatvalue().size()) {
            std::string err = "GRU layer " + layer.name() + " expects reset gate matrix of size " + std::to_string(params.inputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.resetgateweightmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (input_matrix_size != params.outputgateweightmatrix().floatvalue().size()) {
            std::string err = "GRU layer " + layer.name() + " expects output gate matrix of size " + std::to_string(params.inputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.outputgateweightmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        // Check the size of the recurrent matrices
        uint64_t recurrent_matrix_size = params.outputvectorsize() * params.outputvectorsize();
        if (recurrent_matrix_size != params.updategaterecursionmatrix().floatvalue().size()) {
            std::string err = "GRU layer " + layer.name() + " expects update gate recursion matrix of size " + std::to_string(params.outputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.updategaterecursionmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (recurrent_matrix_size != params.resetgaterecursionmatrix().floatvalue().size()) {
            std::string err = "GRU layer " + layer.name() + " expects reset gate recursion matrix of size " + std::to_string(params.outputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.resetgaterecursionmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (recurrent_matrix_size != params.outputgaterecursionmatrix().floatvalue().size()) {
            std::string err = "GRU layer " + layer.name() + " expects output gate recursion matrix of size " + std::to_string(params.outputvectorsize()) + " x " + std::to_string(params.outputvectorsize()) + " but provides " + std::to_string(params.outputgaterecursionmatrix().floatvalue().size()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }

        // Check the size of the biases
        if (params.hasbiasvectors() && params.updategatebiasvector().floatvalue().size() != params.outputvectorsize()) {
            std::string err = "GRU layer " + layer.name() + " has update bias vector of size " + std::to_string(params.updategatebiasvector().floatvalue().size()) + " but expects size " + std::to_string(params.outputvectorsize()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (params.hasbiasvectors() && params.resetgatebiasvector().floatvalue().size() != params.outputvectorsize()) {
            std::string err = "GRU layer " + layer.name() + " has reset bias vector of size " + std::to_string(params.resetgatebiasvector().floatvalue().size()) + " but expects size " + std::to_string(params.outputvectorsize()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
        }
        if (params.hasbiasvectors() && params.outputgatebiasvector().floatvalue().size() != params.outputvectorsize()) {
            std::string err = "GRU layer " + layer.name() + " has output bias vector of size " + std::to_string(params.outputgatebiasvector().floatvalue().size()) + " but expects size " + std::to_string(params.outputvectorsize()) + ".";
            return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
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
        return r;
    }

//    CropLayerParams crop = 30;
    static Result validateCropLayer(const Specification::NeuralNetworkLayer& layer) {
        Result r;
        r = validateInputCount(layer, 1, 2);
        if (r.good()) {
            r = validateOutputCount(layer, 1, 1);
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

    static Result validateFailUnknownType(const Specification::NeuralNetworkLayer& layer) {
        return Result(ResultType::INVALID_MODEL_PARAMETERS, "Unknown Layer type.");
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
                                           "Neural Networks only accept arrays or images as inputs.");
                             }

        
        // For each named data blob, which node produced it
        std::map<std::string, Specification::NeuralNetworkLayer> blobNameToProducingLayer;

        // For each named data blob, the shape it expects
        // This is only K,H,W, since sequence and batch sizes are not specified by the proto
        std::map<std::string, std::vector<int> > blobNameToShape;

        // For input blobs, we'll give them a dummy producing layer
        std::string inputName = "__input";
        Specification::NeuralNetworkLayer dummyInputLayer;
        dummyInputLayer.set_name(inputName);

        for (const auto& input: interface.input()) {
            blobNameToProducingLayer[input.name()] = dummyInputLayer;
            if (input.type().Type_case() == Specification::FeatureType::kImageType) {
                std::vector<int> shape(3);
                if (input.type().imagetype().colorspace() == Specification::ImageFeatureType_ColorSpace::ImageFeatureType_ColorSpace_GRAYSCALE) {
                    shape[0] = 1;
                }
                else if (input.type().imagetype().colorspace() == Specification::ImageFeatureType_ColorSpace::ImageFeatureType_ColorSpace_RGB) {
                    shape[0] = 3;
                } else if (input.type().imagetype().colorspace() == Specification::ImageFeatureType_ColorSpace::ImageFeatureType_ColorSpace_BGR) {
                    shape[0] = 3;
                }
                else {
                    return Result(ResultType::INVALID_MODEL_INTERFACE, "Invalid image color space.");
                }
                shape[1] = static_cast<int>(input.type().imagetype().height());
                shape[2] = static_cast<int>(input.type().imagetype().width());
                blobNameToShape[input.name()] = shape;
            }
            else { // Array
                // we already checked that it's already an array or image
                
                // only vector-like (rank 1) or image-like (rank 3) inputs are allowed
                if (!(input.type().multiarraytype().shape().size() == 1
                    || input.type().multiarraytype().shape().size() == 3)) {
                    return Result(ResultType::INVALID_MODEL_INTERFACE, "Input arrays to neural networks must be rank 1 (single vectors) or rank 3 (image-like arrays).");
                }
                
                blobNameToShape[input.name()] = std::vector<int>(input.type().multiarraytype().shape().begin(),
                                                                 input.type().multiarraytype().shape().end());
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
                if (blobNameToProducingLayer.find(input) == blobNameToProducingLayer.end()) {
                    std::string err = "Layer " + std::string(layer.name()) + " consumes a data blob named "
                        + std::string(input) + " which is not an input and not produced by a blob reached yet.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
            for (const auto& output: layer.output()) {
                if (blobNameToProducingLayer.find(output) != blobNameToProducingLayer.end()) {
                    std::string err = "Layer " + std::string(layer.name()) + " produces a data blob named "
                        + std::string(output) + " which was already given as being produced by layer "
                        + blobNameToProducingLayer[output].name() + ".";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
                blobNameToProducingLayer[output] = layer;
                outputBlobNames.insert(output);
            }

        } // loop over layers
        
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
                std::string err = "Probabilities should be obtained from blob " + probBlob + ", but this blob was not found in any layer of the network.";
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
                    std::string err = "Output " + output.name() + " cannot be produced by any layer of the neural network.";
                    return Result(ResultType::INVALID_MODEL_PARAMETERS, err);
                }
            }
        }
        
        return r;
        
    }

    template <>
    Result validate<MLModelType_neuralNetworkRegressor>(const Specification::Model& format) {
        // must have regressor parameters
        Result r = validateRegressorInterface(format.description());
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
                         [](decltype(*(interface.output().begin())) output) {
                             return output.type().Type_case() == Specification::FeatureType::kMultiArrayType ||
                                    output.type().Type_case() == Specification::FeatureType::kImageType;
                         })) {
                             return Result(ResultType::INVALID_MODEL_INTERFACE,
                                           "Neural Networks only return arrays as outputs.");
                         }
        
        std::set<std::string> outputBlobNames;
        
        Result r = validateNeuralNetwork(format.description(), format.neuralnetwork(), outputBlobNames);
        
        if (r.good()) {
            // Make sure that all of the model interface's outputs are actually produced by some blob
            for (const auto& output : format.description().output()) {
                
                const std::string& name = output.name();
                
                std::string err;
                if (outputBlobNames.count(name) == 0) {
                    err = "Interface specifies output: " + name + ", but no node in the network produces it.";
                    return Result(ResultType::INVALID_MODEL_INTERFACE, err);
                }
                outputBlobNames.erase(name);
                
            }
        }
        
        return r;
        
    }

}
