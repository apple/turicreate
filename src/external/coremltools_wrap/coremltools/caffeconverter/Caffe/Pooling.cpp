//
//  Pooling.cpp
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

void CoreMLConverter::convertCaffePooling(CoreMLConverter::ConvertLayerParameters layerParameters) {


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

    Specification::PoolingLayerParams* specLayerParams = specLayer->mutable_pooling();
    const caffe::PoolingParameter& caffeLayerParams = caffeLayer.pooling_param();

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.pool()==caffe::PoolingParameter::STOCHASTIC){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("pool method",caffeLayer.name(), "Pooling", "Stochastic");
    }
    //***************************************************************

    // Copy over the parameters.
    if (caffeLayerParams.pool() == caffe::PoolingParameter::MAX) {
        specLayerParams->set_type(Specification::PoolingLayerParams::MAX);
    } else if (caffeLayerParams.pool() == caffe::PoolingParameter::AVE) {
        specLayerParams->set_type(Specification::PoolingLayerParams::AVERAGE);
    }

    if (caffeLayerParams.global_pooling()) {
        specLayerParams->set_globalpooling(true);
    }

    uint32_t pad_h = 0;
    uint32_t pad_w = 0;
    if (caffeLayerParams.has_pad()){
        pad_h = caffeLayerParams.pad();
        pad_w = caffeLayerParams.pad();
    } else {
        pad_h = caffeLayerParams.pad_h();
        pad_w = caffeLayerParams.pad_w();
    }

    specLayerParams->mutable_includelastpixel()->add_paddingamounts(static_cast<uint64_t>(pad_h));
    specLayerParams->mutable_includelastpixel()->add_paddingamounts(static_cast<uint64_t>(pad_w));

    uint32_t stride_h = 0;
    uint32_t stride_w = 0;
    if (caffeLayerParams.has_stride()){
        stride_h = caffeLayerParams.stride();
        stride_w = caffeLayerParams.stride();
    } else {
        stride_h = caffeLayerParams.stride_h();
        stride_w = caffeLayerParams.stride_w();
    }
    if (stride_w == 0) {
        stride_w = 1;
    }
    if (stride_h == 0){
        stride_h = 1;
    }
    specLayerParams->add_stride(static_cast<uint64_t>(stride_h));
    specLayerParams->add_stride(static_cast<uint64_t>(stride_w));

    uint32_t kernel_h = 0;
    uint32_t kernel_w = 0;
    if (caffeLayerParams.has_kernel_size()){
        kernel_h = caffeLayerParams.kernel_size();
        kernel_w = caffeLayerParams.kernel_size();
    } else {
        kernel_h = caffeLayerParams.kernel_h();
        kernel_w = caffeLayerParams.kernel_w();
    }
    if((kernel_h == 0 || (kernel_w == 0)) && !(caffeLayerParams.global_pooling())){
        CoreMLConverter::errorInCaffeProto("Kernel size must be non-zero",caffeLayer.name(),caffeLayer.type());
    }

    specLayerParams->add_kernelsize(static_cast<uint64_t>(kernel_h));
    specLayerParams->add_kernelsize(static_cast<uint64_t>(kernel_w));
}
