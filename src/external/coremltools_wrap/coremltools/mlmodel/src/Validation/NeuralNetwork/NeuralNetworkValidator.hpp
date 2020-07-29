//
//  NeuralNetworkValidator.h
//  mlmodel
//
//  Created by aseem wadhwa on 10/25/18.
//  Copyright © 2018 Apple Inc. All rights reserved.
//

#ifndef NeuralNetworkValidator_h
#define NeuralNetworkValidator_h

#include "../../../build/format/NeuralNetwork_enums.h"
#include "../Validators.hpp"
#include "../ValidatorUtils-inl.hpp"
#include "../../transforms/NeuralNetwork.hpp"
#include "NeuralNetworkShapes.hpp"
#include "../QuantizationValidationUtils.hpp"
#include "UpdatableNeuralNetworkValidator.hpp"

namespace CoreML {

    class NeuralNetworkSpecValidator {

    public:

        NeuralNetworkSpecValidator();
        NeuralNetworkSpecValidator(const std::map<std::string, std::set<std::string>> &blobsInitial,
                                   const std::map<std::string, int> &ioBlobNameToRank,
                                   bool ndArrayMode,
                                   int loopDepth,
                                   const std::map<std::string, int> &blobRanks);

        template<typename T>
        Result validateNeuralNetwork(const T& nn);

        bool ndArrayInterpretation;

        // For Model inputs/outputs, map from name to rank
        std::map<std::string, int> ModelIOBlobNameToRank;

        // For all model blobs, map from name to rank
        std::map<std::string, int> blobNameToRank;

        // Collection of data blob names in the Neural Network.
        // The collection starts with the list of all Model inputs, and grows later.
        // As layers are iterated through, they add their output blobs to this map.
        // Its a map since keys correspond to the blob names
        // and values correspond to the names of the list of layers that produce the blob
        // (a data blob maybe produced by more than one layers if its a copy layer, or the layers
        //  are within if and else branches)
        std::map<std::string, std::set<std::string>> blobs;

        int loopStackDepth;


    private:

        Result validateLayer(const Specification::NeuralNetworkLayer& layer);

        Result validateConvolutionLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateConvolution3DLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateInnerProductLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBatchnormLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateActivation(const Specification::NeuralNetworkLayer& layer);
        Result validatePoolingLayer(const Specification::NeuralNetworkLayer& layer);
        Result validatePooling3dLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateGlobalPooling3dLayer(const Specification::NeuralNetworkLayer& layer);
        Result validatePaddingLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateLRNLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSplitLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateAddLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMultiplyLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateUnaryFunctionLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateUpsampleLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBiasLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateL2NormLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReshapeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateFlattenLayer(const Specification::NeuralNetworkLayer& layer);
        Result validatePermuteLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReduceLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReorganizeDataLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSliceLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateLoadConstantLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateScaleLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSimpleRecurrentLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateGRULayer(const Specification::NeuralNetworkLayer& layer);
        Result validateUniDirectionalLSTMLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBiDirectionalLSTMLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateCropLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateDotLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMvnLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateEmbeddingLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateAverageLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMaxLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMinLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSequenceRepeatLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSoftmaxLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateConcatLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateCustomLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateResizeBilinearLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateCropResizeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBranchLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateTransposeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateCopyLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSoftmaxNDLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReverseLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateConcatNDLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBatchedMatmulLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateTrigonometryLayer(const Specification::NeuralNetworkLayer& layer);
        Result validatePowBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateExp2Layer(const Specification::NeuralNetworkLayer& layer);
        Result validateFillLikeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateFillStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateFillDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateWhereLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateUpperTriangularLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateLowerTriangularLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMatrixBandPartLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBooleanElementwiseLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateLoopLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateLoopContinueBreakLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRankPreservingReshapeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateExpandDimsLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateFlattenTo2DLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReshapeLikeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReshapeStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReshapeDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSqueezeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateActivationLayers(const Specification::NeuralNetworkLayer& layer);
        Result validateBroadcastToLikeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBroadcastToStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateBroadcastToDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateAddBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSubtractBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMultiplyBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateDivideBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMaxBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateMinBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateFloorDivBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateModBroadcastableLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateGatherLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateScatterLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateGatherNDLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateScatterNDLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateGatherAlongAxisLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateScatterAlongAxisLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateWhereNonZeroLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateStackLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSplitNDLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateCeilLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateFloorLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRoundLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSignLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateClipLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSliceStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSliceDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateTileLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRangeStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRangeDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateLoadConstantNDLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateGetShapeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateEmbeddingNDLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSlidingWindowsLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomNormalLikeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomNormalStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomNormalDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomUniformLikeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomUniformStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomUniformDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomBernoulliLikeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomBernoulliStaticLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateRandomBernoulliDynamicLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateTopKLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateArgMaxLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateArgMinLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateCategoricalDistributionLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReductionTypeLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateReverseSeqLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateLayerNormalizationLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateConstantPadLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateNMSLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateOneHotLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateCumSumLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateClampedReluLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateArgsortLayer(const Specification::NeuralNetworkLayer& layer);
        Result validateSliceBySizeLayer(const Specification::NeuralNetworkLayer& layer);

        Result validateFailUnknownType(const Specification::NeuralNetworkLayer& layer);
    };
}


#endif /* NeuralNetworkValidator_h */
