//
//  Utils-inl.hpp
//  CoreML
//
//  Created by Srikrishna Sridhar on 11/14/16.
//  Copyright © 2016 Apple Inc. All rights reserved.
//

#ifndef Utils_inl_h
#define Utils_inl_h

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

#include "MLModelSpecification.hpp"

namespace CoreMLConverter {

    /*
     * Convert all the metadata from the caffe layer.
     *
     * @param[in] caffeLayer Caffe layer
     * @param[out] neuralNetwork Layer in our NN.
     */
    inline void convertCaffeMetadata(const std::string& name,
                                     const std::vector<std::string>& bottom,
                                     const std::vector<std::string>& top,
                                     ::google::protobuf::RepeatedPtrField< ::CoreML::Specification::NeuralNetworkLayer >* nnWrite,
                                     std::map<std::string, std::string>& mappingDataBlobNames) {

        int currentLayerNumber = nnWrite->size() - 1;
        CoreML::Specification::NeuralNetworkLayer* specLayer = nnWrite->Mutable(currentLayerNumber);

        //set name
        specLayer->set_name(name);

        //get inputs and outputs
        std::vector<std::string> inputs;
        std::vector<std::string> outputs;
        for (const auto& input: bottom) {
            if(mappingDataBlobNames.find(input) == mappingDataBlobNames.end()){
                inputs.push_back(input);
            } else {
                inputs.push_back(mappingDataBlobNames[input]);
            }
        }
        for (const auto& output: top) {
            if(mappingDataBlobNames.find(output) == mappingDataBlobNames.end()){
                outputs.push_back(output);
            } else {
                outputs.push_back(mappingDataBlobNames[output]);
            }
        }

        if (outputs.size() == inputs.size() == 1 && outputs[0]==inputs[0]) {
            if (currentLayerNumber == 0) {
                std::stringstream ss;
                ss << "CoreML Specification requires unique input and output names for each layer. First layer of the caffe network cannot have identical input and output names." << std::endl;
                throw std::runtime_error(ss.str());
            } else {
                CoreML::Specification::NeuralNetworkLayer* previousSpecLayer = nnWrite->Mutable(currentLayerNumber-1);
                if (previousSpecLayer->output_size()!=1 || previousSpecLayer->output(0)!=inputs[0]){
                    std::stringstream ss;
                    ss << "Current layer's ('" << name << "') input name ('" << inputs[0] << "') does not match previous layer's ('" ;
                    ss << previousSpecLayer->name() << "') output name ('" << previousSpecLayer->output(0) <<")'" << std::endl;
                    throw std::runtime_error(ss.str());
                } else {
                    inputs[0] = inputs[0] + "_" + std::to_string(currentLayerNumber) + name;
                    previousSpecLayer->clear_output();
                    previousSpecLayer->add_output(inputs[0]);
                }
            }
        }

        // set input names
        for (const auto& input: inputs) {
            specLayer->add_input(input);
        }

        //set output names
        for (const auto& output: outputs) {
            specLayer->add_output(output);
        }

    }

    /*
     * Throw an error when an error in caffe proto is found. (e.g.: Contradictory options set)
     *
     * @param[in] parameterName Name of the caffe parameter that's missing.
     * @param[out] layerName Name of the layer in which error is found.
     */
    inline void errorInCaffeProto(const std::string& errorDescription,
                                      const std::string& layerName,
                                      const std::string& layerType) {

        std::stringstream ss;
        ss << "Caffe model error in layer '" << layerName << "' of type '"<< layerType << "': "<< errorDescription <<". " << std::endl;
        throw std::runtime_error(ss.str());
    }


    /*
     * Throw an error with an unsupported caffe parameter.
     *
     * @param[in] parameterName Name of the caffe parameter not supported.
     * @param[out] layerName Name of the layer in which its not supported.
     */
    inline void unsupportedCaffeParrameter(const std::string& parameterName,
                                    const std::string& layerName,
                                    const std::string& layerType) {

        std::stringstream ss;
        ss << "Unsupported parameter '" << parameterName << "' in caffe layer '"
           << layerName  << "' of type '"<< layerType << "'." << std::endl;
        throw std::runtime_error(ss.str());
    }


    /*
     * Throw an error with an unsupported caffe parameter with option.
     *
     * @param[in] parameterName Name of the caffe parameter not supported.
     * @param[out] layerName Name of the layer in which its not supported.
     */
    inline void unsupportedCaffeParrameterWithOption(const std::string& parameterName,
                                                   const std::string& layerName,
                                                   const std::string& layerType,
                                                   const std::string& optionName) {

        std::stringstream ss;
        ss << "Unsupported option '" << optionName << "' for the parameter '"
           << parameterName << "' in layer '"<< layerName<< "' of type '" << layerType<< "' during caffe conversion."
           << std::endl;
        throw std::runtime_error(ss.str());
    }

    /*
     * Validate whether caffe network layer has name and type.
     *
     * @param[in]  caffeSpec Caffe model format
     */
    inline void validateCaffeLayerTypeAndName(const caffe::LayerParameter& caffeLayer) {

        if (!caffeLayer.has_name()) {
            throw std::runtime_error("Invalid caffe network: Encountered a layer that does not have a name.");
        }

        if (!caffeLayer.has_type()) {
            std::stringstream ss;
            ss << "Invalid caffe network: Layer type missing for layer: '" << caffeLayer.name()  << "'." << std::endl;
            throw std::runtime_error(ss.str());
        }
    }

    /*
     * Get the corresponding layer index in the weights file.
     *
     * @param[in]  caffeSpec Caffe model format
     */
    inline int getLayerIndex(const caffe::LayerParameter& caffeLayer,
                                             const std::map<std::string, int>& mapCaffeLayerNamesToIndex) {


        size_t layerIndexInWeightsFile = 0;
        if (mapCaffeLayerNamesToIndex.find(caffeLayer.name()) != mapCaffeLayerNamesToIndex.end()) {
            int l = mapCaffeLayerNamesToIndex.at(caffeLayer.name());
            assert(l >= 0);
            layerIndexInWeightsFile = static_cast<size_t>(l);
        } else {
            std::stringstream ss;
            ss << "Caffe layer '" << caffeLayer.name()
            << "' is defined in the .prototxt file but is missing from the the .caffemodel file" << std::endl;
            throw std::runtime_error(ss.str());
        }
        assert(layerIndexInWeightsFile <= INT_MAX);
        return static_cast<int>(layerIndexInWeightsFile);

    }

}



#endif /* Utils_inl_h */
