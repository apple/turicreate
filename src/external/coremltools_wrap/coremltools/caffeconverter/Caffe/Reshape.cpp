//
//  Reshape.cpp
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

void CoreMLConverter::convertCaffeReshape(CoreMLConverter::ConvertLayerParameters layerParameters) {


    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    //Write Layer metadata
    auto* nnWrite = layerParameters.nnWrite;
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    if (caffeLayer.bottom_size() != 1 || caffeLayer.top_size() != 1) {
        CoreMLConverter::errorInCaffeProto("Must have 1 input and 1 output",caffeLayer.name(),caffeLayer.type());
    }
    std::vector<std::string> bottom;
    std::vector<std::string> top;
    for (const auto& bottomName: caffeLayer.bottom()){
        bottom.push_back(bottomName);
    }
    for (const auto& topName: caffeLayer.top()){
        top.push_back(topName);
    }
    CoreMLConverter::convertCaffeMetadata(caffeLayer.name(),
                                         bottom, top,
                                         nnWrite, mappingDataBlobNames);

    const caffe::ReshapeParameter& caffeLayerParams = caffeLayer.reshape_param();


    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.axis() != 0){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis",caffeLayer.name(),caffeLayer.type(),std::to_string(caffeLayerParams.axis()));
    }
    if (caffeLayerParams.num_axes() != -1){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("num_axes",caffeLayer.name(),caffeLayer.type(),std::to_string(caffeLayerParams.num_axes()));
    }
    if (caffeLayerParams.shape().dim_size() != 4){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("shape size",caffeLayer.name(),caffeLayer.type(),std::to_string(caffeLayerParams.shape().dim_size()));
    }
    if (caffeLayerParams.shape().dim(0) != 0){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("shape dims[0]",caffeLayer.name(),caffeLayer.type(),std::to_string(caffeLayerParams.shape().dim(0)));
    }
    if (!(caffeLayerParams.shape().dim(1) > 0 && caffeLayerParams.shape().dim(2) > 0 && caffeLayerParams.shape().dim(3) > 0)){
        CoreMLConverter::unsupportedCaffeParrameter("shape dims[0], dims[1], dims[2] must all be positve",caffeLayer.name(),caffeLayer.type());
    }
    //***************************************************************

    Specification::ReshapeLayerParams* specLayerParams = specLayer->mutable_reshape();
    specLayerParams->add_targetshape(caffeLayerParams.shape().dim(1)); //C
    specLayerParams->add_targetshape(caffeLayerParams.shape().dim(2)); //H
    specLayerParams->add_targetshape(caffeLayerParams.shape().dim(3)); //W

}
