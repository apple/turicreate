//
//  TrainingLayers.cpp
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

void CoreMLConverter::convertCaffeTrainingLayers(CoreMLConverter::ConvertLayerParameters layerParameters) {

    int layerId = *layerParameters.layerId;
    const caffe::LayerParameter& caffeLayer = layerParameters.prototxt.layer(layerId);
    int numberOfLayers = layerParameters.prototxt.layer_size();
    std::map<std::string, std::string>& mappingDataBlobNames = layerParameters.mappingDataBlobNames;


    std::cout<< "WARNING: Skipping training related layer '" << caffeLayer.name() << "' of type '"
    << caffeLayer.type() << "'." << std::endl;

    if (layerId != numberOfLayers-1){
        if (caffeLayer.top_size() != 0 && caffeLayer.bottom_size() != 0) {
            std::string input = caffeLayer.bottom(0);
            for (const auto& output: caffeLayer.top()){
                mappingDataBlobNames[output] = input;
            }
        }
    }
}
