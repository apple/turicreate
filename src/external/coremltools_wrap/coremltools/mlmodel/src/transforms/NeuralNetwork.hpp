//
//  NeuralNetwork.hpp
//  mlmodelspec
//
//  Created by Zachary Nation on 4/12/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#ifndef NeuralNetwork_hpp
#define NeuralNetwork_hpp

#include "../Model.hpp"

#include <unordered_set>
#include <vector>

namespace CoreML {
    class NeuralNetwork : public Model {
    public:
        // This should only return the names of NN blobs which are to be outputs. This does not
        // require them to be dangling blobs.
        template<typename T>
        static std::vector<std::string> outputNames(const Specification::Model& spec, const T&);
    };

    template<typename T>
    std::vector<std::string> NeuralNetwork::outputNames(const Specification::Model& spec, const T&) {
        // We won't do correctness checking here, that's for the validator.
        std::unordered_set<std::string> layerOutputs;
        for (const auto& output : spec.description().output()) {
            layerOutputs.insert(output.name());
        }
        return std::vector<std::string>(layerOutputs.begin(), layerOutputs.end());
    }

    // The classifier is a special case. Here, we need to not count as layer names the predicted
    // feature name or predicted probabilities name. Additionally, we need to get the blob
    // corresponding to the layer that will generate the probabilities
    template<>
    inline std::vector<std::string> NeuralNetwork::outputNames<Specification::NeuralNetworkClassifier>(const CoreML::Specification::Model& spec, const Specification::NeuralNetworkClassifier& nnClassifier) {

        // We won't do correctness checking here, that's for the validator
        std::unordered_set<std::string> layerOutputs;
        for (const auto& output : spec.description().output()) {
            if (output.name().compare(spec.description().predictedfeaturename()) != 0
                && output.name().compare(spec.description().predictedprobabilitiesname()) != 0)
                layerOutputs.insert(output.name());
        }

        const std::string& probBlob = nnClassifier.labelprobabilitylayername();
        if (probBlob.compare("") != 0) {
            // then just add this to the set
            layerOutputs.insert(probBlob);
        }
        else {
            // for backwards compatibility with compiling old models (or poorly constructed
            // ones)
            std::unordered_set<std::string> allOutputLayerNames;
            std::unordered_set<std::string> allInputLayerNames;
            for (const auto& layer : nnClassifier.layers()) {
                for (const auto& name : layer.input()) {
                    allInputLayerNames.insert(name);
                }
                for (const auto& name : layer.output()) {
                    allOutputLayerNames.insert(name);
                }
            }
            for (const auto& name : allOutputLayerNames) {
                if (allInputLayerNames.find(name) == allInputLayerNames.end()) {
                    layerOutputs.insert(name);
                }
            }
        }
        return std::vector<std::string>(layerOutputs.begin(), layerOutputs.end());

    }


}



#endif /* NeuralNetwork_hpp */
