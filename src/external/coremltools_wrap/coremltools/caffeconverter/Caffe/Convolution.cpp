//
//  Convolution.cpp
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

void CoreMLConverter::convertCaffeConvolution(CoreMLConverter::ConvertLayerParameters layerParameters) {


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


    caffe::ConvolutionParameter caffeLayerParams = caffeLayer.convolution_param();
    Specification::ConvolutionLayerParams* specLayerParams = specLayer->mutable_convolution();

    // weights shape for coreml and caffe:
    // convolution ==> [outputChannels, kernelChannels, kernelHeight, kernelWidth] == [outputChannelsWeight, kernelChannels, kernelHeight, kernelWidth]
    // deconvoltuion ==> [kernelChannels, outputChannels / nGroups, kernelHeight, kernelWidth] == [kernelChannels, outputChannelsWeight, kernelHeight, kernelWidth]
    uint32_t outputChannels = caffeLayerParams.num_output();
    int64_t outputChannelsWeight = 0;
    int64_t kernelChannels = 0;

    // First check that weights exist.
    if (caffeLayerWeights.blobs_size() == 0){
        CoreMLConverter::errorInCaffeProto("Weight blobs not provided", caffeLayer.name(), caffeLayer.type());
    }

    //check that groups are not 0
    uint32_t numberGroups = caffeLayerParams.group();
    if (numberGroups==0) {
        CoreMLConverter::errorInCaffeProto("group parameter cannot be 0", caffeLayer.name(), caffeLayer.type());
    }

    if (caffeLayerWeights.blobs_size() == 2 || caffeLayerWeights.blobs_size() == 1){

        // Sometimes caffe models do not populate the shape (infering from other parameters)
        if (caffeLayerWeights.blobs(0).shape().dim_size() == 0) {
            outputChannelsWeight = caffeLayerWeights.blobs(0).num();
            kernelChannels = caffeLayerWeights.blobs(0).channels();
        } else {
            if (caffeLayer.type() == "Deconvolution") {
                outputChannelsWeight = caffeLayerWeights.blobs(0).shape().dim(1);
                kernelChannels = caffeLayerWeights.blobs(0).shape().dim(0);
            } else {
                outputChannelsWeight = caffeLayerWeights.blobs(0).shape().dim(0);
                kernelChannels = caffeLayerWeights.blobs(0).shape().dim(1);
            }
        }
    } else {
        CoreMLConverter::errorInCaffeProto("Number of blobs must be 2 or 1 (when there is no bias)", caffeLayer.name(), caffeLayer.type());
    }

    bool hasBias = caffeLayerParams.bias_term();
    int caffeBiasLength = 0;
    if (caffeLayerWeights.blobs_size()>1){
        caffeBiasLength = caffeLayerWeights.blobs(1).data_size();
    }

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayer.type() == "Deconvolution") {
        if (outputChannels!= outputChannelsWeight * numberGroups){
            CoreMLConverter::errorInCaffeProto(
                                               "'num_output' ("+std::to_string(outputChannels)+") divided by groups (" + std::to_string(numberGroups) + "does not match the first dimension of weights ("+std::to_string(outputChannelsWeight)+")"
                                               ,caffeLayer.name(), caffeLayer.type());
        }
    } else {
        if (outputChannels!= outputChannelsWeight){
            CoreMLConverter::errorInCaffeProto(
                    "'num_output' ("+std::to_string(outputChannels)+") does not match the first dimension of weights ("+std::to_string(outputChannelsWeight)+")"
                            ,caffeLayer.name(), caffeLayer.type());
        }
        if (outputChannels % numberGroups != 0){
            CoreMLConverter::errorInCaffeProto("'num_output' ("+std::to_string(outputChannels)+") must be divisible by 'groups' (" + std::to_string(numberGroups) + ")"
                                               ,caffeLayer.name(), caffeLayer.type());
        }
    }
    if ( (hasBias && caffeBiasLength==0) || (!hasBias && caffeBiasLength>0)){
        CoreMLConverter::errorInCaffeProto("'bias_term' flag and blob size for bias incompatible", caffeLayer.name(), caffeLayer.type());
    }
    if (caffeLayerParams.axis() != 1) {
        CoreMLConverter::unsupportedCaffeParrameterWithOption("axis",caffeLayer.name(),
                                                             caffeLayer.type(), std::to_string(caffeLayerParams.axis()));
    }
    if (caffeLayerParams.kernel_size_size()>2){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("Number of kernel size values", caffeLayer.name(),
                                                             caffeLayer.type(), std::to_string(caffeLayerParams.kernel_size_size()));
    }
    if (caffeLayerParams.pad_size()>2){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("Number of pad values", caffeLayer.name(),
                                                             caffeLayer.type(), std::to_string(caffeLayerParams.pad_size()));
    }
    if (caffeLayerParams.stride_size()>2){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("Number of stride values", caffeLayer.name(),
                                                             caffeLayer.type(), std::to_string(caffeLayerParams.stride_size()));
    }

    if (caffeLayerParams.dilation_size()>2){
        CoreMLConverter::unsupportedCaffeParrameterWithOption("Number of dilation size values", caffeLayer.name(),
                                                             caffeLayer.type(), std::to_string(caffeLayerParams.dilation_size()));
    }
    //**************************************************************

    if (caffeLayer.type() == "Deconvolution"){
        specLayerParams->set_isdeconvolution(true);
    } else {
        specLayerParams->set_isdeconvolution(false);
    }
    specLayerParams->set_hasbias(hasBias);
    assert(numberGroups >= 0);
    specLayerParams->set_ngroups(static_cast<uint64_t>(numberGroups));

    //Set Stride
    ::google::protobuf::RepeatedField<uint64_t>* stride = specLayerParams->mutable_stride();
    stride->Resize(2, 0);
    uint32_t heightStride, widthStride;
    heightStride = widthStride = 1;
    if (caffeLayerParams.stride_size() != 0) {
        heightStride = caffeLayerParams.stride(0);
        if (caffeLayerParams.stride_size() == 1){
            widthStride = heightStride;
        } else {
            widthStride = caffeLayerParams.stride(1);
        }
    } else if (caffeLayerParams.stride_h() != 0 && caffeLayerParams.stride_w() != 0) {
        heightStride = caffeLayerParams.stride_h();
        widthStride = caffeLayerParams.stride_w();
    }
    specLayerParams->set_stride(0, static_cast<uint64_t>(heightStride));
    specLayerParams->set_stride(1, static_cast<uint64_t>(widthStride));

    //Set Kernel sizes
    ::google::protobuf::RepeatedField<uint64_t>* kernelSize = specLayerParams->mutable_kernelsize();
    kernelSize->Resize(2, 0);
    uint32_t Kh, Kw;
    Kh = Kw = 3;
    if (caffeLayerParams.kernel_size_size() != 0) {
        Kh = caffeLayerParams.kernel_size(0);
        if (caffeLayerParams.kernel_size_size() == 1){
            Kw = Kh;
        } else {
            Kw = caffeLayerParams.kernel_size(1);
        }
    } else if (caffeLayerParams.kernel_h() != 0 && caffeLayerParams.kernel_w() != 0) {
        Kh = caffeLayerParams.kernel_h();
        Kw = caffeLayerParams.kernel_w();
    } else {
        CoreMLConverter::errorInCaffeProto("Kernel size cannot be 0", caffeLayer.name(), caffeLayer.type());
    }
    specLayerParams->set_kernelsize(0, static_cast<uint64_t>(Kh));
    specLayerParams->set_kernelsize(1, static_cast<uint64_t>(Kw));

    //Set padding params
    uint32_t pad_h = 0;
    uint32_t pad_w = 0;
    if (caffeLayerParams.pad_size() != 0) {
        pad_h = caffeLayerParams.pad(0);
        if (caffeLayerParams.pad_size()==1){
            pad_w = pad_h;
        } else {
            pad_w = caffeLayerParams.pad(1);
        }
    } else {
        pad_h = caffeLayerParams.pad_h();
        pad_w = caffeLayerParams.pad_w();
    }
    if (pad_w==0 && pad_h==0){
        (void) specLayerParams->mutable_valid();
    } else {
        auto heightBorder = specLayerParams->mutable_valid()->mutable_paddingamounts()->add_borderamounts();
        heightBorder->set_startedgesize(static_cast<uint64_t>(pad_h));
        heightBorder->set_endedgesize(static_cast<uint64_t>(pad_h));
        auto widthBorder = specLayerParams->mutable_valid()->mutable_paddingamounts()->add_borderamounts();
        widthBorder->set_startedgesize(static_cast<uint64_t>(pad_w));
        widthBorder->set_endedgesize(static_cast<uint64_t>(pad_w));
    }

    //Set dilation params
    ::google::protobuf::RepeatedField<uint64_t>* dilation = specLayerParams->mutable_dilationfactor();
    dilation->Resize(2, 0);
    uint32_t heightDilation, widthDilation;
    heightDilation = widthDilation = 1;
    if (caffeLayerParams.dilation_size() != 0) {
        heightDilation = caffeLayerParams.dilation(0);
        if (caffeLayerParams.dilation_size() == 1){
            widthDilation = heightDilation;
        } else {
            widthDilation = caffeLayerParams.dilation(1);
        }
    }
    specLayerParams->set_dilationfactor(0, static_cast<uint64_t>(heightDilation));
    specLayerParams->set_dilationfactor(1, static_cast<uint64_t>(widthDilation));

    // Write weights
    assert(outputChannels >= 0);
    specLayerParams->set_outputchannels(static_cast<uint64_t>(outputChannels));
    assert(kernelChannels >= 0);
    specLayerParams->set_kernelchannels(static_cast<uint64_t>(kernelChannels));

    uint64_t blobSize = static_cast<uint64_t>(outputChannelsWeight) * static_cast<uint64_t>(kernelChannels) * static_cast<uint64_t>(Kh) * static_cast<uint64_t>(Kw);

    int caffeBlobSizeWeights = caffeLayerWeights.blobs(0).data_size();
    assert(caffeBlobSizeWeights >= 0);
    if (static_cast<uint64_t>(caffeBlobSizeWeights) != blobSize) {
        CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(blobSize)+" but found blob of size = "+std::to_string     (caffeBlobSizeWeights)+" in caffe", caffeLayer.name(), caffeLayer.type());
    }
    ::google::protobuf::RepeatedField<float>* weights = specLayerParams->mutable_weights()->mutable_floatvalue();
    assert(blobSize <= INT_MAX);
    weights->Resize(static_cast<int32_t>(blobSize), 0.0);
    weights->CopyFrom(caffeLayerWeights.blobs(0).data());

    // Write bias
    if (hasBias) {
        if (caffeBiasLength != static_cast<int32_t>(outputChannels)) {
            CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(outputChannels)+" but found blob of size = "+std::to_string(caffeBiasLength)+" in caffe"
                              , caffeLayer.name(), caffeLayer.type());
        }
        ::google::protobuf::RepeatedField<float>* bias = specLayerParams->mutable_bias()->mutable_floatvalue();
        assert(outputChannels <= INT_MAX);
        bias->Resize(static_cast<int32_t>(outputChannels), 0.0);
        bias->CopyFrom(caffeLayerWeights.blobs(1).data());
    }
}
