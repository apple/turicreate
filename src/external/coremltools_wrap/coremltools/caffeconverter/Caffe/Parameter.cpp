//
//  Parameter.cpp
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

void CoreMLConverter::convertCaffeParameter(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
    const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    //Write Layer metadata
    auto* nnWrite = layerParameters.nnWrite;
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    if (caffeLayer.top_size() != 1) {
        CoreMLConverter::errorInCaffeProto("Must have 1 output",caffeLayer.name(),caffeLayer.type());
    }
    std::vector<std::string> bottom;
    std::vector<std::string> top;
    top.push_back(caffeLayer.top(0));
    CoreMLConverter::convertCaffeMetadata(caffeLayer.name(),
                                         bottom, top,
                                         nnWrite, mappingDataBlobNames);

    const caffe::ParameterParameter& caffeLayerParams = caffeLayer.parameter_param();
    //***************** Some Error Checking in Caffe Proto **********
    if (!caffeLayerParams.has_shape()){
        CoreMLConverter::errorInCaffeProto("Must have 'shape' set", caffeLayer.name(), caffeLayer.type());
    }
    if (! (caffeLayerParams.shape().dim_size() == 3 || caffeLayerParams.shape().dim_size() == 4)){
        CoreMLConverter::errorInCaffeProto("'shape' must be either 3 or 4 dimensions",caffeLayer.name(), caffeLayer.type());
    }
    if (caffeLayerParams.shape().dim_size() == 4 && caffeLayerParams.shape().dim(0) != 1){
        CoreMLConverter::errorInCaffeProto("if 'shape' is of 4 dimesnions, first one must be 1",caffeLayer.name(), caffeLayer.type());
    }
    if (caffeLayerWeights.blobs_size() != 1){
        CoreMLConverter::errorInCaffeProto("Must have 1 weight blob",caffeLayer.name(), caffeLayer.type());
    }
    //***************************************************************
    Specification::LoadConstantLayerParams* specLayerParams = specLayer->mutable_loadconstant();
    int dim = caffeLayerParams.shape().dim_size();
    int64_t c = caffeLayerParams.shape().dim(dim-3);
    int64_t h = caffeLayerParams.shape().dim(dim-2);
    int64_t w = caffeLayerParams.shape().dim(dim-1);
    assert(c >= 0);
    specLayerParams->add_shape(static_cast<uint64_t>(c));
    assert(h >= 0);
    specLayerParams->add_shape(static_cast<uint64_t>(h));
    assert(w >= 0);
    specLayerParams->add_shape(static_cast<uint64_t>(w));

    int blobSize = static_cast<int> (c*h*w);
    int caffeBlobSize = caffeLayerWeights.blobs(0).data_size();
    if (caffeBlobSize != blobSize){
        CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(blobSize)+" but found blob of size = "+
                        std::to_string(caffeBlobSize)+" in caffe", caffeLayer.name(), "Inner Product");
    }
    ::google::protobuf::RepeatedField<float>* dataWrite = specLayerParams->mutable_data()->mutable_floatvalue();
    dataWrite->Resize(blobSize, 0.0);
    dataWrite->CopyFrom(caffeLayerWeights.blobs(0).data());
}
