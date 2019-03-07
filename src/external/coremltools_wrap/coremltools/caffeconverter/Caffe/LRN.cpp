//
//  LRN.cpp
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

void CoreMLConverter::convertCaffeLRN(CoreMLConverter::ConvertLayerParameters layerParameters) {


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

    const caffe::LRNParameter& caffeLayerParams = caffeLayer.lrn_param();


    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.norm_region()==caffe::LRNParameter::WITHIN_CHANNEL){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("norm_region",caffeLayer.name(),"LRN","WITHIN CHANNEL");
    }
    if (caffeLayerParams.k() <= 0){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("k",caffeLayer.name(),"LRN",std::to_string(caffeLayerParams.k()));
    }
    //***************************************************************

    Specification::LRNLayerParams* specLayerParams = specLayer->mutable_lrn();
    specLayerParams->set_alpha(caffeLayerParams.alpha());
    specLayerParams->set_beta(caffeLayerParams.beta());
    specLayerParams->set_localsize(caffeLayerParams.local_size());
    specLayerParams->set_k(caffeLayerParams.k());

}
