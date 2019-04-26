//
//  LSTM.cpp
//  CoreML
//
//  Created by Bill March on 1/10/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include "CaffeConverter.hpp"
#include "Utils-inl.hpp"

#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

using namespace CoreML;

void CoreMLConverter::convertCaffeLSTM(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
    const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    //Write Layer metadata
    auto* nnWrite = layerParameters.nnWrite;
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    std::vector<std::string> bottom;
    // Caffe LSTM layers take an extra input, a binary indicator to show where one input sequence
    // ends and the next begins in order to reset the hidden state for this occurrence.
    // Since we don't support this, we need to ignore this input
    bottom.push_back(caffeLayer.bottom(0));
    // Now, we need to add the addtional hidden inputs and outputs
    std::string hnamein = "LSTM_" + std::to_string(layerId) + "_h_in";
    bottom.push_back(hnamein);
    std::string cnamein = "LSTM_" + std::to_string(layerId) + "_c_in";
    bottom.push_back(cnamein);

    // Same deal for top names
    std::vector<std::string> top;
    top.push_back(caffeLayer.top(0));
    std::string hnameout = "LSTM_" + std::to_string(layerId) + "_h_out";
    top.push_back(hnameout);
    std::string cnameout = "LSTM_" + std::to_string(layerId) + "_c_out";
    top.push_back(cnameout);

    CoreMLConverter::convertCaffeMetadata(caffeLayer.name(),
                                         bottom, top,
                                         nnWrite, mappingDataBlobNames);

    const caffe::RecurrentParameter& caffeLayerParams = caffeLayer.recurrent_param();
    uint32_t hidden_size = caffeLayerParams.num_output();
    layerParameters.mapBlobNameToDimensions[hnamein] = std::vector<int64_t>(1, hidden_size);
    layerParameters.mapBlobNameToDimensions[cnamein] = std::vector<int64_t>(1, hidden_size);


    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerWeights.blobs_size()==0){
        CoreMLConverter::errorInCaffeProto("Weights not found in the caffemodel file", caffeLayer.name(), "Recurrent");
    }
    if (hidden_size<=0) {
        CoreMLConverter::errorInCaffeProto("'num_output' cannot be non-positive",caffeLayer.name(),"Recurrent");
    }
    if (caffeLayerParams.expose_hidden()) {
        CoreMLConverter::unsupportedCaffeParrameterWithOption("expose_hidden",caffeLayer.name(),"Recurrent","True");
    }
    //**************************************************************

    Specification::UniDirectionalLSTMLayerParams* specLayerParams = specLayer->mutable_unidirectionallstm();

    // Caffe is stored input, bias, Hidden
    int64_t input_size = caffeLayerWeights.blobs(0).shape().dim(1);

    specLayerParams->set_outputvectorsize(static_cast<uint64_t>(hidden_size));
    assert(input_size >= 0);
    specLayerParams->set_inputvectorsize(static_cast<uint64_t>(input_size));

    specLayerParams->mutable_params()->set_sequenceoutput(false);
    // Caffe doesn't support forget bias, so it won't come up
    specLayerParams->mutable_params()->set_forgetbias(false);
    specLayerParams->mutable_params()->set_hasbiasvectors(true);

    //Add default set of non-linearities
    specLayerParams->add_activations()->mutable_sigmoid();
    specLayerParams->add_activations()->mutable_tanh();
    specLayerParams->add_activations()->mutable_tanh();

    //Copy weights
    //Input Weight Matrices
    int blobSize = caffeLayerWeights.blobs(0).data_size();
    int64_t expectedSize = 4*hidden_size*input_size;
    if (blobSize != expectedSize) {
        CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(expectedSize)+" but found blob of size = "+std::to_string(blobSize)+" in caffe"
                          , caffeLayer.name(), "Recurrent");
    }

    ::google::protobuf::RepeatedField<float>* Wi = specLayerParams->mutable_weightparams()->mutable_inputgateweightmatrix()->mutable_floatvalue();
    Wi->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Wi->Set(i,caffeLayerWeights.blobs(0).data(i));
    }
    ::google::protobuf::RepeatedField<float>* Wf = specLayerParams->mutable_weightparams()->mutable_forgetgateweightmatrix()->mutable_floatvalue();
    Wf->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Wf->Set(i,caffeLayerWeights.blobs(0).data(i + static_cast<int>(hidden_size*input_size)));
    }
    ::google::protobuf::RepeatedField<float>* Wo = specLayerParams->mutable_weightparams()->mutable_outputgateweightmatrix()->mutable_floatvalue();
    Wo->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Wo->Set(i,caffeLayerWeights.blobs(0).data(i + 2*static_cast<int>(hidden_size*input_size)));
    }
    ::google::protobuf::RepeatedField<float>* Wz = specLayerParams->mutable_weightparams()->mutable_blockinputweightmatrix()->mutable_floatvalue();
    Wz->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Wz->Set(i,caffeLayerWeights.blobs(0).data(i + 3*static_cast<int>(hidden_size*input_size)));
    }

    //biases
    blobSize = caffeLayerWeights.blobs(1).data_size();
    expectedSize = 4*hidden_size;
    if (blobSize != expectedSize) {
        CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(expectedSize)+" but found blob of size = "+std::to_string(blobSize)+" in caffe"
                          , caffeLayer.name(), "Recurrent");
    }
    ::google::protobuf::RepeatedField<float>* bi = specLayerParams->mutable_weightparams()->mutable_inputgatebiasvector()->mutable_floatvalue();
    bi->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        bi->Set(i,caffeLayerWeights.blobs(1).data(i));
    }
    ::google::protobuf::RepeatedField<float>* bf = specLayerParams->mutable_weightparams()->mutable_forgetgatebiasvector()->mutable_floatvalue();
    bf->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        bf->Set(i,caffeLayerWeights.blobs(1).data(i + static_cast<int>(hidden_size)));
    }
    ::google::protobuf::RepeatedField<float>* bo = specLayerParams->mutable_weightparams()->mutable_outputgatebiasvector()->mutable_floatvalue();
    bo->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        bo->Set(i,caffeLayerWeights.blobs(1).data(i + 2*static_cast<int>(hidden_size)));
    }
    ::google::protobuf::RepeatedField<float>* bz = specLayerParams->mutable_weightparams()->mutable_blockinputbiasvector()->mutable_floatvalue();
    bz->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        bz->Set(i,caffeLayerWeights.blobs(1).data(i + 3*static_cast<int>(hidden_size)));
    }


    //Recursion matrices
    blobSize = caffeLayerWeights.blobs(2).data_size();
    expectedSize = 4*hidden_size*hidden_size;
    if (blobSize != expectedSize) {
        CoreMLConverter::errorInCaffeProto("Expected blob size = "+std::to_string(expectedSize)+" but found blob of size = "+std::to_string(blobSize)+" in caffe"
                          , caffeLayer.name(), "Recurrent");
    }
    ::google::protobuf::RepeatedField<float>* Ri = specLayerParams->mutable_weightparams()->mutable_inputgaterecursionmatrix()->mutable_floatvalue();
    Ri->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Ri->Set(i,caffeLayerWeights.blobs(2).data(i));
    }
    ::google::protobuf::RepeatedField<float>* Rf = specLayerParams->mutable_weightparams()->mutable_forgetgaterecursionmatrix()->mutable_floatvalue();
    Rf->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Rf->Set(i,caffeLayerWeights.blobs(2).data(i + static_cast<int>(hidden_size*hidden_size)));
    }
    ::google::protobuf::RepeatedField<float>* Ro = specLayerParams->mutable_weightparams()->mutable_outputgaterecursionmatrix()->mutable_floatvalue();
    Ro->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Ro->Set(i,caffeLayerWeights.blobs(2).data(i + 2*static_cast<int>(hidden_size*hidden_size)));
    }
    ::google::protobuf::RepeatedField<float>* Rz = specLayerParams->mutable_weightparams()->mutable_blockinputrecursionmatrix()->mutable_floatvalue();
    Rz->Resize(blobSize/4, 0.0);
    for (int i=0; i<blobSize/4; i++){
        Rz->Set(i,caffeLayerWeights.blobs(2).data(i + 3*static_cast<int>(hidden_size*hidden_size)));
    }
}
