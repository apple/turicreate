//
//  Power.cpp
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

void CoreMLConverter::convertCaffePower(CoreMLConverter::ConvertLayerParameters layerParameters) {


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

    const caffe::PowerParameter& caffeLayerParams = caffeLayer.power_param();

    //***************** Some Error Checking in Caffe Proto **********
    //nothing
    //***************************************************************

    Specification::UnaryFunctionLayerParams* specLayerParams = specLayer->mutable_unary();
    specLayerParams->set_type(Specification::UnaryFunctionLayerParams::POWER);
    specLayerParams->set_shift(caffeLayerParams.shift());
    specLayerParams->set_scale(caffeLayerParams.scale());
    specLayerParams->set_alpha(caffeLayerParams.power());

}
