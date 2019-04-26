//
//  Split.cpp
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

void CoreMLConverter::convertCaffeSplit(CoreMLConverter::ConvertLayerParameters layerParameters) {

    /*
     Split: Split layer in caffe is basically a Renaming layer:  needs special operation: add names to the hash
     */

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;

    if (caffeLayer.bottom_size() != 1) {
        CoreMLConverter::errorInCaffeProto("Must have 1 input",caffeLayer.name(),caffeLayer.type());
    }

    std::string input = caffeLayer.bottom(0);
    for (const auto& output: caffeLayer.top()){
        mappingDataBlobNames[output] = input;
    }
}
