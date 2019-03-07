//
//  InnerProduct.cpp
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

void CoreMLConverter::convertCaffeInnnerProduct(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
    const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    //Write Layer metadata
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

    //We add two layers for the inner product layer: flatten + innerproduct
    std::vector<std::string> top_flatten;
    top_flatten.push_back(bottom[0] + "_" + std::to_string(layerId) + "_flattened");
    auto* nnWrite = layerParameters.nnWrite;

    //first write flatten
    Specification::NeuralNetworkLayer* specLayer_flatten = nnWrite->Add();
    CoreMLConverter::convertCaffeMetadata(caffeLayer.name() + "_preflatten",
                                         bottom, top_flatten, nnWrite, mappingDataBlobNames);

    Specification::FlattenLayerParams* flatten_params = specLayer_flatten->mutable_flatten();
    flatten_params->set_mode(Specification::FlattenLayerParams::CHANNEL_FIRST);

    //now write inner product
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    CoreMLConverter::convertCaffeMetadata(caffeLayer.name(),
                                         top_flatten, top, nnWrite, mappingDataBlobNames);

    Specification::InnerProductLayerParams* specLayerParams = specLayer->mutable_innerproduct();
    const caffe::InnerProductParameter& caffeLayerParams = caffeLayer.inner_product_param();
    int64_t inputChannels = 0;
    int64_t outputChannels = 0;

    // First check that weights exist.
    if (caffeLayerWeights.blobs_size() == 0){
        CoreMLConverter::errorInCaffeProto(
            "Weight blobs not provided", caffeLayer.name(), "Innerproduct");
    }

    // Sometimes caffe models do not populate the shape (infering from other parameters)
    if (caffeLayerWeights.blobs(0).shape().dim_size() == 0) {
        outputChannels = caffeLayerWeights.blobs(0).height();
        inputChannels = caffeLayerWeights.blobs(0).width();
    } else {
        outputChannels = caffeLayerWeights.blobs(0).shape().dim(0);
        inputChannels = caffeLayerWeights.blobs(0).shape().dim(1);
    }

    uint64_t numOutput = caffeLayerParams.num_output();
    bool hasBias = caffeLayerParams.bias_term();
    int64_t caffeBiasLength = 0;
    if (caffeLayerWeights.blobs_size()>1){
        caffeBiasLength = caffeLayerWeights.blobs(1).data_size();
    }

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerParams.transpose() == true) {
        CoreMLConverter::unsupportedCaffeParrameter("transpose",caffeLayer.name(), "Inner Product");
    }
    if (caffeLayerParams.axis() != 1) {
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis", caffeLayer.name(), "Inner Product",
                                  std::to_string(caffeLayerParams.axis()));
    }
    if ( (hasBias && caffeBiasLength==0) || (!hasBias && caffeBiasLength>0)){
        CoreMLConverter::errorInCaffeProto("'bias_term' flag and blob size for bias incompatible", caffeLayer.name(), "Inner Product");
    }
    assert(outputChannels >= 0);
    if (static_cast<uint64_t>(outputChannels) != numOutput) {
        CoreMLConverter::errorInCaffeProto("'num_output' ("+std::to_string(numOutput)+") does not match the first dimension of the weight matrix ("+std::to_string(outputChannels)+")"
                                          ,caffeLayer.name(), "Inner Product");
    }
    //**************************************************************

    assert(inputChannels >= 0);
    specLayerParams->set_inputchannels(static_cast<uint64_t>(inputChannels));

    assert(outputChannels >= 0);
    specLayerParams->set_outputchannels(static_cast<uint64_t>(outputChannels));
    specLayerParams->set_hasbias(hasBias);

    // Write weights
    int64_t blobSize = inputChannels*outputChannels;
    int caffeBlobSizeWeights = caffeLayerWeights.blobs(0).data_size();
    if (caffeBlobSizeWeights != blobSize) {
        CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(blobSize)+" but found blob of size = "+std::to_string(caffeBlobSizeWeights)+" in caffe"
                                          , caffeLayer.name(), "Inner Product");
    }
    ::google::protobuf::RepeatedField<float>* weightsWrite = specLayerParams->mutable_weights()->mutable_floatvalue();
    assert(blobSize <= INT_MAX);
    weightsWrite->Resize(static_cast<int32_t>(blobSize), 0.0);
    weightsWrite->CopyFrom(caffeLayerWeights.blobs(0).data());


    // Write bias
    if (hasBias) {
        if (caffeBiasLength != outputChannels) {
            CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(outputChannels)+" but found blob of size = "+std::to_string(caffeBiasLength)+" in caffe"
                                              , caffeLayer.name(), "Inner Product");
        }
        ::google::protobuf::RepeatedField<float>* biasWrite = specLayerParams->mutable_bias()->mutable_floatvalue();
        assert(outputChannels <= INT_MAX);
        biasWrite->Resize(static_cast<int32_t>(outputChannels), 0.0);
        biasWrite->CopyFrom(caffeLayerWeights.blobs(1).data());
    }

}
