//
//  NeuralNetworkShapes.hpp
//  mlmodel
//
//  Created by William March on 12/5/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#ifndef MLMODEL_NeuralNetworkShapes_hpp
#define MLMODEL_NeuralNetworkShapes_hpp

#include "../build/format/NeuralNetwork_enums.h"
#include "Validators.hpp"
#include "ValidatorUtils-inl.hpp"
#include "LayerShapeConstraints.hpp"
#include "transforms/NeuralNetwork.hpp"
#include <iostream>
#include <functional>

#define COREML_VALIDATOR_VERBOSE 0

namespace CoreML {

    typedef std::function<void(const CoreML::Specification::NeuralNetworkLayer& specLayer)> shapeComputeFn;

    class NeuralNetworkShaper {

    public:

        NeuralNetworkShaper(const Specification::Model& model, bool useInputAndOutputConstraints = true);

        NeuralNetworkShaper(const Specification::ModelDescription& interface, const google::protobuf::RepeatedPtrField<Specification::NeuralNetworkLayer>& nn, bool useInputAndOutputConstraints = true);

        bool isValid() const;

        const ShapeConstraint& shape(const std::string& name) const;

        void print() const;

    private:

        void shapeConvolutionLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapePoolingLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeUnchanged(const Specification::NeuralNetworkLayer& specLayer);
        void shapeInnerProductLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeEmbeddingLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeCropLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapePaddingLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeUpsampleLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeBroadcastLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeDotLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeReduceLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeLoadConstantLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeReshapeLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeFlattenLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapePermuteLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeConcatLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeSplitLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeSequenceRepeatLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeReorganizeDataLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeSliceLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeSimpleRecurrentLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeGRULayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeUnidirectionalLSTMLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeBidirectionalLSTMLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeCustomLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeResizeBilinearLayer(const Specification::NeuralNetworkLayer& specLayer);
        void shapeCropResizeLayer(const Specification::NeuralNetworkLayer& specLayer);
        void ProcessLayer(const Specification::NeuralNetworkLayer& layer);
        void PassColorsDown(const Specification::NeuralNetworkLayer& layer);
        void PassColorsUp(const Specification::NeuralNetworkLayer& layer);
        bool AllShapesDone();

        shapeComputeFn getShapeFunction(const Specification::NeuralNetworkLayer::LayerCase& layerCase);

        int numColors;
        std::map<std::string, std::set<int> > blobColors;
        std::map<std::string, ShapeConstraint> blobShapes;

    };

}

#endif /* NeuralNetworkShapes_hpp */
