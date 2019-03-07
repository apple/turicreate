//
//  Embed.cpp
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

void CoreMLConverter::convertCaffeEmbed(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
    const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
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

    const caffe::EmbedParameter& caffeLayerParams = caffeLayer.embed_param();

    uint32_t inputChannels = caffeLayerParams.input_dim();
    uint32_t outputChannels = caffeLayerParams.num_output();
    bool hasBias = caffeLayerParams.bias_term();
    int caffeBiasLength = 0;
    if (caffeLayerWeights.blobs_size()>1){
        caffeBiasLength = caffeLayerWeights.blobs(1).data_size();
    }

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerWeights.blobs_size() == 0){
        CoreMLConverter::errorInCaffeProto("Weight blobs not provided", caffeLayer.name(), "Embed");
    }
    if (hasBias && caffeBiasLength == 0){
        CoreMLConverter::errorInCaffeProto("Expected bias parameters when 'bias_term' flag is set to True.", caffeLayer.name(), "Embed");
    }
    if (!hasBias && caffeBiasLength > 0) {
        CoreMLConverter::errorInCaffeProto("Found bias parameters even though 'bias_term' flag is False", caffeLayer.name(), "Embed");
    }
    if (caffeLayerWeights.blobs(0).shape().dim_size() != 2){
        CoreMLConverter::errorInCaffeProto("Weight blob dim size is not 2", caffeLayer.name(), "Embed");
    }
    if (caffeLayerWeights.blobs(0).shape().dim(1) != outputChannels){
        CoreMLConverter::errorInCaffeProto("num_output("+std::to_string(outputChannels)+") does not match the second dimension of the weight matrix("+std::to_string(caffeLayerWeights.blobs(0).shape().dim(1))+")"
                                          ,caffeLayer.name(), "Embed");
    }
    if (caffeLayerWeights.blobs(0).shape().dim(0) != inputChannels){
        CoreMLConverter::errorInCaffeProto("input_dim("+std::to_string(inputChannels)+") does not match the first dimension of the weight matrix("+std::to_string(caffeLayerWeights.blobs(0).shape().dim(0))+")"
                                          ,caffeLayer.name(), "Embed");
    }
    //**************************************************************

    Specification::EmbeddingLayerParams* specLayerParams = specLayer->mutable_embedding();
    specLayerParams->set_inputdim(inputChannels);
    specLayerParams->set_outputchannels(outputChannels);
    specLayerParams->set_hasbias(hasBias);

    // Write weights: need to transpose
    // Caffe embed weights are stored as [inputChannels,outputChannels]
    // CoreML needs in the format: [outputChannels,inputChannels]
    uint32_t blobSize = inputChannels*outputChannels;
    int caffeBlobSizeWeights = caffeLayerWeights.blobs(0).data_size();
    if (caffeBlobSizeWeights < 0 ||
        static_cast<uint32_t>(caffeBlobSizeWeights) != blobSize) {
        CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(blobSize)+" but found blob of size = "+std::to_string(caffeBlobSizeWeights)+" in caffe"
                                          , caffeLayer.name(), "Inner Product");
    }
    ::google::protobuf::RepeatedField<float>* weightsWrite = specLayerParams->mutable_weights()->mutable_floatvalue();
    assert(blobSize <= INT_MAX);
    weightsWrite->Resize(static_cast<int>(blobSize), 0.0);
    for (uint32_t r=0; r<outputChannels; r++){
        for (uint32_t c=0; c<inputChannels; c++){
            uint32_t index = r*inputChannels+c;
            assert(index <= INT_MAX);
            uint32_t dataIndex = c*outputChannels+r;
            assert(dataIndex <= INT_MAX);
            weightsWrite->Set(static_cast<int>(index), caffeLayerWeights.blobs(0).data(static_cast<int>(dataIndex)));
        }
    }

    // Write bias
    if (hasBias) {
        if (caffeBiasLength < 0 ||
            static_cast<uint32_t>(caffeBiasLength) != outputChannels) {
            CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(outputChannels)+" but found blob of size = "+std::to_string(caffeBiasLength)+" in caffe"
                                              , caffeLayer.name(), "Inner Product");
        }
        ::google::protobuf::RepeatedField<float>* biasWrite = specLayerParams->mutable_bias()->mutable_floatvalue();
        assert(outputChannels <= INT_MAX);
        biasWrite->Resize(static_cast<int>(outputChannels), 0.0);
        biasWrite->CopyFrom(caffeLayerWeights.blobs(1).data());
    }

}
