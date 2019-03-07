//
//  Batchnorm.cpp
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

void CoreMLConverter::convertCaffeBatchnorm(CoreMLConverter::ConvertLayerParameters layerParameters) {

    /*
     If Batchnorm layer is followed by a Scale layer, we try to fuse the two into 1 CoreML Batchnorm layer (its more efficient).
     */

    bool fuseScale = false;

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int layerIdWeights = CoreMLConverter::getLayerIndex(caffeLayer,layerParameters.mapCaffeLayerNamesToIndex);
    const caffe::LayerParameter& caffeLayerWeights = layerParameters.protoweights.layer(layerIdWeights);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    if (caffeLayer.bottom_size() != 1 || caffeLayer.top_size() != 1) {
        CoreMLConverter::errorInCaffeProto("Must have 1 input and 1 output",caffeLayer.name(),"BatchNorm");
    }

    std::string TopName = caffeLayer.top(0);
    ::google::protobuf::RepeatedField<float> gamma;
    ::google::protobuf::RepeatedField<float> beta;
    /*
     Check whether Scale layer is to be fused: several conditions must be satisfied
     */
    //check whether the current BN is not the last layer and that the next layer is a "Scale" layer
    if (layerId < layerParameters.prototxt.layer_size()-1 && layerParameters.prototxt.layer(layerId+1).type() == "Scale"){
        const caffe::LayerParameter& caffeScaleLayer = layerParameters.prototxt.layer(layerId+1);

        //check whether the Scale layer has 1 bottom and top. Its bottom must be same as BN layer's top,
        //so that we know for sure that they are connected.
        if (caffeScaleLayer.bottom_size() == 1 && caffeScaleLayer.top_size() == 1 &&
            caffeScaleLayer.bottom(0) == caffeLayer.top(0)){

            const caffe::ScaleParameter& caffeLayerParamsScale = caffeScaleLayer.scale_param();

            //check that the Scale layer only applies scale to the channel axis and that it also includes the bias term.
            if ((caffeLayerParamsScale.axis() == 1 || caffeLayerParamsScale.axis() == -3) &&
                caffeLayerParamsScale.num_axes() == 1 && caffeLayerParamsScale.bias_term()){

                int layerIdWeightsScale = CoreMLConverter::getLayerIndex(caffeScaleLayer,layerParameters.mapCaffeLayerNamesToIndex);
                const caffe::LayerParameter& caffeScaleLayerWeights = layerParameters.protoweights.layer(layerIdWeightsScale);

                //check that the weights proto message corresponding to the scale layer is correct
                if (caffeScaleLayerWeights.blobs_size() == 2 &&
                    caffeScaleLayerWeights.blobs(0).data_size() == caffeScaleLayerWeights.blobs(1).data_size()
                    == caffeLayerWeights.blobs(0).data_size()){

                    //All conditions are satisfied: Scale layer can now be fused with the BN layer.
                    fuseScale = true;
                    TopName = caffeScaleLayer.top(0);
                    gamma = caffeScaleLayerWeights.blobs(0).data();
                    beta = caffeScaleLayerWeights.blobs(1).data();
                    (*layerParameters.layerId)++;
                    //Succesfully fused batchNorm and scale layers.

                }
            }
        }
    }

    #pragma unused(gamma)
    #pragma unused(beta)

    //Write Layer metadata
    ::google::protobuf::RepeatedPtrField< ::CoreML::Specification::NeuralNetworkLayer >* nnWrite = layerParameters.nnWrite;
    Specification::NeuralNetworkLayer* specLayer = nnWrite->Add();
    std::vector<std::string> bottom;
    std::vector<std::string> top;
    bottom.push_back(caffeLayer.bottom(0));
    top.push_back(TopName);
    CoreMLConverter::convertCaffeMetadata(caffeLayer.name(),
                                         bottom, top,
                                         nnWrite, mappingDataBlobNames);


    Specification::BatchnormLayerParams* specLayerParams = specLayer->mutable_batchnorm();
    const caffe::BatchNormParameter& caffeLayerParamsBN = caffeLayer.batch_norm_param();

    //***************** Some Error Checking in Caffe Proto **********
    if (caffeLayerWeights.blobs_size()!=3) {
        CoreMLConverter::errorInCaffeProto("Must have 3 weight blobs for mean, variance and scale",caffeLayer.name(),"BatchNorm");
    }
    if (!caffeLayerParamsBN.use_global_stats()){
        /*
         WARNING: BatchNorm paramater 'use_global_stats' is False. It will be ignored during inference.
        The converter will look for mean/variance weights anyways. If they are not found, it will error out.
        */
    }
    //***************************************************************

    int C = caffeLayerWeights.blobs(0).data_size();
    int varianceLength = caffeLayerWeights.blobs(1).data_size();

    //****Some error checking in caffe file*************************
    if (C == 0){
        CoreMLConverter::errorInCaffeProto("Empty mean vector blob",caffeLayer.name(),"BatchNorm");
    }
    if (varianceLength == 0){
        CoreMLConverter::errorInCaffeProto("Empty variance vector blob",caffeLayer.name(),"BatchNorm");
    }
    if (varianceLength != C){
        CoreMLConverter::errorInCaffeProto("Lengths of mean/variance vectors do not match",caffeLayer.name(),"BatchNorm");
    }
    if (caffeLayerWeights.blobs(2).data_size() == 0){
        CoreMLConverter::errorInCaffeProto("Empty scale factor blob",caffeLayer.name(),"BatchNorm");
    }
    //***************************************************************

    specLayerParams->set_epsilon(caffeLayerParamsBN.eps());
    assert(C >= 0);
    specLayerParams->set_channels(static_cast<uint64_t>(C));
    float scale = caffeLayerWeights.blobs(2).data(0);
    float multiplicativeScale = (scale < 1e-5f) ? 0 : 1/scale;
    ::google::protobuf::RepeatedField<float> mean = caffeLayerWeights.blobs(0).data();
    ::google::protobuf::RepeatedField<float> variance = caffeLayerWeights.blobs(1).data();
    ::google::protobuf::RepeatedField<float>* meanWrite = specLayerParams->mutable_mean()->mutable_floatvalue();
    ::google::protobuf::RepeatedField<float>* varianceWrite = specLayerParams->mutable_variance()->mutable_floatvalue();
    ::google::protobuf::RepeatedField<float>* gammaWrite = specLayerParams->mutable_gamma()->mutable_floatvalue();
    ::google::protobuf::RepeatedField<float>* betaWrite = specLayerParams->mutable_beta()->mutable_floatvalue();
    meanWrite->Resize(C, 0.0);
    varianceWrite->Resize(C, 0.0);
    gammaWrite->Resize(C, 0.0);
    betaWrite->Resize(C, 0.0);
    for (int i=0; i<C; i++){
        meanWrite->Set(i,mean[i] * multiplicativeScale);
        varianceWrite->Set(i,variance[i] * multiplicativeScale);
        if (fuseScale){
            gammaWrite->Set(i, gamma[i]);
            betaWrite->Set(i, beta[i]);
        } else{
            gammaWrite->Set(i,1.0);
            betaWrite->Set(i,0.0);
        }
    }
}
