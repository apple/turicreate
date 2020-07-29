//
//  UpdatableNeuralNetworkValidator.cpp
//  CoreML_framework
//


#include "UpdatableNeuralNetworkValidator.hpp"
#include "../ParameterValidator.hpp"


using namespace CoreML;

/**
 * This method validates loss layer input and output.
 * The loss layer's input must be generated from a softmax layer's output.
 * The loss layer's target must not be generated from within the graph.
 */
static Result validateLossLayer(const Specification::LossLayer *lossLayer, const NeuralNetworkValidatorGraph *graph) {
    Result r;
    std::string err;
    
    switch (lossLayer->LossLayerType_case()) {
        case CoreML::Specification::LossLayer::kCategoricalCrossEntropyLossLayer:
        {
            std::string lossInputName = lossLayer->categoricalcrossentropylosslayer().input();

            // validate loss input.
            std::string lossLayerName = lossLayer->name();
            const auto *lossNode = graph->getNodeFromName(lossLayerName);
            if (lossNode == NULL) {
                err = "Failed to look up node for '" + lossLayerName + "'.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            bool lossInputValidated = false;
            const auto &parents = lossNode->parents;
            for (const auto *node : parents) {
                if (node->layerType == Specification::NeuralNetworkLayer::kSoftmax) {
                    if (node->outputNames[0] != lossInputName) {
                        err = "For the categorical cross entropy loss layer named '" + lossLayer->name() + "', input is not generated from a softmax output.";
                        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
                    }
                    lossInputValidated = true;
                    break;
                }
            }

            if (!lossInputValidated) {
                err = "For the categorical cross entropy loss layer named '" + lossLayer->name() + "', input is not generated from a softmax output.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }

            // validate loss target
            std::string targetName = lossLayer->categoricalcrossentropylosslayer().target();
            if (graph->blobNameToProducingNode.find(targetName) != graph->blobNameToProducingNode.end()) {
                err = "For the cross entropy loss layer named '" + lossLayer->name() + "', target is generated within the graph.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            break;
        }
        case CoreML::Specification::LossLayer::kMeanSquaredErrorLossLayer:
        {
            std::string inputName = lossLayer->meansquarederrorlosslayer().input();
            if (graph->blobNameToProducingNode.find(inputName) == graph->blobNameToProducingNode.end()) {
                err = "For the MSE loss layer named '" + lossLayer->name() + "', input is not generated within the graph.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            std::string targetName = lossLayer->meansquarederrorlosslayer().target();
            if (graph->blobNameToProducingNode.find(targetName) != graph->blobNameToProducingNode.end()) {
                err = "For the MSE loss layer named '" + lossLayer->name() + "', target is generated within the graph.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            break;
        }
        default:
            err = "Loss function is not recognized in the loss layer named '" + lossLayer->name() + "', only cross entropy loss and MSE are supported.";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }
    
    return r;
}

template<typename T> Result validateTrainingInputs(const Specification::ModelDescription& modelDescription, const T& nn) {
    Result r;
    std::string err;

    if (modelDescription.traininginput_size() <= 1) {
        err = "Must provide training inputs for updatable neural network (expecting both input and target for loss function).";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }

    std::vector<int> trainingInputExclusiveIndices;
    for (int i = 0; i < modelDescription.traininginput_size(); i++) {
        const Specification::FeatureDescription& trainingInput = modelDescription.traininginput(i);
        bool trainingInputIsPredictionInput = false;
        for (int j = 0; j < modelDescription.input_size(); j++) {
            const Specification::FeatureDescription& input = modelDescription.input(j);
            if (Specification::isEquivalent(trainingInput, input)) {
                trainingInputIsPredictionInput = true;
                break;
            }
        }
        if (!trainingInputIsPredictionInput) {
            trainingInputExclusiveIndices.push_back(i);
        }
    }

    // Check that training inputs are specified to at least contain the target (which we'll validate is the target further down)
    if (trainingInputExclusiveIndices.size() < 1) {
        err = "Training inputs don't describe required inputs for the loss (needs both the input and the target).";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }

    // Ensure other inputs (excluding the target) are present
    // This should prevent issues where the only training input described is the target itself
    // Given we don't yet know what inputs are explicitly required for training we can't vet beyond this for what model inputs to require
    size_t numberOfNonExclusiveTrainingInputs = static_cast<size_t>(modelDescription.traininginput_size()) - trainingInputExclusiveIndices.size();
    if (numberOfNonExclusiveTrainingInputs <= 0) { // Given at least one input from the inference model's inputs must be supplied this should be positive
        err = "The training inputs must include at least one input from the model itself as required for training (should have at least one input in common with those used for prediction).";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }

    std::string target;
    const Specification::NetworkUpdateParameters& updateParams = nn.updateparams();
    if (updateParams.losslayers(0).has_categoricalcrossentropylosslayer()) {
        target = updateParams.losslayers(0).categoricalcrossentropylosslayer().target();
    } else if (updateParams.losslayers(0).has_meansquarederrorlosslayer()) {
        target = updateParams.losslayers(0).meansquarederrorlosslayer().target();
    }

    bool isClassifier = (dynamic_cast<const Specification::NeuralNetworkClassifier*>(&nn) != nullptr);

    bool trainingInputMeetsRequirement = false;
    for (size_t i = 0; i < trainingInputExclusiveIndices.size(); i++) {
        const Specification::FeatureDescription& trainingInputDescription = modelDescription.traininginput(trainingInputExclusiveIndices[i]);
        std::string trainingInputTarget = trainingInputDescription.name();

        // If the neural network is a classifier, check if the predictedFeatureNames is a training input (and ensure matching types)
        if (isClassifier) {
            if (trainingInputTarget == modelDescription.predictedfeaturename()) {

                // Find the predictedFeatureName's output and use to check types
                for (const auto& output : modelDescription.output()) {
                    if (trainingInputTarget == output.name()) {
                        if (trainingInputDescription.type() == output.type()) {
                            trainingInputMeetsRequirement = true;
                            break;
                        } else {
                            std::string typeString = output.type().has_int64type() ? "Int64" : "String";
                            std::string targetTypeString = trainingInputDescription.type().has_int64type() ? "Int64" : "String";
                            err = "The type of the training input provided: " + trainingInputTarget + " doesn't match the expected type of the classifier. Found: " + targetTypeString + ", expected: " + typeString + ".";
                            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
                        }
                    }
                }
            }
        }

        // If NN was not a classifier (or predictedFeatureName was not in the training inputs), ensure the target is in the training inputs
        if (target == trainingInputTarget) {
            trainingInputMeetsRequirement = true;
        }
    }

    // Raise an error if the target isn't found (or if the target or predictedFeatureNames aren't found for classifiers)
    // Users can supply either / or for a classifier, but if neither is found we'll request the predictedFeatureNames
    if (!trainingInputMeetsRequirement) {
        if (isClassifier) {
            err = "The training inputs don't include the target of the classifier: " + modelDescription.predictedfeaturename();
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
        }
        err = "The training inputs don't include the loss layer's target: " + target;
        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }

    return Result();
}

template Result validateTrainingInputs<CoreML::Specification::NeuralNetwork>(Specification::ModelDescription const&, CoreML::Specification::NeuralNetwork const&);
template Result validateTrainingInputs<CoreML::Specification::NeuralNetworkRegressor>(Specification::ModelDescription const&, CoreML::Specification::NeuralNetworkRegressor const&);
template Result validateTrainingInputs<CoreML::Specification::NeuralNetworkClassifier>(Specification::ModelDescription const&, CoreML::Specification::NeuralNetworkClassifier const&);


static Result validateOptimizer(const Specification::Optimizer& optimizer) {
    Result r;
    std::string err;
    
    switch (optimizer.OptimizerType_case()) {
        case Specification::Optimizer::kSgdOptimizer:
        {
            const Specification::SGDOptimizer &sgdOptimizer = optimizer.sgdoptimizer();
            
            if (false == sgdOptimizer.has_learningrate()) {
                err = "SGD optimizer should include learningRate parameter.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            r = validateDoubleParameter("learningRate", sgdOptimizer.learningrate());
            if (!r.good()) {return r;}
            
            if (false == sgdOptimizer.has_minibatchsize()) {
                err = "SGD optimizer should include miniBatchSize parameter.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            r = validateInt64Parameter("miniBatchSize", sgdOptimizer.minibatchsize(), true);
            if (!r.good()) {return r;}
            
            break;
        }
        case Specification::Optimizer::kAdamOptimizer:
        {
            const Specification::AdamOptimizer &adamOptimizer = optimizer.adamoptimizer();
            
            if (false == adamOptimizer.has_learningrate()) {
                err = "ADAM optimizer should include learningRate parameter.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            r = validateDoubleParameter("learningRate", adamOptimizer.learningrate());
            if (!r.good()) {return r;}
            
            if (false == adamOptimizer.has_minibatchsize()) {
                err = "ADAM optimizer should include miniBatchSize parameter.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            r = validateInt64Parameter("miniBatchSize", adamOptimizer.minibatchsize(), true);
            if (!r.good()) {return r;}
            
            if (false == adamOptimizer.has_beta1()) {
                err = "ADAM optimizer should include beta1 parameter.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            r = validateDoubleParameter("beta1", adamOptimizer.beta1());
            if (!r.good()) {return r;}
            
            if (false == adamOptimizer.has_beta2()) {
                err = "ADAM optimizer should include beta2 parameter.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            r = validateDoubleParameter("beta2", adamOptimizer.beta2());
            if (!r.good()) {return r;}
            
            if (false == adamOptimizer.has_eps()) {
                err = "ADAM optimizer should include eps (epslion) parameter.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            r = validateDoubleParameter("eps", adamOptimizer.eps());
            if (!r.good()) {return r;}
    
            break;
        }
        default:
            err = "Optimizer is not recognized.";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }
    
    return r;
}

static Result validateOtherTopLevelUpdateParameters(const Specification::NetworkUpdateParameters& updateParameters) {
    Result r;
    std::string err;
    
    if (false == updateParameters.has_epochs()) {
        err = "Epochs should be included in neural network update parameters.";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }
    
    r = validateInt64Parameter("epochs", updateParameters.epochs(), true);
    if (!r.good()) {return r;}

    if (updateParameters.has_seed()) {
        r = validateInt64Parameter("seed", updateParameters.seed(), false);
        if (!r.good()) {return r;}
    }

    return r;
}

template<typename T> static Result isTrainingConfigurationSupported(const T& nn) {
    Result r;
    
    if (nn.updateparams().losslayers_size() > 1) {
        std::string err;
        err = "This model has more than one loss layers specified, which is not supported at the moment.";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
    }
    
    NeuralNetworkValidatorGraph graph;
    std::vector<LayerNode> layerNodes;  // Holds on to all the nodes until the validation is over.
    
    // Make sure the layerNodes is large enough to hold all the elements
    size_t numberOfLayers = (size_t)nn.layers_size() + (size_t)nn.updateparams().losslayers_size();
    layerNodes.resize(numberOfLayers);
    
    /* first traverse "nn" and build the graph object
     */
    for (int i = 0; i < nn.layers_size(); i++) {
        const Specification::NeuralNetworkLayer* layer = &nn.layers(i);
        layerNodes.push_back(LayerNode(layer));
        LayerNode *nodePtr = &(layerNodes[layerNodes.size() - 1]);
        graph.insertNode(nodePtr);
    }
    for (int i = 0; i < nn.updateparams().losslayers_size(); i++) {
        const Specification::LossLayer* lossLayer = &nn.updateparams().losslayers(i);
        layerNodes.push_back(LayerNode(lossLayer));
        LayerNode *nodePtr = &(layerNodes[layerNodes.size() - 1]);
        graph.insertNode(nodePtr);
        r = validateLossLayer(lossLayer, &graph);
        if (!r.good()) {return r;}
    }
    
    r = validateOptimizer(nn.updateparams().optimizer());
    if (!r.good()) {return r;}

    r = validateOtherTopLevelUpdateParameters(nn.updateparams());
    if (!r.good()) {return r;}
    
    /* Now we check the following, by doing a BFS starting from the loss layers:
     - All the layers on the route from the loss layers to the updatable layers must support back-propagation
     */
    
    std::set<std::string> visitedLayersSet;
    
    for (int i = 0; i < nn.updateparams().losslayers_size(); i++) {
        const Specification::LossLayer& lossLayer = nn.updateparams().losslayers(i);
        std::string lossLayerName = lossLayer.name();
        
        // queue for BFS
        std::queue<std::string> BFSQueue;
        BFSQueue.push(lossLayerName);
        
        bool nonBackPropagableLayerSeen = false;
        
        std::string firstNonBackpropogabaleLayerSeen;
        
        while(!BFSQueue.empty()) {
            std::string currentNodeName = BFSQueue.front();
            BFSQueue.pop();
            const auto *currentNode = graph.getNodeFromName(currentNodeName);
            if (currentNode == NULL) {
                std::string err = "Failed to look up node for '" + currentNodeName + "'.";
                return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
            }
            
            // we are traversing the graph in reverse
            for (const auto *parentNode: currentNode->parents) {
                std::string parentName = parentNode->name;
                if (visitedLayersSet.find(parentName) != visitedLayersSet.end()) {
                    continue;
                }
                visitedLayersSet.insert(parentName);
                BFSQueue.push(parentName);
                
                // check if this "parentNode" is marked as updatable
                if (parentNode->isUpdatable) {
                    if (nonBackPropagableLayerSeen) {
                        // if this is true, it means an updatable node exists beyond a non-backpropagable layer
                        std::string err;
                        err = "There is a layer (" + firstNonBackpropogabaleLayerSeen + "), which does not support backpropagation, between an updatable marked layer and the loss function.";
                        return Result(ResultType::INVALID_UPDATABLE_MODEL_CONFIGURATION, err);
                    }
                }
                
                // check if this "parentNode" is a non-backpropagable layer
                if (!parentNode->isBackPropagable) {
                    // softmax is a nonBackPropagableLayer. However, It is valid config if it is attached to a kCategoricalCrossEntropyLossLayer layer.
                    if ((parentNode->layerType == Specification::NeuralNetworkLayer::kSoftmax) && (currentNode->lossLayerType == Specification::LossLayer::kCategoricalCrossEntropyLossLayer)) {
                        continue;
                    }
                    nonBackPropagableLayerSeen = true;
                    firstNonBackpropogabaleLayerSeen = parentNode->name;
                }
            }
        }
    }
    
    return r;
    
}

static Result validateWeightParamsUpdatable(const Specification::NeuralNetworkLayer& layer) {
    Result r;
    
    bool weight_update_flag = false;
    bool bias_update_flag = false;
    bool has_bias = false;
    bool weights_are_quantized = false;
    bool bias_is_quantized = false;
    
    std::string err;
    
    switch (layer.layer_case()) {
        case Specification::NeuralNetworkLayer::kConvolution:
            has_bias = layer.convolution().hasbias();
            if (has_bias) {
                bias_update_flag = layer.convolution().bias().isupdatable();
                bias_is_quantized = layer.convolution().bias().has_quantization();
            }
            weights_are_quantized = layer.convolution().weights().has_quantization();
            weight_update_flag = layer.convolution().weights().isupdatable();
            break;
        case Specification::NeuralNetworkLayer::kInnerProduct:
            has_bias = layer.innerproduct().hasbias();
            if (has_bias) {
                bias_update_flag = layer.innerproduct().bias().isupdatable();
                bias_is_quantized = layer.innerproduct().bias().has_quantization();
            }
            weights_are_quantized = layer.innerproduct().weights().has_quantization();
            weight_update_flag = layer.innerproduct().weights().isupdatable();
            break;
        default:
            return r;
    }

    if (weights_are_quantized || bias_is_quantized) {
        err = "An updatable layer, named '" + layer.name() + "', has quantized weights/bias param. Quantized weights/bias not supported for update.";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_PARAMETERS, err);
    }
    
    if (!weight_update_flag || ((has_bias) && (!bias_update_flag))) {
        err = "An updatable layer, named '" + layer.name() + "', has a weight/bias param which is not marked as updatable.";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_PARAMETERS, err);
    }
    
    return r;
}

template<typename T> static Result validateLayerAndLossLayerNamesCollisions(const T& nn) {
    Result r;
    std::set<std::string> setOfNames;
    std::string err;
    
    for (int i = 0; i < nn.layers_size(); i++) {
        std::string layerName = nn.layers(i).name();
        if (setOfNames.count(layerName)) {
            err = "The updatable model has a name collision for: '" + layerName + "', i.e., there are more than one layers or loss layers with this name.";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_PARAMETERS, err);
        }
        setOfNames.insert(layerName);
    }
    
    const Specification::NetworkUpdateParameters& updateParams = nn.updateparams();
    for (int j = 0; j < updateParams.losslayers_size(); j++) {
        std::string lossLayerName = updateParams.losslayers(j).name();
        if (setOfNames.count(lossLayerName)) {
            err = "The updatable model has a name collision for: '" + lossLayerName + "', i.e., there are more than one layers or loss layers with this name.";
            return Result(ResultType::INVALID_UPDATABLE_MODEL_PARAMETERS, err);
        }
        setOfNames.insert(lossLayerName);
    }
    
    return r;
}

/**
 * This method validates an updatble model against:
 * At least one layer must be updatable.
 * Checks only Convolution and/or InnerProduct layers are marked as updatable.
 * Checks weights of the updatable layers are marked as updatable.
 * Checks if the biases (if any) on the updatable layers are marked as updatable.
 */
template<typename T> static Result validateUpdatableLayerSupport(const T& nn) {
    
    Result r;
    bool isAtleastOneLayerUpdatable = false;

    for (int i = 0; i < nn.layers_size(); i++) {
        const Specification::NeuralNetworkLayer& layer = nn.layers(i);
        bool isUpdatable = layer.isupdatable();
        if (isUpdatable) {
            isAtleastOneLayerUpdatable = true;
            switch (layer.layer_case()) {
                case Specification::NeuralNetworkLayer::kConvolution:
                case Specification::NeuralNetworkLayer::kInnerProduct:
                    r = validateWeightParamsUpdatable(layer);
                    if (!r.good()) {return r;}
                    break;
                default:
                    std::string err;
                    err = "The layer named '" + layer.name() + "' is marked as updatable, however, it is not supported as the type of this layer is neither convolution nor inner-product.";
                    return Result(ResultType::INVALID_UPDATABLE_MODEL_PARAMETERS, err);
            }
        }
    }
    
    if (!isAtleastOneLayerUpdatable) {
        std::string err;
        err = "The model is marked as updatable, but none of the layers are updatable.";
        return Result(ResultType::INVALID_UPDATABLE_MODEL_PARAMETERS, err);
    }
    
    return r;
}

/*
 Top level function for validating whether a Neural network, marked as updatable
 is valid or not, which includes the check whether it is supported or not.
 */

template<typename T> Result validateUpdatableNeuralNetwork(const T& nn) {
    
    Result r;

    r = validateUpdatableLayerSupport(nn);
    if (!r.good()) {return r;}
    
    r = validateLayerAndLossLayerNamesCollisions(nn);
    if (!r.good()) {return r;}

    r = isTrainingConfigurationSupported(nn);
    if (!r.good()) {return r;}
    
    return r;
}

template Result validateUpdatableNeuralNetwork<CoreML::Specification::NeuralNetwork>(CoreML::Specification::NeuralNetwork const&);
template Result validateUpdatableNeuralNetwork<CoreML::Specification::NeuralNetworkRegressor>(CoreML::Specification::NeuralNetworkRegressor const&);
template Result validateUpdatableNeuralNetwork<CoreML::Specification::NeuralNetworkClassifier>(CoreML::Specification::NeuralNetworkClassifier const&);
