//
//  InputLayers.cpp
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

void CoreMLConverter::convertCaffeInputLayers(CoreMLConverter::ConvertLayerParameters layerParameters) {


    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    std::map<std::string, std::vector<int64_t> >& mapBlobNameToDimensions = layerParameters.mapBlobNameToDimensions;
    std::set<std::string>& caffeNetworkInputNames = layerParameters.caffeNetworkInputNames;

    /*
     Mapping from Caffe Input Layer dimensions to CoreML Specification input dimensions:
     1-D (C) ----> (C)
     2-D : (Batch/Seq,C) ----> (C) [last dimension retained]
     >=3-D (...,C,H,W) ----> (C,H,W) [last 3 dimensions retained]
     */

    if (caffeLayer.type() == "Input"){
        if (caffeLayer.input_param().shape_size() == 0) {
            std::stringstream ss;
            ss << "Invalid caffe model: Input layer '" << caffeLayer.name() << "' does not specify the shape parameter." << std::endl;
            throw std::runtime_error(ss.str());
        }
        const ::caffe::BlobShape& shape = caffeLayer.input_param().shape(0);
        std::vector<int64_t> dims;
        if (shape.dim_size() == 0) {
            std::stringstream ss;
            ss << "Invalid caffe model: Input layer '" << caffeLayer.name() << "' does not specify dimensions." << std::endl;
            throw std::runtime_error(ss.str());
        }
        for (const auto& dim: shape.dim()) {
            assert(dim >= 0);
            dims.push_back(dim);
        }
        if (dims.size() == 2) {
            std::cout<<"Ignoring batch/seq size and retaining only the last dimension for conversion. " << std::endl;
            dims.erase(dims.begin(),dims.end()-1);
        }
        if (dims.size() > 3) {
            std::cout<<"Ignoring batch size and retaining only the trailing 3 dimensions for conversion. " << std::endl;
            dims.erase(dims.begin(),dims.end()-3);
        }


        if (caffeLayer.top_size() == 0) {
            CoreMLConverter::errorInCaffeProto("Caffe layer does not have a top blob ",caffeLayer.name(),caffeLayer.type());
        }
        mapBlobNameToDimensions[caffeLayer.top(0)] = dims;
        caffeNetworkInputNames.insert(caffeLayer.top(0));
    } else {
        std::cout<<"WARNING: Skipping Data Layer '"<< caffeLayer.name() << "' of type '" << caffeLayer.type() <<"'. It is recommended to use Input layer for deployment."
                <<std::endl;
    }
}
