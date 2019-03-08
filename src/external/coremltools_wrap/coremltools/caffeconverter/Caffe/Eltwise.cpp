//
//  Eltwise.cpp
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

void CoreMLConverter::convertCaffeEltwise(CoreMLConverter::ConvertLayerParameters layerParameters) {


    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    //Write Layer metadata
    auto* nnWrite = layerParameters.nnWrite;
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    if (caffeLayer.bottom_size() <= 1 || caffeLayer.top_size() != 1) {
        CoreMLConverter::errorInCaffeProto("Must have more than 1 input and exactly 1 output",caffeLayer.name(),caffeLayer.type());
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

    const caffe::EltwiseParameter& caffeLayerParams = caffeLayer.eltwise_param();

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.coeff_size()!=0) {
        CoreMLConverter::unsupportedCaffeParrameter("coeff", caffeLayer.name(), "Elementwise");
    }
    //***************************************************************

    if (caffeLayerParams.operation()==caffe::EltwiseParameter::SUM){
        (void) specLayer->mutable_add();
    } else if (caffeLayerParams.operation()==caffe::EltwiseParameter::PROD) {
        (void) specLayer->mutable_multiply();
    } else if (caffeLayerParams.operation()==caffe::EltwiseParameter::MAX) {
        (void) specLayer->mutable_max();
    } else {
        CoreMLConverter::errorInCaffeProto("Operation type should be one of 'sum', 'prod' or 'max' ",caffeLayer.name(),caffeLayer.type());
    }
}
