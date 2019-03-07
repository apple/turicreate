//
//  Bias.cpp
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

void CoreMLConverter::convertCaffeBias(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
    const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    const caffe::BiasParameter& caffeLayerParams =  caffeLayer.bias_param();

    #pragma unused(caffeLayerWeights)

    /* CoreML Bias layer does not support all the functionality of Caffe Bias layer.
     Certain error modes can only be detected at compile time when we have input shapes available.
     Such errors cannot be detected at conversion time.
     */
    if (caffeLayer.bottom_size() == 2){
        /*
        WARNING: Caffe Bias layer is currently not fully supported in CoreML.
        Conversion will proceed but certain modes exercised in Caffe may lead to error during compilation/runtime.
        Please refer to the CoreML documentation to see what is supported.
         */
    }

    /*
     Caffe can have bias as either an additional input or as a learned parameter. If former, there will be 2 bottoms,
     otherwise 1.
     if bottom_size == 1 => add an CoreML "bias" layer
     if bottom_size == 2 => add an MLkit "add" layer.
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

    //***************** Some Error Checking in Caffe Proto **********
    if (nBottom == 1){
        if (caffeLayerWeights.blobs_size() != 1){
            CoreMLConverter::errorInCaffeProto("Bias parameters not found.",caffeLayer.name(),caffeLayer.type());
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
                CoreMLConverter::errorInCaffeProto("Bias of size 2D when axis = 1 is currently not supported",
                                                  caffeLayer.name(),caffeLayer.type());
            }
        }
    }
    if (caffeLayerParams.axis() == 2 || caffeLayerParams.axis() == -2){
        if (nBottom == 1){
            if (caffeLayerWeights.blobs(0).shape().dim_size() == 1){
                CoreMLConverter::errorInCaffeProto("Bias of size 1D when axis = 2 is currently not supported",
                                                  caffeLayer.name(),caffeLayer.type());
            }
        }
    }
    //***************************************************************

    /*
     Get params related to bias when it is a learned parameter
     */
    std::vector<int64_t> biasShape;
    int biasSize = 1;
    if (nBottom == 1) {

        int dimSize = caffeLayerWeights.blobs(0).shape().dim_size();

        if (dimSize == 0){
            biasSize = 1;
        } else if (dimSize == 1){
            biasShape.push_back(caffeLayerWeights.blobs(0).shape().dim(0));
            biasSize *= biasShape.back();
        } else if (dimSize == 2){
            biasShape.push_back(caffeLayerWeights.blobs(0).shape().dim(0));
            biasSize *= biasShape.back();
            biasShape.push_back(caffeLayerWeights.blobs(0).shape().dim(1));
            biasSize *= biasShape.back();
        } else if (dimSize == 3 || dimSize == 4){
            biasShape.push_back(caffeLayerWeights.blobs(0).shape().dim(dimSize-3));
            biasSize *= biasShape.back();
            biasShape.push_back(caffeLayerWeights.blobs(0).shape().dim(dimSize-2));
            biasSize *= biasShape.back();
            biasShape.push_back(caffeLayerWeights.blobs(0).shape().dim(dimSize-1));
            biasSize *= biasShape.back();
            if (dimSize == 4 && caffeLayerWeights.blobs(0).shape().dim(0) != 1) {
                CoreMLConverter::unsupportedCaffeParrameterWithOption("bias",caffeLayer.name(),caffeLayer.type(),
                                                                     "4D bias only supported when 1st dimension is 1");
            }
        } else {
            CoreMLConverter::unsupportedCaffeParrameterWithOption("bias",caffeLayer.name(),caffeLayer.type(),
                                                                 ">4D bias not supported");
        }
        if (caffeLayerWeights.blobs(0).data_size() != biasSize){
            CoreMLConverter::errorInCaffeProto("Bias blob's size inconsistent with the blob dimensions",caffeLayer.name(),caffeLayer.type());
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

        Specification::BiasLayerParams* specLayerParams = specLayer->mutable_bias();
        for (const auto& dim: biasShape){
            assert(dim >= 0);
            specLayerParams->add_shape(static_cast<uint64_t>(dim));
        }
        ::google::protobuf::RepeatedField<float>* biasWrite = specLayerParams->mutable_bias()->mutable_floatvalue();
        biasWrite->Resize(biasSize, 0.0);
        biasWrite->CopyFrom(caffeLayerWeights.blobs(0).data());
    } else {
        Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
        bottom.push_back(caffeLayer.bottom(0));
        bottom.push_back(caffeLayer.bottom(1));
        CoreMLConverter::convertCaffeMetadata(caffeLayer.name() + "_add",
                                             bottom, top,
                                             nnWrite, mappingDataBlobNames);
        (void) specLayer->mutable_add();
    }
}
