//
//  Slice.cpp
//  CoreML
//
//  Created by aseem wadhwa on 2/5/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include "CaffeConverter.hpp"
#include "Utils-inl.hpp"

#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

using namespace CoreML;

void CoreMLConverter::convertCaffeSlice(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    //Write Layer metadata
    auto* nnWrite = layerParameters.nnWrite;
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    if (caffeLayer.bottom_size() != 1 || caffeLayer.top_size() <= 1) {
        CoreMLConverter::errorInCaffeProto("Must have 1 input and more than 1 output",caffeLayer.name(),caffeLayer.type());
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

    const caffe::SliceParameter& caffeLayerParams = caffeLayer.slice_param();

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.axis()!=1){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis",caffeLayer.name(), "Slice",std::to_string(caffeLayerParams.axis()));
    }
    if (caffeLayerParams.slice_point_size()!=0) {
        CoreMLConverter::unsupportedCaffeParrameter("slice_point", caffeLayer.name(), "Slice");
    }
    //***************************************************************

    Specification::SplitLayerParams* specLayerParams = specLayer->mutable_split();

    int topSize = caffeLayer.top_size();
    assert(topSize >= 0);
    specLayerParams->set_noutputs(static_cast<uint64_t>(topSize));
}
