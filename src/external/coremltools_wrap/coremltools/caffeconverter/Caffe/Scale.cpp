//
//  Scale.cpp
//  CoreML
//
//  Created by Srikrishna Sridhar on 11/13/16.
//  Copyright © 2016 Apple Inc. All rights reserved.
//
#include "CaffeConverter.hpp"
#include "Utils-inl.hpp"

#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

using namespace CoreML;

void CoreMLConverter::convertCaffeScale(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
    const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    const caffe::ScaleParameter& caffeLayerParams =  caffeLayer.scale_param();

    #pragma unused(caffeLayerWeights)

    /* CoreML Scale layer does not support all the functionality of Caffe Scale layer.
     Certain error modes can only be detected at compile time when we have input shapes available.
     Such errors cannot be detected at conversion time.
     */
    if (caffeLayer.bottom_size() == 2){
        /*
        WARNING: Caffe Scale layer is currently not fully supported in CoreML.
        Conversion will proceed but certain modes exercised in Caffe may lead to error during compilation/runtime.
        Please refer to the CoreML documentation to see what is supported.
         */
    }

    /*
     Caffe can have scale as either an additional input or as a learned parameter. If former, there will be 2 bottoms,
     otherwise 1.
     if bottom_size == 1 => add an CoreML "scale" layer
     if bottom_size == 2 => add an MLkit "multiply" layer. If bias term is true then add another CoreML "bias" layer as well.
     */

    if (! (caffeLayer.bottom_size() == 1 || caffeLayer.bottom_size() == 2)){
        CoreMLConverter::errorInCaffeProto("Must have 1 or 2 input(s)",caffeLayer.name(),caffeLayer.type());
    }
    if (caffeLayer.top_size() != 1) {
        CoreMLConverter::errorInCaffeProto("Must have 1 output",caffeLayer.name(),caffeLayer.type());
    }

    auto* nnWrite = layerParameters.nnWrite;
    std::vector<std::string> bottom;
    std::vector<std::string> top;
    top.push_back(caffeLayer.top(0));

    int nBottom = caffeLayer.bottom_size();
    bool biasTerm = caffeLayerParams.bias_term();

    int biasLocation = 0;
    if (biasTerm) {
        if (nBottom == 1){
            biasLocation = 1;
        }
    }

    //***************** Some Error Checking in Caffe Proto **********
    if ((nBottom == 1 && !biasTerm) || (nBottom == 2 && biasTerm)){
        if (caffeLayerWeights.blobs_size() != 1){
            CoreMLConverter::errorInCaffeProto("There must be 1 weight blob",caffeLayer.name(),caffeLayer.type());
        }
    }
    if (nBottom == 1 && biasTerm){
        if (caffeLayerWeights.blobs_size() != 2){
            CoreMLConverter::errorInCaffeProto("There must be 2 weight blobs",caffeLayer.name(),caffeLayer.type());
        }
    }
    if (caffeLayerParams.axis() == 0 || caffeLayerParams.axis() == -4 ||
        caffeLayerParams.axis() == 3 || caffeLayerParams.axis() == -1 ) {
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis",caffeLayer.name(),caffeLayer.type(),
                                                             std::to_string(caffeLayerParams.axis()));
    }
    if (caffeLayerParams.axis() == 1 || caffeLayerParams.axis() == -3){
        if (nBottom == 1){
            if (caffeLayerWeights.blobs(0).shape().dim_size() == 2){
                CoreMLConverter::errorInCaffeProto("Scale of size 2D when axis = 1 is currently not supported",
                                                  caffeLayer.name(),caffeLayer.type());
            }
        }
    }
    if (caffeLayerParams.axis() == 1 || caffeLayerParams.axis() == -3){
        if (biasTerm){
            if (caffeLayerWeights.blobs(biasLocation).shape().dim_size() == 2){
                CoreMLConverter::errorInCaffeProto("Bias of size 2D when axis = 1 is currently not supported",
                                                  caffeLayer.name(),caffeLayer.type());
            }
        }
    }
    if (caffeLayerParams.axis() == 2 || caffeLayerParams.axis() == -2){
        if (nBottom == 1){
            if (caffeLayerWeights.blobs(0).shape().dim_size() == 1){
                CoreMLConverter::errorInCaffeProto("Scale of size 1D when axis = 2 is currently not supported",
                                                  caffeLayer.name(),caffeLayer.type());
            }
        }
    }
    if (caffeLayerParams.axis() == 2 || caffeLayerParams.axis() == -2){
        if (biasTerm){
            if (caffeLayerWeights.blobs(biasLocation).shape().dim_size() == 1){
                CoreMLConverter::errorInCaffeProto("Bias of size 1D when axis = 2 is currently not supported",
                                                  caffeLayer.name(),caffeLayer.type());
            }
        }
    }
    //***************************************************************

    /*
     Get params related to the bias
     */
    std::vector<int64_t> biasShape;
    int biasSize = 1;
    if (biasTerm) {

        int dimSize = caffeLayerWeights.blobs(biasLocation).shape().dim_size();

        if (dimSize == 0){
            biasSize = 1;
        } else if (dimSize == 1){
            biasShape.push_back(caffeLayerWeights.blobs(biasLocation).shape().dim(0));
            biasSize *= biasShape.back();
        } else if (dimSize == 2){
            biasShape.push_back(caffeLayerWeights.blobs(biasLocation).shape().dim(0));
            biasSize *= biasShape.back();
            biasShape.push_back(caffeLayerWeights.blobs(biasLocation).shape().dim(1));
            biasSize *= biasShape.back();
        } else if (dimSize == 3 || dimSize == 4){
            biasShape.push_back(caffeLayerWeights.blobs(biasLocation).shape().dim(dimSize-3));
            biasSize *= biasShape.back();
            biasShape.push_back(caffeLayerWeights.blobs(biasLocation).shape().dim(dimSize-2));
            biasSize *= biasShape.back();
            biasShape.push_back(caffeLayerWeights.blobs(biasLocation).shape().dim(dimSize-1));
            biasSize *= biasShape.back();
            if (dimSize == 4 && caffeLayerWeights.blobs(biasLocation).shape().dim(0) != 1) {
                CoreMLConverter::unsupportedCaffeParrameterWithOption("bias",caffeLayer.name(),caffeLayer.type(),
                                                                     "4D bias only supported when 1st dimension is 1");
            }
        } else {
            CoreMLConverter::unsupportedCaffeParrameterWithOption("bias",caffeLayer.name(),caffeLayer.type(),
                                                                 ">4D bias not supported");
        }
        if (caffeLayerWeights.blobs(biasLocation).data_size() != biasSize){
            CoreMLConverter::errorInCaffeProto("Bias blob data inconsistent with the blob dimensions",caffeLayer.name(),caffeLayer.type());
        }
    }

    /*
     Get params related to scale when it is a learned parameter
     */
    std::vector<int64_t> scaleShape;
    int scaleSize = 1;
    if (nBottom == 1) {

        int dimSize = caffeLayerWeights.blobs(0).shape().dim_size();

        if (dimSize == 0){
            scaleSize = 1;
        } else if (dimSize == 1){
            scaleShape.push_back(caffeLayerWeights.blobs(0).shape().dim(0));
            scaleSize *= scaleShape.back();
        } else if (dimSize == 2){
            scaleShape.push_back(caffeLayerWeights.blobs(0).shape().dim(0));
            scaleSize *= scaleShape.back();
            scaleShape.push_back(caffeLayerWeights.blobs(0).shape().dim(1));
            scaleSize *= scaleShape.back();
        } else if (dimSize == 3 || dimSize == 4){
            scaleShape.push_back(caffeLayerWeights.blobs(0).shape().dim(dimSize-3));
            scaleSize *= scaleShape.back();
            scaleShape.push_back(caffeLayerWeights.blobs(0).shape().dim(dimSize-2));
            scaleSize *= scaleShape.back();
            scaleShape.push_back(caffeLayerWeights.blobs(0).shape().dim(dimSize-1));
            scaleSize *= scaleShape.back();
            if (dimSize == 4 && caffeLayerWeights.blobs(0).shape().dim(0) != 1) {
                CoreMLConverter::unsupportedCaffeParrameterWithOption("scale",caffeLayer.name(),caffeLayer.type(),
                                                                     "4D scale only supported when 1st dimension is 1");
            }
        } else {
            CoreMLConverter::unsupportedCaffeParrameterWithOption("scale",caffeLayer.name(),caffeLayer.type(),
                                                                 ">4D scale not supported");
        }
        if (caffeLayerWeights.blobs(0).data_size() != scaleSize){
            CoreMLConverter::errorInCaffeProto("Scale blob data size inconsistent with the  blob dimensions",caffeLayer.name(),caffeLayer.type());
        }
    }

    /*
     Add appropriate CoreML layer(s) now
     */

    if (nBottom == 1){
        Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
        bottom.push_back(caffeLayer.bottom(0));
        CoreMLConverter::convertCaffeMetadata(caffeLayer.name(),
                                             bottom, top,
                                             nnWrite, mappingDataBlobNames);

        Specification::ScaleLayerParams* specLayerParams = specLayer->mutable_scale();
        for (const auto& dim: scaleShape){
            assert(dim >= 0);
            specLayerParams->add_shapescale(static_cast<uint64_t>(dim));
        }
        ::google::protobuf::RepeatedField<float>* scaleWrite = specLayerParams->mutable_scale()->mutable_floatvalue();
        scaleWrite->Resize(scaleSize, 0.0);
        scaleWrite->CopyFrom(caffeLayerWeights.blobs(0).data());
        if (biasTerm) {
            specLayerParams->set_hasbias(true);
            for (const auto& dim: biasShape){
                assert(dim >= 0);
                specLayerParams->add_shapebias(static_cast<uint64_t>(dim));
            }
            ::google::protobuf::RepeatedField<float>* biasWrite = specLayerParams->mutable_bias()->mutable_floatvalue();
            biasWrite->Resize(biasSize, 0.0);
            biasWrite->CopyFrom(caffeLayerWeights.blobs(1).data());
        }
    } else if (biasTerm) {
        Specification::NeuralNetworkLayer* specLayerMul = nnWrite->Add();
        bottom.push_back(caffeLayer.bottom(0));
        bottom.push_back(caffeLayer.bottom(1));
        std::vector<std::string> topMulLayer;
        topMulLayer.push_back(caffeLayer.name() + "_Mul_output");
        CoreMLConverter::convertCaffeMetadata(caffeLayer.name() + "_Mul",
                                             bottom, topMulLayer,
                                             nnWrite, mappingDataBlobNames);

        (void) specLayerMul->mutable_multiply();
        Specification::NeuralNetworkLayer* specLayerBias = nnWrite->Add();
        CoreMLConverter::convertCaffeMetadata(caffeLayer.name() + "_Bias",
                                             topMulLayer, top,
                                             nnWrite, mappingDataBlobNames);

        Specification::BiasLayerParams* specLayerParamsBias = specLayerBias->mutable_bias();
        for (const auto& dim: biasShape){
            assert(dim >= 0);
            specLayerParamsBias->add_shape(static_cast<uint64_t>(dim));
        }
        ::google::protobuf::RepeatedField<float>* biasWrite = specLayerParamsBias->mutable_bias()->mutable_floatvalue();
        biasWrite->Resize(biasSize, 0.0);
        biasWrite->CopyFrom(caffeLayerWeights.blobs(0).data());
    } else {
        Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
        bottom.push_back(caffeLayer.bottom(0));
        bottom.push_back(caffeLayer.bottom(1));
        CoreMLConverter::convertCaffeMetadata(caffeLayer.name() + "_Mul",
                                             bottom, top,
                                             nnWrite, mappingDataBlobNames);
        (void) specLayer->mutable_multiply();
    }
}
