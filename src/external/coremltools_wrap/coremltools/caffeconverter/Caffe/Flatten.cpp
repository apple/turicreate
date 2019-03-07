//
//  Flatten.cpp
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

void CoreMLConverter::convertCaffeFlatten(CoreMLConverter::ConvertLayerParameters layerParameters){


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

    const caffe::FlattenParameter& caffeLayerParams = caffeLayer.flatten_param();

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.axis() != 1){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis",caffeLayer.name(), "Flatten",std::to_string(caffeLayerParams.axis()));
    }
    if (caffeLayerParams.end_axis() != -1) {
        if (caffeLayerParams.end_axis() != 3) {
            CoreMLConverter::unsupportedCaffeParrameterWithOption("end_axis", caffeLayer.name(), "Flatten",std::to_string(caffeLayerParams.end_axis()));
        }
    }
    //***************************************************************
    Specification::FlattenLayerParams* flatten_params = specLayer->mutable_flatten();
    flatten_params->set_mode(Specification::FlattenLayerParams::CHANNEL_FIRST);
}
