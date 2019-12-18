//
//  NeuralNetworkValidatorGraph.h
//  mlmodel
//

#include "../Validators.hpp"

using namespace CoreML;

static bool isLayerSupportedForBackprop(const Specification::NeuralNetworkLayer* layer) {
    switch (layer->layer_case()) {

        case Specification::NeuralNetworkLayer::kConvolution:
        case Specification::NeuralNetworkLayer::kInnerProduct:
        case Specification::NeuralNetworkLayer::kFlatten:
        case Specification::NeuralNetworkLayer::kPooling:
        case Specification::NeuralNetworkLayer::kBatchnorm:
            return true;
        case Specification::NeuralNetworkLayer::kActivation:{
            switch(layer->activation().NonlinearityType_case()) {
                case Specification::ActivationParams::NonlinearityTypeCase::kReLU:
                case Specification::ActivationParams::NonlinearityTypeCase::kSigmoid:
                case Specification::ActivationParams::NonlinearityTypeCase::kTanh:
                    return true;
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

struct LayerNode {
    
    public:
    
    std::vector<LayerNode *> parents; // list of nodes that are parent to this node
    std::vector<LayerNode *> children;
    Specification::NeuralNetworkLayer::LayerCase layerType;
    Specification::LossLayer::LossLayerTypeCase lossLayerType;
    std::string name; // name of this node
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    bool isUpdatable;
    bool isBackPropagable;
    
    LayerNode () {}
    
    LayerNode(const Specification::LossLayer *lossLayer)
    {
        name = lossLayer->name();
        
        switch (lossLayer->LossLayerType_case()) {
            case CoreML::Specification::LossLayer::kCategoricalCrossEntropyLossLayer:
                inputNames.push_back(lossLayer->categoricalcrossentropylosslayer().input());
                break;
            case CoreML::Specification::LossLayer::kMeanSquaredErrorLossLayer:
                inputNames.push_back(lossLayer->meansquarederrorlosslayer().input());
                break;
            default:
                break;
        }

        lossLayerType = lossLayer->LossLayerType_case();
        layerType = Specification::NeuralNetworkLayer::LAYER_NOT_SET;
        isUpdatable = false;
        isBackPropagable = false;
    }
    
    LayerNode(const Specification::NeuralNetworkLayer *layer)
    {
        std::vector<std::string> inNames;
        std::vector<std::string> outNames;
        for (const auto& elem: layer->input()) {
            inNames.push_back(elem);
        }
        for (const auto& elem: layer->output()) {
            outNames.push_back(elem);
        }
        layerType =  layer->layer_case();
        lossLayerType = Specification::LossLayer::LOSSLAYERTYPE_NOT_SET;
        inputNames = inNames;
        outputNames = outNames;
        name = layer->name();
        isUpdatable = layer->isupdatable();
        isBackPropagable = isLayerSupportedForBackprop(layer);
    }
};

struct NeuralNetworkValidatorGraph {
    
    public:
    
    std::map<std::string, LayerNode *> nodeNameToNode;
    std::map<std::string, LayerNode *> blobNameToProducingNode;
    
    NeuralNetworkValidatorGraph() {}
    
    void insertNode(LayerNode *node)
    {
        for (const auto& name: node->inputNames) {
            if (blobNameToProducingNode.find(name) != blobNameToProducingNode.end()) {
                LayerNode *producingNode = blobNameToProducingNode.at(name);
                node->parents.push_back(producingNode);
                producingNode->children.push_back(node);
            }
        }
        for (const auto& name: node->outputNames) {
            blobNameToProducingNode[name] = node;
        }
        nodeNameToNode[node->name] = node;
    }
    
    LayerNode *getNodeFromName(std::string name) const
    {
        if (nodeNameToNode.find(name) == nodeNameToNode.end()) {
            return NULL;
        }
        return nodeNameToNode.at(name);
    }
};



