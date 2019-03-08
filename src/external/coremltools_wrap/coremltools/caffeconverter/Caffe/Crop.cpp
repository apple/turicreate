//
//  Crop.cpp
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

void CoreMLConverter::convertCaffeCrop(CoreMLConverter::ConvertLayerParameters layerParameters) {


    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    //Write Layer metadata
    auto* nnWrite = layerParameters.nnWrite;
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    if (caffeLayer.bottom_size() != 2 || caffeLayer.top_size() != 1) {
        CoreMLConverter::errorInCaffeProto("Must have 2 inputs and 1 output",caffeLayer.name(),caffeLayer.type());
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

    const caffe::CropParameter& caffeLayerParams = caffeLayer.crop_param();


    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.axis() != 2){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis",caffeLayer.name(),caffeLayer.type(),std::to_string(caffeLayerParams.axis()));
    }
    if (!(caffeLayerParams.offset_size() == 1 || caffeLayerParams.offset_size() == 2)){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("offset size",caffeLayer.name(),caffeLayer.type(),std::to_string(caffeLayerParams.offset_size()));
    }
    //***************************************************************

    Specification::CropLayerParams* specLayerParams = specLayer->mutable_crop();
    specLayerParams->add_offset(static_cast<uint64_t>(caffeLayerParams.offset(0)));
    if (caffeLayerParams.offset_size() == 2){
        specLayerParams->add_offset(static_cast<uint64_t>(caffeLayerParams.offset(1)));
    } else {
        specLayerParams->add_offset(static_cast<uint64_t>(caffeLayerParams.offset(0)));
    }

}
