//
//  RELU.cpp
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

void CoreMLConverter::convertCaffeActivation(CoreMLConverter::ConvertLayerParameters layerParameters) {

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


    Specification::ActivationParams* specLayerParams = specLayer->mutable_activation();

    if (caffeLayer.type() == "ReLU"){
        const caffe::ReLUParameter& caffeLayerParams = caffeLayer.relu_param();
        if (std::abs(caffeLayerParams.negative_slope()) < 1e-6f) {
            (void) specLayerParams->mutable_relu();
        } else {
            auto* leakyRelu = specLayerParams->mutable_leakyrelu();
            leakyRelu->set_alpha(caffeLayerParams.negative_slope());
        }
    } else if (caffeLayer.type() == "TanH"){
        (void) specLayerParams->mutable_tanh();
    } else if (caffeLayer.type() == "Sigmoid"){
        (void) specLayerParams->mutable_sigmoid();
    } else if (caffeLayer.type() == "ELU") {
        const caffe::ELUParameter& caffeLayerParams = caffeLayer.elu_param();
        specLayerParams->mutable_elu()->set_alpha(caffeLayerParams.alpha());
    } else if (caffeLayer.type() == "BNLL") {
        (void) specLayerParams->mutable_softplus();
    } else if (caffeLayer.type() == "PReLU") {
        int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
        const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
        const caffe::PReLUParameter& caffeLayerParams = caffeLayer.prelu_param();
        auto* prelu = specLayerParams->mutable_prelu();
        //***************** Some Error Checking in Caffe Proto **********
        if (caffeLayerWeights.blobs_size() == 0){
            CoreMLConverter::errorInCaffeProto("Parameters (alpha values) not found", caffeLayer.name(), "PReLU");
        }
        if (caffeLayerParams.channel_shared()) {
            if (caffeLayerWeights.blobs(0).data_size() != 1){
                CoreMLConverter::errorInCaffeProto("Expected a scalar parameter (alpha) when 'channel_shared' flag is set", caffeLayer.name(), "PReLU");
            }
        }
        //***************************************************************
        int C = caffeLayerWeights.blobs(0).data_size();
        ::google::protobuf::RepeatedField<float>* alpha = prelu->mutable_alpha()->mutable_floatvalue();
        alpha->Resize(C, 0.0);
        alpha->CopyFrom(caffeLayerWeights.blobs(0).data());
    }

}
