/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __NEURALNETWORK_ENUMS_H
#define __NEURALNETWORK_ENUMS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
enum MLNeuralNetworkPreprocessingpreprocessor: int {
    MLNeuralNetworkPreprocessingpreprocessor_scaler = 10,
    MLNeuralNetworkPreprocessingpreprocessor_meanImage = 11,
    MLNeuralNetworkPreprocessingpreprocessor_NOT_SET = 0,
};

static const char * MLNeuralNetworkPreprocessingpreprocessor_Name(MLNeuralNetworkPreprocessingpreprocessor x) {
    switch (x) {
        case MLNeuralNetworkPreprocessingpreprocessor_scaler:
            return "MLNeuralNetworkPreprocessingpreprocessor_scaler";
        case MLNeuralNetworkPreprocessingpreprocessor_meanImage:
            return "MLNeuralNetworkPreprocessingpreprocessor_meanImage";
        case MLNeuralNetworkPreprocessingpreprocessor_NOT_SET:
            return "INVALID";
    }
}

enum MLActivationParamsNonlinearityType: int {
    MLActivationParamsNonlinearityType_linear = 5,
    MLActivationParamsNonlinearityType_ReLU = 10,
    MLActivationParamsNonlinearityType_leakyReLU = 15,
    MLActivationParamsNonlinearityType_thresholdedReLU = 20,
    MLActivationParamsNonlinearityType_PReLU = 25,
    MLActivationParamsNonlinearityType_tanh = 30,
    MLActivationParamsNonlinearityType_scaledTanh = 31,
    MLActivationParamsNonlinearityType_sigmoid = 40,
    MLActivationParamsNonlinearityType_sigmoidHard = 41,
    MLActivationParamsNonlinearityType_ELU = 50,
    MLActivationParamsNonlinearityType_softsign = 60,
    MLActivationParamsNonlinearityType_softplus = 70,
    MLActivationParamsNonlinearityType_parametricSoftplus = 71,
    MLActivationParamsNonlinearityType_NOT_SET = 0,
};

static const char * MLActivationParamsNonlinearityType_Name(MLActivationParamsNonlinearityType x) {
    switch (x) {
        case MLActivationParamsNonlinearityType_linear:
            return "MLActivationParamsNonlinearityType_linear";
        case MLActivationParamsNonlinearityType_ReLU:
            return "MLActivationParamsNonlinearityType_ReLU";
        case MLActivationParamsNonlinearityType_leakyReLU:
            return "MLActivationParamsNonlinearityType_leakyReLU";
        case MLActivationParamsNonlinearityType_thresholdedReLU:
            return "MLActivationParamsNonlinearityType_thresholdedReLU";
        case MLActivationParamsNonlinearityType_PReLU:
            return "MLActivationParamsNonlinearityType_PReLU";
        case MLActivationParamsNonlinearityType_tanh:
            return "MLActivationParamsNonlinearityType_tanh";
        case MLActivationParamsNonlinearityType_scaledTanh:
            return "MLActivationParamsNonlinearityType_scaledTanh";
        case MLActivationParamsNonlinearityType_sigmoid:
            return "MLActivationParamsNonlinearityType_sigmoid";
        case MLActivationParamsNonlinearityType_sigmoidHard:
            return "MLActivationParamsNonlinearityType_sigmoidHard";
        case MLActivationParamsNonlinearityType_ELU:
            return "MLActivationParamsNonlinearityType_ELU";
        case MLActivationParamsNonlinearityType_softsign:
            return "MLActivationParamsNonlinearityType_softsign";
        case MLActivationParamsNonlinearityType_softplus:
            return "MLActivationParamsNonlinearityType_softplus";
        case MLActivationParamsNonlinearityType_parametricSoftplus:
            return "MLActivationParamsNonlinearityType_parametricSoftplus";
        case MLActivationParamsNonlinearityType_NOT_SET:
            return "INVALID";
    }
}

enum MLNeuralNetworkLayerlayer: int {
    MLNeuralNetworkLayerlayer_convolution = 100,
    MLNeuralNetworkLayerlayer_pooling = 120,
    MLNeuralNetworkLayerlayer_activation = 130,
    MLNeuralNetworkLayerlayer_innerProduct = 140,
    MLNeuralNetworkLayerlayer_embedding = 150,
    MLNeuralNetworkLayerlayer_batchnorm = 160,
    MLNeuralNetworkLayerlayer_mvn = 165,
    MLNeuralNetworkLayerlayer_l2normalize = 170,
    MLNeuralNetworkLayerlayer_softmax = 175,
    MLNeuralNetworkLayerlayer_lrn = 180,
    MLNeuralNetworkLayerlayer_crop = 190,
    MLNeuralNetworkLayerlayer_padding = 200,
    MLNeuralNetworkLayerlayer_upsample = 210,
    MLNeuralNetworkLayerlayer_unary = 220,
    MLNeuralNetworkLayerlayer_add = 230,
    MLNeuralNetworkLayerlayer_multiply = 231,
    MLNeuralNetworkLayerlayer_average = 240,
    MLNeuralNetworkLayerlayer_scale = 245,
    MLNeuralNetworkLayerlayer_bias = 250,
    MLNeuralNetworkLayerlayer_max = 260,
    MLNeuralNetworkLayerlayer_min = 261,
    MLNeuralNetworkLayerlayer_dot = 270,
    MLNeuralNetworkLayerlayer_reduce = 280,
    MLNeuralNetworkLayerlayer_loadConstant = 290,
    MLNeuralNetworkLayerlayer_reshape = 300,
    MLNeuralNetworkLayerlayer_flatten = 301,
    MLNeuralNetworkLayerlayer_permute = 310,
    MLNeuralNetworkLayerlayer_concat = 320,
    MLNeuralNetworkLayerlayer_split = 330,
    MLNeuralNetworkLayerlayer_sequenceRepeat = 340,
    MLNeuralNetworkLayerlayer_simpleRecurrent = 400,
    MLNeuralNetworkLayerlayer_gru = 410,
    MLNeuralNetworkLayerlayer_uniDirectionalLSTM = 420,
    MLNeuralNetworkLayerlayer_biDirectionalLSTM = 430,
    MLNeuralNetworkLayerlayer_NOT_SET = 0,
};

static const char * MLNeuralNetworkLayerlayer_Name(MLNeuralNetworkLayerlayer x) {
    switch (x) {
        case MLNeuralNetworkLayerlayer_convolution:
            return "MLNeuralNetworkLayerlayer_convolution";
        case MLNeuralNetworkLayerlayer_pooling:
            return "MLNeuralNetworkLayerlayer_pooling";
        case MLNeuralNetworkLayerlayer_activation:
            return "MLNeuralNetworkLayerlayer_activation";
        case MLNeuralNetworkLayerlayer_innerProduct:
            return "MLNeuralNetworkLayerlayer_innerProduct";
        case MLNeuralNetworkLayerlayer_embedding:
            return "MLNeuralNetworkLayerlayer_embedding";
        case MLNeuralNetworkLayerlayer_batchnorm:
            return "MLNeuralNetworkLayerlayer_batchnorm";
        case MLNeuralNetworkLayerlayer_mvn:
            return "MLNeuralNetworkLayerlayer_mvn";
        case MLNeuralNetworkLayerlayer_l2normalize:
            return "MLNeuralNetworkLayerlayer_l2normalize";
        case MLNeuralNetworkLayerlayer_softmax:
            return "MLNeuralNetworkLayerlayer_softmax";
        case MLNeuralNetworkLayerlayer_lrn:
            return "MLNeuralNetworkLayerlayer_lrn";
        case MLNeuralNetworkLayerlayer_crop:
            return "MLNeuralNetworkLayerlayer_crop";
        case MLNeuralNetworkLayerlayer_padding:
            return "MLNeuralNetworkLayerlayer_padding";
        case MLNeuralNetworkLayerlayer_upsample:
            return "MLNeuralNetworkLayerlayer_upsample";
        case MLNeuralNetworkLayerlayer_unary:
            return "MLNeuralNetworkLayerlayer_unary";
        case MLNeuralNetworkLayerlayer_add:
            return "MLNeuralNetworkLayerlayer_add";
        case MLNeuralNetworkLayerlayer_multiply:
            return "MLNeuralNetworkLayerlayer_multiply";
        case MLNeuralNetworkLayerlayer_average:
            return "MLNeuralNetworkLayerlayer_average";
        case MLNeuralNetworkLayerlayer_scale:
            return "MLNeuralNetworkLayerlayer_scale";
        case MLNeuralNetworkLayerlayer_bias:
            return "MLNeuralNetworkLayerlayer_bias";
        case MLNeuralNetworkLayerlayer_max:
            return "MLNeuralNetworkLayerlayer_max";
        case MLNeuralNetworkLayerlayer_min:
            return "MLNeuralNetworkLayerlayer_min";
        case MLNeuralNetworkLayerlayer_dot:
            return "MLNeuralNetworkLayerlayer_dot";
        case MLNeuralNetworkLayerlayer_reduce:
            return "MLNeuralNetworkLayerlayer_reduce";
        case MLNeuralNetworkLayerlayer_loadConstant:
            return "MLNeuralNetworkLayerlayer_loadConstant";
        case MLNeuralNetworkLayerlayer_reshape:
            return "MLNeuralNetworkLayerlayer_reshape";
        case MLNeuralNetworkLayerlayer_flatten:
            return "MLNeuralNetworkLayerlayer_flatten";
        case MLNeuralNetworkLayerlayer_permute:
            return "MLNeuralNetworkLayerlayer_permute";
        case MLNeuralNetworkLayerlayer_concat:
            return "MLNeuralNetworkLayerlayer_concat";
        case MLNeuralNetworkLayerlayer_split:
            return "MLNeuralNetworkLayerlayer_split";
        case MLNeuralNetworkLayerlayer_sequenceRepeat:
            return "MLNeuralNetworkLayerlayer_sequenceRepeat";
        case MLNeuralNetworkLayerlayer_simpleRecurrent:
            return "MLNeuralNetworkLayerlayer_simpleRecurrent";
        case MLNeuralNetworkLayerlayer_gru:
            return "MLNeuralNetworkLayerlayer_gru";
        case MLNeuralNetworkLayerlayer_uniDirectionalLSTM:
            return "MLNeuralNetworkLayerlayer_uniDirectionalLSTM";
        case MLNeuralNetworkLayerlayer_biDirectionalLSTM:
            return "MLNeuralNetworkLayerlayer_biDirectionalLSTM";
        case MLNeuralNetworkLayerlayer_NOT_SET:
            return "INVALID";
    }
}

enum MLSamePaddingMode: int {
    MLSamePaddingModeBOTTOM_RIGHT_HEAVY = 0,
    MLSamePaddingModeTOP_LEFT_HEAVY = 1,
};

enum MLConvolutionLayerParamsConvolutionPaddingType: int {
    MLConvolutionLayerParamsConvolutionPaddingType_valid = 50,
    MLConvolutionLayerParamsConvolutionPaddingType_same = 51,
    MLConvolutionLayerParamsConvolutionPaddingType_NOT_SET = 0,
};

static const char * MLConvolutionLayerParamsConvolutionPaddingType_Name(MLConvolutionLayerParamsConvolutionPaddingType x) {
    switch (x) {
        case MLConvolutionLayerParamsConvolutionPaddingType_valid:
            return "MLConvolutionLayerParamsConvolutionPaddingType_valid";
        case MLConvolutionLayerParamsConvolutionPaddingType_same:
            return "MLConvolutionLayerParamsConvolutionPaddingType_same";
        case MLConvolutionLayerParamsConvolutionPaddingType_NOT_SET:
            return "INVALID";
    }
}

enum MLPoolingType: int {
    MLPoolingTypeMAX = 0,
    MLPoolingTypeAVERAGE = 1,
    MLPoolingTypeL2 = 2,
};

enum MLPoolingLayerParamsPoolingPaddingType: int {
    MLPoolingLayerParamsPoolingPaddingType_valid = 30,
    MLPoolingLayerParamsPoolingPaddingType_same = 31,
    MLPoolingLayerParamsPoolingPaddingType_includeLastPixel = 32,
    MLPoolingLayerParamsPoolingPaddingType_NOT_SET = 0,
};

static const char * MLPoolingLayerParamsPoolingPaddingType_Name(MLPoolingLayerParamsPoolingPaddingType x) {
    switch (x) {
        case MLPoolingLayerParamsPoolingPaddingType_valid:
            return "MLPoolingLayerParamsPoolingPaddingType_valid";
        case MLPoolingLayerParamsPoolingPaddingType_same:
            return "MLPoolingLayerParamsPoolingPaddingType_same";
        case MLPoolingLayerParamsPoolingPaddingType_includeLastPixel:
            return "MLPoolingLayerParamsPoolingPaddingType_includeLastPixel";
        case MLPoolingLayerParamsPoolingPaddingType_NOT_SET:
            return "INVALID";
    }
}

enum MLPaddingLayerParamsPaddingType: int {
    MLPaddingLayerParamsPaddingType_constant = 1,
    MLPaddingLayerParamsPaddingType_reflection = 2,
    MLPaddingLayerParamsPaddingType_replication = 3,
    MLPaddingLayerParamsPaddingType_NOT_SET = 0,
};

static const char * MLPaddingLayerParamsPaddingType_Name(MLPaddingLayerParamsPaddingType x) {
    switch (x) {
        case MLPaddingLayerParamsPaddingType_constant:
            return "MLPaddingLayerParamsPaddingType_constant";
        case MLPaddingLayerParamsPaddingType_reflection:
            return "MLPaddingLayerParamsPaddingType_reflection";
        case MLPaddingLayerParamsPaddingType_replication:
            return "MLPaddingLayerParamsPaddingType_replication";
        case MLPaddingLayerParamsPaddingType_NOT_SET:
            return "INVALID";
    }
}

enum MLOperation: int {
    MLOperationSQRT = 0,
    MLOperationRSQRT = 1,
    MLOperationINVERSE = 2,
    MLOperationPOWER = 3,
    MLOperationEXP = 4,
    MLOperationLOG = 5,
    MLOperationABS = 6,
    MLOperationTHRESHOLD = 7,
};

enum MLFlattenOrder: int {
    MLFlattenOrderCHANNEL_FIRST = 0,
    MLFlattenOrderCHANNEL_LAST = 1,
};

enum MLReshapeOrder: int {
    MLReshapeOrderCHANNEL_FIRST = 0,
    MLReshapeOrderCHANNEL_LAST = 1,
};

enum MLReduceOperation: int {
    MLReduceOperationSUM = 0,
    MLReduceOperationAVG = 1,
    MLReduceOperationPROD = 2,
    MLReduceOperationLOGSUM = 3,
    MLReduceOperationSUMSQUARE = 4,
    MLReduceOperationL1 = 5,
    MLReduceOperationL2 = 6,
};

enum MLNeuralNetworkClassifierClassLabels: int {
    MLNeuralNetworkClassifierClassLabels_stringClassLabels = 100,
    MLNeuralNetworkClassifierClassLabels_int64ClassLabels = 101,
    MLNeuralNetworkClassifierClassLabels_NOT_SET = 0,
};

static const char * MLNeuralNetworkClassifierClassLabels_Name(MLNeuralNetworkClassifierClassLabels x) {
    switch (x) {
        case MLNeuralNetworkClassifierClassLabels_stringClassLabels:
            return "MLNeuralNetworkClassifierClassLabels_stringClassLabels";
        case MLNeuralNetworkClassifierClassLabels_int64ClassLabels:
            return "MLNeuralNetworkClassifierClassLabels_int64ClassLabels";
        case MLNeuralNetworkClassifierClassLabels_NOT_SET:
            return "INVALID";
    }
}

#pragma clang diagnostic pop
#endif
