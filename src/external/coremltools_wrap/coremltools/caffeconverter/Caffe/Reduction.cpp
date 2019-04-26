//
//  Reduction.cpp
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

void CoreMLConverter::convertCaffeReduction(CoreMLConverter::ConvertLayerParameters layerParameters) {


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

    const caffe::ReductionParameter& caffeLayerParams = caffeLayer.reduction_param();


    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.axis() != 0){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis",caffeLayer.name(),caffeLayer.type(),std::to_string(caffeLayerParams.axis()));
    }
    //***************************************************************

    Specification::ReduceLayerParams* specLayerParams = specLayer->mutable_reduce();
    switch(caffeLayerParams.operation()) {
        case caffe::ReductionParameter::ASUM:
            specLayerParams->set_mode(Specification::ReduceLayerParams::L1);
            break;
        case caffe::ReductionParameter::SUM:
            specLayerParams->set_mode(Specification::ReduceLayerParams::SUM);
            break;
        case caffe::ReductionParameter::SUMSQ:
            specLayerParams->set_mode(Specification::ReduceLayerParams::SUMSQUARE);
            break;
        case caffe::ReductionParameter::MEAN:
            specLayerParams->set_mode(Specification::ReduceLayerParams::AVG);
            break;
        default:
            CoreMLConverter::errorInCaffeProto("operation not set",caffeLayer.name(), caffeLayer.type());
    }
}
