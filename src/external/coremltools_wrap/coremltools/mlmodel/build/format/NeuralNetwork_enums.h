#ifndef __NEURALNETWORK_ENUMS_H
#define __NEURALNETWORK_ENUMS_H
enum MLNeuralNetworkMultiArrayShapeMapping: int {
    MLNeuralNetworkMultiArrayShapeMappingRANK5_ARRAY_MAPPING = 0,
    MLNeuralNetworkMultiArrayShapeMappingEXACT_ARRAY_MAPPING = 1,
};

enum MLNeuralNetworkImageShapeMapping: int {
    MLNeuralNetworkImageShapeMappingRANK5_IMAGE_MAPPING = 0,
    MLNeuralNetworkImageShapeMappingRANK4_IMAGE_MAPPING = 1,
};

enum MLScatterMode: int {
    MLScatterModeSCATTER_UPDATE = 0,
    MLScatterModeSCATTER_ADD = 1,
    MLScatterModeSCATTER_SUB = 2,
    MLScatterModeSCATTER_MUL = 3,
    MLScatterModeSCATTER_DIV = 4,
    MLScatterModeSCATTER_MAX = 5,
    MLScatterModeSCATTER_MIN = 6,
};

enum MLNeuralNetworkPreprocessingpreprocessor: int {
    MLNeuralNetworkPreprocessingpreprocessor_scaler = 10,
    MLNeuralNetworkPreprocessingpreprocessor_meanImage = 11,
    MLNeuralNetworkPreprocessingpreprocessor_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLNeuralNetworkPreprocessingpreprocessor_Name(MLNeuralNetworkPreprocessingpreprocessor x) {
    switch (x) {
        case MLNeuralNetworkPreprocessingpreprocessor_scaler:
            return "MLNeuralNetworkPreprocessingpreprocessor_scaler";
        case MLNeuralNetworkPreprocessingpreprocessor_meanImage:
            return "MLNeuralNetworkPreprocessingpreprocessor_meanImage";
        case MLNeuralNetworkPreprocessingpreprocessor_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
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

__attribute__((__unused__))
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
    return "INVALID";
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
    MLNeuralNetworkLayerlayer_resizeBilinear = 211,
    MLNeuralNetworkLayerlayer_cropResize = 212,
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
    MLNeuralNetworkLayerlayer_reorganizeData = 345,
    MLNeuralNetworkLayerlayer_slice = 350,
    MLNeuralNetworkLayerlayer_simpleRecurrent = 400,
    MLNeuralNetworkLayerlayer_gru = 410,
    MLNeuralNetworkLayerlayer_uniDirectionalLSTM = 420,
    MLNeuralNetworkLayerlayer_biDirectionalLSTM = 430,
    MLNeuralNetworkLayerlayer_custom = 500,
    MLNeuralNetworkLayerlayer_copy = 600,
    MLNeuralNetworkLayerlayer_branch = 605,
    MLNeuralNetworkLayerlayer_loop = 615,
    MLNeuralNetworkLayerlayer_loopBreak = 620,
    MLNeuralNetworkLayerlayer_loopContinue = 625,
    MLNeuralNetworkLayerlayer_rangeStatic = 635,
    MLNeuralNetworkLayerlayer_rangeDynamic = 640,
    MLNeuralNetworkLayerlayer_clip = 660,
    MLNeuralNetworkLayerlayer_ceil = 665,
    MLNeuralNetworkLayerlayer_floor = 670,
    MLNeuralNetworkLayerlayer_sign = 680,
    MLNeuralNetworkLayerlayer_round = 685,
    MLNeuralNetworkLayerlayer_exp2 = 700,
    MLNeuralNetworkLayerlayer_sin = 710,
    MLNeuralNetworkLayerlayer_cos = 715,
    MLNeuralNetworkLayerlayer_tan = 720,
    MLNeuralNetworkLayerlayer_asin = 730,
    MLNeuralNetworkLayerlayer_acos = 735,
    MLNeuralNetworkLayerlayer_atan = 740,
    MLNeuralNetworkLayerlayer_sinh = 750,
    MLNeuralNetworkLayerlayer_cosh = 755,
    MLNeuralNetworkLayerlayer_tanh = 760,
    MLNeuralNetworkLayerlayer_asinh = 770,
    MLNeuralNetworkLayerlayer_acosh = 775,
    MLNeuralNetworkLayerlayer_atanh = 780,
    MLNeuralNetworkLayerlayer_erf = 790,
    MLNeuralNetworkLayerlayer_gelu = 795,
    MLNeuralNetworkLayerlayer_equal = 815,
    MLNeuralNetworkLayerlayer_notEqual = 820,
    MLNeuralNetworkLayerlayer_lessThan = 825,
    MLNeuralNetworkLayerlayer_lessEqual = 827,
    MLNeuralNetworkLayerlayer_greaterThan = 830,
    MLNeuralNetworkLayerlayer_greaterEqual = 832,
    MLNeuralNetworkLayerlayer_logicalOr = 840,
    MLNeuralNetworkLayerlayer_logicalXor = 845,
    MLNeuralNetworkLayerlayer_logicalNot = 850,
    MLNeuralNetworkLayerlayer_logicalAnd = 855,
    MLNeuralNetworkLayerlayer_modBroadcastable = 865,
    MLNeuralNetworkLayerlayer_minBroadcastable = 870,
    MLNeuralNetworkLayerlayer_maxBroadcastable = 875,
    MLNeuralNetworkLayerlayer_addBroadcastable = 880,
    MLNeuralNetworkLayerlayer_powBroadcastable = 885,
    MLNeuralNetworkLayerlayer_divideBroadcastable = 890,
    MLNeuralNetworkLayerlayer_floorDivBroadcastable = 895,
    MLNeuralNetworkLayerlayer_multiplyBroadcastable = 900,
    MLNeuralNetworkLayerlayer_subtractBroadcastable = 905,
    MLNeuralNetworkLayerlayer_tile = 920,
    MLNeuralNetworkLayerlayer_stack = 925,
    MLNeuralNetworkLayerlayer_gather = 930,
    MLNeuralNetworkLayerlayer_scatter = 935,
    MLNeuralNetworkLayerlayer_gatherND = 940,
    MLNeuralNetworkLayerlayer_scatterND = 945,
    MLNeuralNetworkLayerlayer_softmaxND = 950,
    MLNeuralNetworkLayerlayer_gatherAlongAxis = 952,
    MLNeuralNetworkLayerlayer_scatterAlongAxis = 954,
    MLNeuralNetworkLayerlayer_reverse = 960,
    MLNeuralNetworkLayerlayer_reverseSeq = 965,
    MLNeuralNetworkLayerlayer_splitND = 975,
    MLNeuralNetworkLayerlayer_concatND = 980,
    MLNeuralNetworkLayerlayer_transpose = 985,
    MLNeuralNetworkLayerlayer_sliceStatic = 995,
    MLNeuralNetworkLayerlayer_sliceDynamic = 1000,
    MLNeuralNetworkLayerlayer_slidingWindows = 1005,
    MLNeuralNetworkLayerlayer_topK = 1015,
    MLNeuralNetworkLayerlayer_argMin = 1020,
    MLNeuralNetworkLayerlayer_argMax = 1025,
    MLNeuralNetworkLayerlayer_embeddingND = 1040,
    MLNeuralNetworkLayerlayer_batchedMatmul = 1045,
    MLNeuralNetworkLayerlayer_getShape = 1065,
    MLNeuralNetworkLayerlayer_loadConstantND = 1070,
    MLNeuralNetworkLayerlayer_fillLike = 1080,
    MLNeuralNetworkLayerlayer_fillStatic = 1085,
    MLNeuralNetworkLayerlayer_fillDynamic = 1090,
    MLNeuralNetworkLayerlayer_broadcastToLike = 1100,
    MLNeuralNetworkLayerlayer_broadcastToStatic = 1105,
    MLNeuralNetworkLayerlayer_broadcastToDynamic = 1110,
    MLNeuralNetworkLayerlayer_squeeze = 1120,
    MLNeuralNetworkLayerlayer_expandDims = 1125,
    MLNeuralNetworkLayerlayer_flattenTo2D = 1130,
    MLNeuralNetworkLayerlayer_reshapeLike = 1135,
    MLNeuralNetworkLayerlayer_reshapeStatic = 1140,
    MLNeuralNetworkLayerlayer_reshapeDynamic = 1145,
    MLNeuralNetworkLayerlayer_rankPreservingReshape = 1150,
    MLNeuralNetworkLayerlayer_constantPad = 1155,
    MLNeuralNetworkLayerlayer_randomNormalLike = 1170,
    MLNeuralNetworkLayerlayer_randomNormalStatic = 1175,
    MLNeuralNetworkLayerlayer_randomNormalDynamic = 1180,
    MLNeuralNetworkLayerlayer_randomUniformLike = 1190,
    MLNeuralNetworkLayerlayer_randomUniformStatic = 1195,
    MLNeuralNetworkLayerlayer_randomUniformDynamic = 1200,
    MLNeuralNetworkLayerlayer_randomBernoulliLike = 1210,
    MLNeuralNetworkLayerlayer_randomBernoulliStatic = 1215,
    MLNeuralNetworkLayerlayer_randomBernoulliDynamic = 1220,
    MLNeuralNetworkLayerlayer_categoricalDistribution = 1230,
    MLNeuralNetworkLayerlayer_reduceL1 = 1250,
    MLNeuralNetworkLayerlayer_reduceL2 = 1255,
    MLNeuralNetworkLayerlayer_reduceMax = 1260,
    MLNeuralNetworkLayerlayer_reduceMin = 1265,
    MLNeuralNetworkLayerlayer_reduceSum = 1270,
    MLNeuralNetworkLayerlayer_reduceProd = 1275,
    MLNeuralNetworkLayerlayer_reduceMean = 1280,
    MLNeuralNetworkLayerlayer_reduceLogSum = 1285,
    MLNeuralNetworkLayerlayer_reduceSumSquare = 1290,
    MLNeuralNetworkLayerlayer_reduceLogSumExp = 1295,
    MLNeuralNetworkLayerlayer_whereNonZero = 1313,
    MLNeuralNetworkLayerlayer_matrixBandPart = 1315,
    MLNeuralNetworkLayerlayer_lowerTriangular = 1320,
    MLNeuralNetworkLayerlayer_upperTriangular = 1325,
    MLNeuralNetworkLayerlayer_whereBroadcastable = 1330,
    MLNeuralNetworkLayerlayer_layerNormalization = 1350,
    MLNeuralNetworkLayerlayer_NonMaximumSuppression = 1400,
    MLNeuralNetworkLayerlayer_oneHot = 1450,
    MLNeuralNetworkLayerlayer_cumSum = 1455,
    MLNeuralNetworkLayerlayer_clampedReLU = 1460,
    MLNeuralNetworkLayerlayer_argSort = 1461,
    MLNeuralNetworkLayerlayer_pooling3d = 1465,
    MLNeuralNetworkLayerlayer_globalPooling3d = 1466,
    MLNeuralNetworkLayerlayer_sliceBySize = 1470,
    MLNeuralNetworkLayerlayer_convolution3d = 1471,
    MLNeuralNetworkLayerlayer_NOT_SET = 0,
};

__attribute__((__unused__))
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
        case MLNeuralNetworkLayerlayer_resizeBilinear:
            return "MLNeuralNetworkLayerlayer_resizeBilinear";
        case MLNeuralNetworkLayerlayer_cropResize:
            return "MLNeuralNetworkLayerlayer_cropResize";
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
        case MLNeuralNetworkLayerlayer_reorganizeData:
            return "MLNeuralNetworkLayerlayer_reorganizeData";
        case MLNeuralNetworkLayerlayer_slice:
            return "MLNeuralNetworkLayerlayer_slice";
        case MLNeuralNetworkLayerlayer_simpleRecurrent:
            return "MLNeuralNetworkLayerlayer_simpleRecurrent";
        case MLNeuralNetworkLayerlayer_gru:
            return "MLNeuralNetworkLayerlayer_gru";
        case MLNeuralNetworkLayerlayer_uniDirectionalLSTM:
            return "MLNeuralNetworkLayerlayer_uniDirectionalLSTM";
        case MLNeuralNetworkLayerlayer_biDirectionalLSTM:
            return "MLNeuralNetworkLayerlayer_biDirectionalLSTM";
        case MLNeuralNetworkLayerlayer_custom:
            return "MLNeuralNetworkLayerlayer_custom";
        case MLNeuralNetworkLayerlayer_copy:
            return "MLNeuralNetworkLayerlayer_copy";
        case MLNeuralNetworkLayerlayer_branch:
            return "MLNeuralNetworkLayerlayer_branch";
        case MLNeuralNetworkLayerlayer_loop:
            return "MLNeuralNetworkLayerlayer_loop";
        case MLNeuralNetworkLayerlayer_loopBreak:
            return "MLNeuralNetworkLayerlayer_loopBreak";
        case MLNeuralNetworkLayerlayer_loopContinue:
            return "MLNeuralNetworkLayerlayer_loopContinue";
        case MLNeuralNetworkLayerlayer_rangeStatic:
            return "MLNeuralNetworkLayerlayer_rangeStatic";
        case MLNeuralNetworkLayerlayer_rangeDynamic:
            return "MLNeuralNetworkLayerlayer_rangeDynamic";
        case MLNeuralNetworkLayerlayer_clip:
            return "MLNeuralNetworkLayerlayer_clip";
        case MLNeuralNetworkLayerlayer_ceil:
            return "MLNeuralNetworkLayerlayer_ceil";
        case MLNeuralNetworkLayerlayer_floor:
            return "MLNeuralNetworkLayerlayer_floor";
        case MLNeuralNetworkLayerlayer_sign:
            return "MLNeuralNetworkLayerlayer_sign";
        case MLNeuralNetworkLayerlayer_round:
            return "MLNeuralNetworkLayerlayer_round";
        case MLNeuralNetworkLayerlayer_exp2:
            return "MLNeuralNetworkLayerlayer_exp2";
        case MLNeuralNetworkLayerlayer_sin:
            return "MLNeuralNetworkLayerlayer_sin";
        case MLNeuralNetworkLayerlayer_cos:
            return "MLNeuralNetworkLayerlayer_cos";
        case MLNeuralNetworkLayerlayer_tan:
            return "MLNeuralNetworkLayerlayer_tan";
        case MLNeuralNetworkLayerlayer_asin:
            return "MLNeuralNetworkLayerlayer_asin";
        case MLNeuralNetworkLayerlayer_acos:
            return "MLNeuralNetworkLayerlayer_acos";
        case MLNeuralNetworkLayerlayer_atan:
            return "MLNeuralNetworkLayerlayer_atan";
        case MLNeuralNetworkLayerlayer_sinh:
            return "MLNeuralNetworkLayerlayer_sinh";
        case MLNeuralNetworkLayerlayer_cosh:
            return "MLNeuralNetworkLayerlayer_cosh";
        case MLNeuralNetworkLayerlayer_tanh:
            return "MLNeuralNetworkLayerlayer_tanh";
        case MLNeuralNetworkLayerlayer_asinh:
            return "MLNeuralNetworkLayerlayer_asinh";
        case MLNeuralNetworkLayerlayer_acosh:
            return "MLNeuralNetworkLayerlayer_acosh";
        case MLNeuralNetworkLayerlayer_atanh:
            return "MLNeuralNetworkLayerlayer_atanh";
        case MLNeuralNetworkLayerlayer_erf:
            return "MLNeuralNetworkLayerlayer_erf";
        case MLNeuralNetworkLayerlayer_gelu:
            return "MLNeuralNetworkLayerlayer_gelu";
        case MLNeuralNetworkLayerlayer_equal:
            return "MLNeuralNetworkLayerlayer_equal";
        case MLNeuralNetworkLayerlayer_notEqual:
            return "MLNeuralNetworkLayerlayer_notEqual";
        case MLNeuralNetworkLayerlayer_lessThan:
            return "MLNeuralNetworkLayerlayer_lessThan";
        case MLNeuralNetworkLayerlayer_lessEqual:
            return "MLNeuralNetworkLayerlayer_lessEqual";
        case MLNeuralNetworkLayerlayer_greaterThan:
            return "MLNeuralNetworkLayerlayer_greaterThan";
        case MLNeuralNetworkLayerlayer_greaterEqual:
            return "MLNeuralNetworkLayerlayer_greaterEqual";
        case MLNeuralNetworkLayerlayer_logicalOr:
            return "MLNeuralNetworkLayerlayer_logicalOr";
        case MLNeuralNetworkLayerlayer_logicalXor:
            return "MLNeuralNetworkLayerlayer_logicalXor";
        case MLNeuralNetworkLayerlayer_logicalNot:
            return "MLNeuralNetworkLayerlayer_logicalNot";
        case MLNeuralNetworkLayerlayer_logicalAnd:
            return "MLNeuralNetworkLayerlayer_logicalAnd";
        case MLNeuralNetworkLayerlayer_modBroadcastable:
            return "MLNeuralNetworkLayerlayer_modBroadcastable";
        case MLNeuralNetworkLayerlayer_minBroadcastable:
            return "MLNeuralNetworkLayerlayer_minBroadcastable";
        case MLNeuralNetworkLayerlayer_maxBroadcastable:
            return "MLNeuralNetworkLayerlayer_maxBroadcastable";
        case MLNeuralNetworkLayerlayer_addBroadcastable:
            return "MLNeuralNetworkLayerlayer_addBroadcastable";
        case MLNeuralNetworkLayerlayer_powBroadcastable:
            return "MLNeuralNetworkLayerlayer_powBroadcastable";
        case MLNeuralNetworkLayerlayer_divideBroadcastable:
            return "MLNeuralNetworkLayerlayer_divideBroadcastable";
        case MLNeuralNetworkLayerlayer_floorDivBroadcastable:
            return "MLNeuralNetworkLayerlayer_floorDivBroadcastable";
        case MLNeuralNetworkLayerlayer_multiplyBroadcastable:
            return "MLNeuralNetworkLayerlayer_multiplyBroadcastable";
        case MLNeuralNetworkLayerlayer_subtractBroadcastable:
            return "MLNeuralNetworkLayerlayer_subtractBroadcastable";
        case MLNeuralNetworkLayerlayer_tile:
            return "MLNeuralNetworkLayerlayer_tile";
        case MLNeuralNetworkLayerlayer_stack:
            return "MLNeuralNetworkLayerlayer_stack";
        case MLNeuralNetworkLayerlayer_gather:
            return "MLNeuralNetworkLayerlayer_gather";
        case MLNeuralNetworkLayerlayer_scatter:
            return "MLNeuralNetworkLayerlayer_scatter";
        case MLNeuralNetworkLayerlayer_gatherND:
            return "MLNeuralNetworkLayerlayer_gatherND";
        case MLNeuralNetworkLayerlayer_scatterND:
            return "MLNeuralNetworkLayerlayer_scatterND";
        case MLNeuralNetworkLayerlayer_softmaxND:
            return "MLNeuralNetworkLayerlayer_softmaxND";
        case MLNeuralNetworkLayerlayer_gatherAlongAxis:
            return "MLNeuralNetworkLayerlayer_gatherAlongAxis";
        case MLNeuralNetworkLayerlayer_scatterAlongAxis:
            return "MLNeuralNetworkLayerlayer_scatterAlongAxis";
        case MLNeuralNetworkLayerlayer_reverse:
            return "MLNeuralNetworkLayerlayer_reverse";
        case MLNeuralNetworkLayerlayer_reverseSeq:
            return "MLNeuralNetworkLayerlayer_reverseSeq";
        case MLNeuralNetworkLayerlayer_splitND:
            return "MLNeuralNetworkLayerlayer_splitND";
        case MLNeuralNetworkLayerlayer_concatND:
            return "MLNeuralNetworkLayerlayer_concatND";
        case MLNeuralNetworkLayerlayer_transpose:
            return "MLNeuralNetworkLayerlayer_transpose";
        case MLNeuralNetworkLayerlayer_sliceStatic:
            return "MLNeuralNetworkLayerlayer_sliceStatic";
        case MLNeuralNetworkLayerlayer_sliceDynamic:
            return "MLNeuralNetworkLayerlayer_sliceDynamic";
        case MLNeuralNetworkLayerlayer_slidingWindows:
            return "MLNeuralNetworkLayerlayer_slidingWindows";
        case MLNeuralNetworkLayerlayer_topK:
            return "MLNeuralNetworkLayerlayer_topK";
        case MLNeuralNetworkLayerlayer_argMin:
            return "MLNeuralNetworkLayerlayer_argMin";
        case MLNeuralNetworkLayerlayer_argMax:
            return "MLNeuralNetworkLayerlayer_argMax";
        case MLNeuralNetworkLayerlayer_embeddingND:
            return "MLNeuralNetworkLayerlayer_embeddingND";
        case MLNeuralNetworkLayerlayer_batchedMatmul:
            return "MLNeuralNetworkLayerlayer_batchedMatmul";
        case MLNeuralNetworkLayerlayer_getShape:
            return "MLNeuralNetworkLayerlayer_getShape";
        case MLNeuralNetworkLayerlayer_loadConstantND:
            return "MLNeuralNetworkLayerlayer_loadConstantND";
        case MLNeuralNetworkLayerlayer_fillLike:
            return "MLNeuralNetworkLayerlayer_fillLike";
        case MLNeuralNetworkLayerlayer_fillStatic:
            return "MLNeuralNetworkLayerlayer_fillStatic";
        case MLNeuralNetworkLayerlayer_fillDynamic:
            return "MLNeuralNetworkLayerlayer_fillDynamic";
        case MLNeuralNetworkLayerlayer_broadcastToLike:
            return "MLNeuralNetworkLayerlayer_broadcastToLike";
        case MLNeuralNetworkLayerlayer_broadcastToStatic:
            return "MLNeuralNetworkLayerlayer_broadcastToStatic";
        case MLNeuralNetworkLayerlayer_broadcastToDynamic:
            return "MLNeuralNetworkLayerlayer_broadcastToDynamic";
        case MLNeuralNetworkLayerlayer_squeeze:
            return "MLNeuralNetworkLayerlayer_squeeze";
        case MLNeuralNetworkLayerlayer_expandDims:
            return "MLNeuralNetworkLayerlayer_expandDims";
        case MLNeuralNetworkLayerlayer_flattenTo2D:
            return "MLNeuralNetworkLayerlayer_flattenTo2D";
        case MLNeuralNetworkLayerlayer_reshapeLike:
            return "MLNeuralNetworkLayerlayer_reshapeLike";
        case MLNeuralNetworkLayerlayer_reshapeStatic:
            return "MLNeuralNetworkLayerlayer_reshapeStatic";
        case MLNeuralNetworkLayerlayer_reshapeDynamic:
            return "MLNeuralNetworkLayerlayer_reshapeDynamic";
        case MLNeuralNetworkLayerlayer_rankPreservingReshape:
            return "MLNeuralNetworkLayerlayer_rankPreservingReshape";
        case MLNeuralNetworkLayerlayer_constantPad:
            return "MLNeuralNetworkLayerlayer_constantPad";
        case MLNeuralNetworkLayerlayer_randomNormalLike:
            return "MLNeuralNetworkLayerlayer_randomNormalLike";
        case MLNeuralNetworkLayerlayer_randomNormalStatic:
            return "MLNeuralNetworkLayerlayer_randomNormalStatic";
        case MLNeuralNetworkLayerlayer_randomNormalDynamic:
            return "MLNeuralNetworkLayerlayer_randomNormalDynamic";
        case MLNeuralNetworkLayerlayer_randomUniformLike:
            return "MLNeuralNetworkLayerlayer_randomUniformLike";
        case MLNeuralNetworkLayerlayer_randomUniformStatic:
            return "MLNeuralNetworkLayerlayer_randomUniformStatic";
        case MLNeuralNetworkLayerlayer_randomUniformDynamic:
            return "MLNeuralNetworkLayerlayer_randomUniformDynamic";
        case MLNeuralNetworkLayerlayer_randomBernoulliLike:
            return "MLNeuralNetworkLayerlayer_randomBernoulliLike";
        case MLNeuralNetworkLayerlayer_randomBernoulliStatic:
            return "MLNeuralNetworkLayerlayer_randomBernoulliStatic";
        case MLNeuralNetworkLayerlayer_randomBernoulliDynamic:
            return "MLNeuralNetworkLayerlayer_randomBernoulliDynamic";
        case MLNeuralNetworkLayerlayer_categoricalDistribution:
            return "MLNeuralNetworkLayerlayer_categoricalDistribution";
        case MLNeuralNetworkLayerlayer_reduceL1:
            return "MLNeuralNetworkLayerlayer_reduceL1";
        case MLNeuralNetworkLayerlayer_reduceL2:
            return "MLNeuralNetworkLayerlayer_reduceL2";
        case MLNeuralNetworkLayerlayer_reduceMax:
            return "MLNeuralNetworkLayerlayer_reduceMax";
        case MLNeuralNetworkLayerlayer_reduceMin:
            return "MLNeuralNetworkLayerlayer_reduceMin";
        case MLNeuralNetworkLayerlayer_reduceSum:
            return "MLNeuralNetworkLayerlayer_reduceSum";
        case MLNeuralNetworkLayerlayer_reduceProd:
            return "MLNeuralNetworkLayerlayer_reduceProd";
        case MLNeuralNetworkLayerlayer_reduceMean:
            return "MLNeuralNetworkLayerlayer_reduceMean";
        case MLNeuralNetworkLayerlayer_reduceLogSum:
            return "MLNeuralNetworkLayerlayer_reduceLogSum";
        case MLNeuralNetworkLayerlayer_reduceSumSquare:
            return "MLNeuralNetworkLayerlayer_reduceSumSquare";
        case MLNeuralNetworkLayerlayer_reduceLogSumExp:
            return "MLNeuralNetworkLayerlayer_reduceLogSumExp";
        case MLNeuralNetworkLayerlayer_whereNonZero:
            return "MLNeuralNetworkLayerlayer_whereNonZero";
        case MLNeuralNetworkLayerlayer_matrixBandPart:
            return "MLNeuralNetworkLayerlayer_matrixBandPart";
        case MLNeuralNetworkLayerlayer_lowerTriangular:
            return "MLNeuralNetworkLayerlayer_lowerTriangular";
        case MLNeuralNetworkLayerlayer_upperTriangular:
            return "MLNeuralNetworkLayerlayer_upperTriangular";
        case MLNeuralNetworkLayerlayer_whereBroadcastable:
            return "MLNeuralNetworkLayerlayer_whereBroadcastable";
        case MLNeuralNetworkLayerlayer_layerNormalization:
            return "MLNeuralNetworkLayerlayer_layerNormalization";
        case MLNeuralNetworkLayerlayer_NonMaximumSuppression:
            return "MLNeuralNetworkLayerlayer_NonMaximumSuppression";
        case MLNeuralNetworkLayerlayer_oneHot:
            return "MLNeuralNetworkLayerlayer_oneHot";
        case MLNeuralNetworkLayerlayer_cumSum:
            return "MLNeuralNetworkLayerlayer_cumSum";
        case MLNeuralNetworkLayerlayer_clampedReLU:
            return "MLNeuralNetworkLayerlayer_clampedReLU";
        case MLNeuralNetworkLayerlayer_argSort:
            return "MLNeuralNetworkLayerlayer_argSort";
        case MLNeuralNetworkLayerlayer_pooling3d:
            return "MLNeuralNetworkLayerlayer_pooling3d";
        case MLNeuralNetworkLayerlayer_globalPooling3d:
            return "MLNeuralNetworkLayerlayer_globalPooling3d";
        case MLNeuralNetworkLayerlayer_sliceBySize:
            return "MLNeuralNetworkLayerlayer_sliceBySize";
        case MLNeuralNetworkLayerlayer_convolution3d:
            return "MLNeuralNetworkLayerlayer_convolution3d";
        case MLNeuralNetworkLayerlayer_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLSamePaddingMode: int {
    MLSamePaddingModeBOTTOM_RIGHT_HEAVY = 0,
    MLSamePaddingModeTOP_LEFT_HEAVY = 1,
};

enum MLMethod: int {
    MLMethodSTRICT_ALIGN_ENDPOINTS_MODE = 0,
    MLMethodALIGN_ENDPOINTS_MODE = 1,
    MLMethodUPSAMPLE_MODE = 2,
    MLMethodROI_ALIGN_MODE = 3,
};

enum MLCoordinates: int {
    MLCoordinatesCORNERS_HEIGHT_FIRST = 0,
    MLCoordinatesCORNERS_WIDTH_FIRST = 1,
    MLCoordinatesCENTER_SIZE_HEIGHT_FIRST = 2,
    MLCoordinatesCENTER_SIZE_WIDTH_FIRST = 3,
};

enum MLQuantizationParamsQuantizationType: int {
    MLQuantizationParamsQuantizationType_linearQuantization = 101,
    MLQuantizationParamsQuantizationType_lookupTableQuantization = 102,
    MLQuantizationParamsQuantizationType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLQuantizationParamsQuantizationType_Name(MLQuantizationParamsQuantizationType x) {
    switch (x) {
        case MLQuantizationParamsQuantizationType_linearQuantization:
            return "MLQuantizationParamsQuantizationType_linearQuantization";
        case MLQuantizationParamsQuantizationType_lookupTableQuantization:
            return "MLQuantizationParamsQuantizationType_lookupTableQuantization";
        case MLQuantizationParamsQuantizationType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLConvolutionLayerParamsConvolutionPaddingType: int {
    MLConvolutionLayerParamsConvolutionPaddingType_valid = 50,
    MLConvolutionLayerParamsConvolutionPaddingType_same = 51,
    MLConvolutionLayerParamsConvolutionPaddingType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLConvolutionLayerParamsConvolutionPaddingType_Name(MLConvolutionLayerParamsConvolutionPaddingType x) {
    switch (x) {
        case MLConvolutionLayerParamsConvolutionPaddingType_valid:
            return "MLConvolutionLayerParamsConvolutionPaddingType_valid";
        case MLConvolutionLayerParamsConvolutionPaddingType_same:
            return "MLConvolutionLayerParamsConvolutionPaddingType_same";
        case MLConvolutionLayerParamsConvolutionPaddingType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLPaddingType: int {
    MLPaddingTypeCUSTOM = 0,
    MLPaddingTypeVALID = 1,
    MLPaddingTypeSAME = 2,
};

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

__attribute__((__unused__))
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
    return "INVALID";
}

enum MLPoolingType3D: int {
    MLPoolingType3DMAX = 0,
    MLPoolingType3DAVERAGE = 1,
};

enum MLPooling3DPaddingType: int {
    MLPooling3DPaddingTypeCUSTOM = 0,
    MLPooling3DPaddingTypeVALID = 1,
    MLPooling3DPaddingTypeSAME = 2,
};

enum MLGlobalPoolingType3D: int {
    MLGlobalPoolingType3DMAX = 0,
    MLGlobalPoolingType3DAVERAGE = 1,
};

enum MLPaddingLayerParamsPaddingType: int {
    MLPaddingLayerParamsPaddingType_constant = 1,
    MLPaddingLayerParamsPaddingType_reflection = 2,
    MLPaddingLayerParamsPaddingType_replication = 3,
    MLPaddingLayerParamsPaddingType_NOT_SET = 0,
};

__attribute__((__unused__))
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
    return "INVALID";
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

enum MLInterpolationMode: int {
    MLInterpolationModeNN = 0,
    MLInterpolationModeBILINEAR = 1,
};

enum MLLinearUpsampleMode: int {
    MLLinearUpsampleModeDEFAULT = 0,
    MLLinearUpsampleModeALIGN_CORNERS_TRUE = 1,
    MLLinearUpsampleModeALIGN_CORNERS_FALSE = 2,
};

enum MLFlattenOrder: int {
    MLFlattenOrderCHANNEL_FIRST = 0,
    MLFlattenOrderCHANNEL_LAST = 1,
};

enum MLReshapeOrder: int {
    MLReshapeOrderCHANNEL_FIRST = 0,
    MLReshapeOrderCHANNEL_LAST = 1,
};

enum MLReorganizationType: int {
    MLReorganizationTypeSPACE_TO_DEPTH = 0,
    MLReorganizationTypeDEPTH_TO_SPACE = 1,
    MLReorganizationTypePIXEL_SHUFFLE = 2,
};

enum MLSliceAxis: int {
    MLSliceAxisCHANNEL_AXIS = 0,
    MLSliceAxisHEIGHT_AXIS = 1,
    MLSliceAxisWIDTH_AXIS = 2,
};

enum MLReduceOperation: int {
    MLReduceOperationSUM = 0,
    MLReduceOperationAVG = 1,
    MLReduceOperationPROD = 2,
    MLReduceOperationLOGSUM = 3,
    MLReduceOperationSUMSQUARE = 4,
    MLReduceOperationL1 = 5,
    MLReduceOperationL2 = 6,
    MLReduceOperationMAX = 7,
    MLReduceOperationMIN = 8,
    MLReduceOperationARGMAX = 9,
};

enum MLReduceAxis: int {
    MLReduceAxisCHW = 0,
    MLReduceAxisHW = 1,
    MLReduceAxisC = 2,
    MLReduceAxisH = 3,
    MLReduceAxisW = 4,
};

enum MLCustomLayerParamValuevalue: int {
    MLCustomLayerParamValuevalue_doubleValue = 10,
    MLCustomLayerParamValuevalue_stringValue = 20,
    MLCustomLayerParamValuevalue_intValue = 30,
    MLCustomLayerParamValuevalue_longValue = 40,
    MLCustomLayerParamValuevalue_boolValue = 50,
    MLCustomLayerParamValuevalue_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLCustomLayerParamValuevalue_Name(MLCustomLayerParamValuevalue x) {
    switch (x) {
        case MLCustomLayerParamValuevalue_doubleValue:
            return "MLCustomLayerParamValuevalue_doubleValue";
        case MLCustomLayerParamValuevalue_stringValue:
            return "MLCustomLayerParamValuevalue_stringValue";
        case MLCustomLayerParamValuevalue_intValue:
            return "MLCustomLayerParamValuevalue_intValue";
        case MLCustomLayerParamValuevalue_longValue:
            return "MLCustomLayerParamValuevalue_longValue";
        case MLCustomLayerParamValuevalue_boolValue:
            return "MLCustomLayerParamValuevalue_boolValue";
        case MLCustomLayerParamValuevalue_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLGeluMode: int {
    MLGeluModeEXACT = 0,
    MLGeluModeTANH_APPROXIMATION = 1,
    MLGeluModeSIGMOID_APPROXIMATION = 2,
};

enum MLNeuralNetworkClassifierClassLabels: int {
    MLNeuralNetworkClassifierClassLabels_stringClassLabels = 100,
    MLNeuralNetworkClassifierClassLabels_int64ClassLabels = 101,
    MLNeuralNetworkClassifierClassLabels_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLNeuralNetworkClassifierClassLabels_Name(MLNeuralNetworkClassifierClassLabels x) {
    switch (x) {
        case MLNeuralNetworkClassifierClassLabels_stringClassLabels:
            return "MLNeuralNetworkClassifierClassLabels_stringClassLabels";
        case MLNeuralNetworkClassifierClassLabels_int64ClassLabels:
            return "MLNeuralNetworkClassifierClassLabels_int64ClassLabels";
        case MLNeuralNetworkClassifierClassLabels_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLLossLayerLossLayerType: int {
    MLLossLayerLossLayerType_categoricalCrossEntropyLossLayer = 10,
    MLLossLayerLossLayerType_meanSquaredErrorLossLayer = 11,
    MLLossLayerLossLayerType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLLossLayerLossLayerType_Name(MLLossLayerLossLayerType x) {
    switch (x) {
        case MLLossLayerLossLayerType_categoricalCrossEntropyLossLayer:
            return "MLLossLayerLossLayerType_categoricalCrossEntropyLossLayer";
        case MLLossLayerLossLayerType_meanSquaredErrorLossLayer:
            return "MLLossLayerLossLayerType_meanSquaredErrorLossLayer";
        case MLLossLayerLossLayerType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLOptimizerOptimizerType: int {
    MLOptimizerOptimizerType_sgdOptimizer = 10,
    MLOptimizerOptimizerType_adamOptimizer = 11,
    MLOptimizerOptimizerType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLOptimizerOptimizerType_Name(MLOptimizerOptimizerType x) {
    switch (x) {
        case MLOptimizerOptimizerType_sgdOptimizer:
            return "MLOptimizerOptimizerType_sgdOptimizer";
        case MLOptimizerOptimizerType_adamOptimizer:
            return "MLOptimizerOptimizerType_adamOptimizer";
        case MLOptimizerOptimizerType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
