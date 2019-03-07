//
//  NNShaperTest.cpp
//  CoreML
//
//  Created by William March on 12/11/17.
//  Copyright © 2017 Apple Inc. All rights reserved.
//

#include "MLModelTests.hpp"
#include "../src/Format.hpp"
#include "../src/Model.hpp"
#include "../src/NeuralNetworkShapes.hpp"

#include "framework/TestUtils.hpp"

using namespace CoreML;

int testSimpleNNShape() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(4);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(3);

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_inputchannels(4);
    innerProductParams->set_outputchannels(3);
    for (int i = 0; i < 12; i++) {
        innerProductParams->mutable_weights()->add_floatvalue(1.0);
    }

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    NeuralNetworkShaper shapes = NeuralNetworkShaper(m1);

    ML_ASSERT(shapes.isValid());

    return 0;

}

int testSimpleNNShapeBad() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(3);

    auto *nn = m1.mutable_neuralnetwork();

    // The parameters of the layer don't match the network inputs & outputs
    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_inputchannels(4);
    innerProductParams->set_outputchannels(3);
    for (int i = 0; i < 12; i++) {
        innerProductParams->mutable_weights()->add_floatvalue(1.0);
    }

    innerProductParams->set_hasbias(false);

    // This is not valid from the perspective of the shapes in the network, but that is being raised as
    // a warning instead of being an invalid model.
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    ML_ASSERT(res.type() == ResultType::POTENTIALLY_INVALID_NEURAL_NETWORK_SHAPES);

    return 0;

}


int testSimpleNNShapeBadOutput() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(4);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(2);

    auto *nn = m1.mutable_neuralnetwork();

    // The parameters of the layer don't match the network inputs & outputs
    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_inputchannels(4);
    innerProductParams->set_outputchannels(3);
    for (int i = 0; i < 12; i++) {
        innerProductParams->mutable_weights()->add_floatvalue(1.0);
    }

    innerProductParams->set_hasbias(false);

    // This is not valid from the perspective of the shapes in the network, but that is being raised as
    // a warning instead of being an invalid model.
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    ML_ASSERT(res.type() == ResultType::POTENTIALLY_INVALID_NEURAL_NETWORK_SHAPES);

    return 0;

}

int testSimple1DConv() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(4);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(3);

    auto *nn = m1.mutable_neuralnetwork();

    // The parameters of the layer don't match the network inputs & outputs
    Specification::NeuralNetworkLayer *permLayer = nn->add_layers();
    permLayer->add_input("input");
    permLayer->add_output("perm1_out");
    Specification::PermuteLayerParams *permParams = permLayer->mutable_permute();
    permParams->add_axis(3);
    permParams->add_axis(1);
    permParams->add_axis(2);
    permParams->add_axis(0);

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("perm1_out");
    convLayer->add_output("conv_out");
    Specification::ConvolutionLayerParams *convParams = convLayer->mutable_convolution();
    convParams->set_outputchannels(5);
    convParams->set_kernelchannels(4);
    convParams->add_kernelsize(1);
    convParams->add_kernelsize(10);
    convParams->mutable_valid();
    convParams->set_hasbias(false);
    for (int i = 0; i < 200; i++) {
        convParams->mutable_weights()->add_floatvalue(1.0);
    }

    Specification::NeuralNetworkLayer *permLayer2 = nn->add_layers();
    permLayer2->add_input("conv_out");
    permLayer2->add_output("perm2_out");
    Specification::PermuteLayerParams *permParams2 = permLayer2->mutable_permute();
    permParams2->add_axis(3);
    permParams2->add_axis(1);
    permParams2->add_axis(2);
    permParams2->add_axis(0);

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("perm2_out");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_inputchannels(5);
    innerProductParams->set_outputchannels(3);
    for (int i = 0; i < 15; i++) {
        innerProductParams->mutable_weights()->add_floatvalue(1.0);
    }
    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    NeuralNetworkShaper shapes(m1);
    ML_ASSERT(shapes.isValid());

    ML_ASSERT(shapes.shape("input").sequenceRange().minimum().value() == 10);
    ML_ASSERT(shapes.shape("input").sequenceRange().maximum().isUnbound());

    ML_ASSERT(shapes.shape("input").batchRange().minimum().value() == 0);
    ML_ASSERT(shapes.shape("input").batchRange().maximum().isUnbound());

    ML_ASSERT(shapes.shape("input").channelRange().equals(4));
    ML_ASSERT(shapes.shape("input").heightRange().equals(1));
    ML_ASSERT(shapes.shape("input").widthRange().equals(1));

    ML_ASSERT(shapes.shape("conv_out").sequenceRange().equals(1));
    ML_ASSERT(shapes.shape("conv_out").batchRange().minimum().value() == 0);
    ML_ASSERT(shapes.shape("conv_out").batchRange().maximum().isUnbound());

    ML_ASSERT(shapes.shape("conv_out").channelRange().equals(5));
    ML_ASSERT(shapes.shape("conv_out").heightRange().equals(1));
    ML_ASSERT(shapes.shape("conv_out").widthRange().minimum().value() == 1);
    ML_ASSERT(shapes.shape("conv_out").widthRange().maximum().isUnbound());

    ML_ASSERT(shapes.shape("output").sequenceRange().minimum().value() == 1);
    ML_ASSERT(shapes.shape("output").sequenceRange().maximum().isUnbound());

    ML_ASSERT(shapes.shape("output").batchRange().minimum().value() == 0);
    ML_ASSERT(shapes.shape("output").batchRange().maximum().isUnbound());

    ML_ASSERT(shapes.shape("output").channelRange().equals(3));
    ML_ASSERT(shapes.shape("output").heightRange().equals(1));
    ML_ASSERT(shapes.shape("output").widthRange().equals(1));

    return 0;

}

int testPermuteShape() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(3);
    shape->add_shape(64);
    shape->add_shape(10);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    // The parameters of the layer don't match the network inputs & outputs
    Specification::NeuralNetworkLayer *permLayer = nn->add_layers();
    permLayer->add_input("input");
    permLayer->add_output("output");
    Specification::PermuteLayerParams *permParams = permLayer->mutable_permute();
    permParams->add_axis(0);
    permParams->add_axis(3);
    permParams->add_axis(2);
    permParams->add_axis(1);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    return 0;

}

int testUpwardPass() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    auto *shape1 = shape->mutable_shaperange()->add_sizeranges();
    shape1->set_lowerbound(4);
    shape1->set_upperbound(4);
    auto *shape2 = shape->mutable_shaperange()->add_sizeranges();
    shape2->set_lowerbound(1);
    shape2->set_upperbound(1);
    auto *shape3 = shape->mutable_shaperange()->add_sizeranges();
    shape3->set_lowerbound(1);
    shape3->set_upperbound(-1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("conv_out");
    Specification::ConvolutionLayerParams *convParams = convLayer->mutable_convolution();
    convParams->set_outputchannels(5);
    convParams->set_kernelchannels(4);
    convParams->add_kernelsize(1);
    convParams->add_kernelsize(10);
    convParams->mutable_valid();
    convParams->set_hasbias(false);
    for (int i = 0; i < 5*4*10; i++) {
        convParams->mutable_weights()->add_floatvalue(1.0);
    }

    Specification::NeuralNetworkLayer *ipLayer = nn->add_layers();
    ipLayer->add_input("conv_out");
    ipLayer->add_output("output");
    Specification::InnerProductLayerParams *ipParams = ipLayer->mutable_innerproduct();
    ipParams->set_inputchannels(5);
    ipParams->set_outputchannels(1);
    ipParams->set_hasbias(false);
    for (int i = 0; i < 5; i++) {
        ipParams->mutable_weights()->add_floatvalue(1.0);
    }

    NeuralNetworkShaper shapes(m1);

    const ShapeConstraint &inShape = shapes.shape("input");
    ML_ASSERT(inShape.sequenceRange().minimumValue() == 0);
    ML_ASSERT(inShape.sequenceRange().maximumValue().isUnbound());

    ML_ASSERT(inShape.batchRange().minimumValue() == 0);
    ML_ASSERT(inShape.batchRange().maximumValue().isUnbound());

    ML_ASSERT(inShape.channelRange().minimumValue() == 4);
    ML_ASSERT(inShape.channelRange().maximumValue().value() == 4);

    ML_ASSERT(inShape.heightRange().minimumValue() == 1);
    ML_ASSERT(inShape.heightRange().maximumValue().value() == 1);

    ML_ASSERT(inShape.widthRange().minimumValue() == 10);
    ML_ASSERT(inShape.widthRange().maximumValue().value() == 10);

    return 0;

}

int testSamePaddingConvolution() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    auto *shape1 = shape->mutable_shaperange()->add_sizeranges();
    shape1->set_lowerbound(4);
    shape1->set_upperbound(4);
    auto *shape2 = shape->mutable_shaperange()->add_sizeranges();
    shape2->set_lowerbound(100);
    shape2->set_upperbound(100);
    auto *shape3 = shape->mutable_shaperange()->add_sizeranges();
    shape3->set_lowerbound(100);
    shape3->set_upperbound(200);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("output");
    Specification::ConvolutionLayerParams *convParams = convLayer->mutable_convolution();
    convParams->set_outputchannels(5);
    convParams->set_kernelchannels(4);
    convParams->add_kernelsize(7);
    convParams->add_kernelsize(8);
    convParams->mutable_same();
    convParams->set_hasbias(false);
    for (int i = 0; i < 5*4*7*8; i++) {
        convParams->mutable_weights()->add_floatvalue(1.0);
    }

    NeuralNetworkShaper shapes(m1);

    const ShapeConstraint &inShape = shapes.shape("input");
    ML_ASSERT(inShape.sequenceRange().minimumValue() == 0);
    ML_ASSERT(inShape.sequenceRange().maximumValue().isUnbound());

    ML_ASSERT(inShape.batchRange().minimumValue() == 0);
    ML_ASSERT(inShape.batchRange().maximumValue().isUnbound());

    ML_ASSERT(inShape.channelRange().minimumValue() == 4);
    ML_ASSERT(inShape.channelRange().maximumValue().value() == 4);

    ML_ASSERT(inShape.heightRange().minimumValue() == 100);
    ML_ASSERT(inShape.heightRange().maximumValue().value() == 100);

    ML_ASSERT(inShape.widthRange().minimumValue() == 100);
    ML_ASSERT(inShape.widthRange().maximumValue().value() == 200);

    const ShapeConstraint &outShape = shapes.shape("output");
    ML_ASSERT(outShape.sequenceRange().minimumValue() == 0);
    ML_ASSERT(outShape.sequenceRange().maximumValue().isUnbound());

    ML_ASSERT(outShape.batchRange().minimumValue() == 0);
    ML_ASSERT(outShape.batchRange().maximumValue().isUnbound());

    ML_ASSERT(outShape.channelRange().minimumValue() == 5);
    ML_ASSERT(outShape.channelRange().maximumValue().value() == 5);

    ML_ASSERT(outShape.heightRange().minimumValue() == 100);
    ML_ASSERT(outShape.heightRange().maximumValue().value() == 100);

    ML_ASSERT(outShape.widthRange().minimumValue() == 100);
    ML_ASSERT(outShape.widthRange().maximumValue().value() == 200);


    return 0;

}

int testSamePaddingConvolution2() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    auto *shape1 = shape->mutable_shaperange()->add_sizeranges();
    shape1->set_lowerbound(4);
    shape1->set_upperbound(4);
    auto *shape2 = shape->mutable_shaperange()->add_sizeranges();
    shape2->set_lowerbound(100);
    shape2->set_upperbound(100);
    auto *shape3 = shape->mutable_shaperange()->add_sizeranges();
    shape3->set_lowerbound(100);
    shape3->set_upperbound(200);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("output");
    Specification::ConvolutionLayerParams *convParams = convLayer->mutable_convolution();
    convParams->set_outputchannels(5);
    convParams->set_kernelchannels(4);
    convParams->add_kernelsize(7);
    convParams->add_kernelsize(8);
    convParams->mutable_same();
    convParams->add_stride(2);
    convParams->add_stride(3);
    convParams->set_hasbias(false);
    for (int i = 0; i < 5*4*7*8; i++) {
        convParams->mutable_weights()->add_floatvalue(1.0);
    }

    NeuralNetworkShaper shapes(m1);

    const ShapeConstraint &inShape = shapes.shape("input");
    ML_ASSERT(inShape.sequenceRange().minimumValue() == 0);
    ML_ASSERT(inShape.sequenceRange().maximumValue().isUnbound());

    ML_ASSERT(inShape.batchRange().minimumValue() == 0);
    ML_ASSERT(inShape.batchRange().maximumValue().isUnbound());

    ML_ASSERT(inShape.channelRange().minimumValue() == 4);
    ML_ASSERT(inShape.channelRange().maximumValue().value() == 4);

    ML_ASSERT(inShape.heightRange().minimumValue() == 100);
    ML_ASSERT(inShape.heightRange().maximumValue().value() == 100);

    ML_ASSERT(inShape.widthRange().minimumValue() == 100);
    ML_ASSERT(inShape.widthRange().maximumValue().value() == 200);

    const ShapeConstraint &outShape = shapes.shape("output");
    ML_ASSERT(outShape.sequenceRange().minimumValue() == 0);
    ML_ASSERT(outShape.sequenceRange().maximumValue().isUnbound());

    ML_ASSERT(outShape.batchRange().minimumValue() == 0);
    ML_ASSERT(outShape.batchRange().maximumValue().isUnbound());

    ML_ASSERT(outShape.channelRange().minimumValue() == 5);
    ML_ASSERT(outShape.channelRange().maximumValue().value() == 5);

    ML_ASSERT(outShape.heightRange().minimumValue() == 50);
    ML_ASSERT(outShape.heightRange().maximumValue().value() == 50);

    ML_ASSERT(outShape.widthRange().minimumValue() == 34);
    ML_ASSERT(outShape.widthRange().maximumValue().value() == 67);


    return 0;

}
