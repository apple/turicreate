//
//  NNValidatorTests.cpp
//  libmlmodelspec
//
//  Created by Bill March on 3/16/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include "MLModelTests.hpp"
#include "../src/Format.hpp"
#include "../src/Model.hpp"
#include "../src/NeuralNetworkShapes.hpp"

#include "framework/TestUtils.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

using namespace CoreML;


int testNNValidatorSimple() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_inputchannels(1);
    innerProductParams->set_outputchannels(1);
    innerProductParams->mutable_weights()->add_floatvalue(1.0);

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    return 0;
}

int testNNValidatorBadInput() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    topIn->mutable_type()->mutable_multiarraytype();

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}

int testNNValidatorBadInput2() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    shape->add_shape(2);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}

int testNNValidatorBadOutput() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("bad_name");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}

int testNNValidatorBadOutput2() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}



int testNNValidatorAllOptional() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("A");
    auto type = topIn->mutable_type();
    type->mutable_multiarraytype();
    type->set_isoptional(true);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}


int testNNValidatorMissingInput() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("E");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m1.mutable_description()->add_output();
    out->set_name("D");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *ip1 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams1 = ip1->mutable_innerproduct();
    ip1->set_name("ip1");

    innerProductParams1->set_hasbias(false);

    Specification::NeuralNetworkLayer *ip2 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams2 = ip2->mutable_innerproduct();

    innerProductParams2->set_hasbias(false);

    ip2->set_name("ip2");

    Specification::NeuralNetworkLayer *ip3 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams3 = ip3->mutable_innerproduct();

    innerProductParams3->set_hasbias(false);

    ip3->set_name("ip3");


    // Make a loop

    ip1->add_input("A");
    ip1->add_output("B");

    ip2->add_input("B");
    ip2->add_output("C");

    ip3->add_input("C");
    ip3->add_output("D");

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}

int testNNValidatorMissingOutput() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("E");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *ip1 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams1 = ip1->mutable_innerproduct();
    ip1->set_name("ip1");

    innerProductParams1->set_hasbias(false);

    Specification::NeuralNetworkLayer *ip2 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams2 = ip2->mutable_innerproduct();

    innerProductParams2->set_hasbias(false);

    ip2->set_name("ip2");

    Specification::NeuralNetworkLayer *ip3 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams3 = ip3->mutable_innerproduct();

    innerProductParams3->set_hasbias(false);

    ip3->set_name("ip3");

    // Make a loop

    ip1->add_input("A");
    ip1->add_output("B");

    ip2->add_input("B");
    ip2->add_output("C");

    ip3->add_input("C");
    ip3->add_output("D");

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}

int testNNValidatorLoop() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *ip1 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams1 = ip1->mutable_innerproduct();
    ip1->set_name("ip1");

    innerProductParams1->set_hasbias(false);

    Specification::NeuralNetworkLayer *ip2 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams2 = ip2->mutable_innerproduct();

    innerProductParams2->set_hasbias(false);

    ip2->set_name("ip2");

    Specification::NeuralNetworkLayer *ip3 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams3 = ip3->mutable_innerproduct();

    innerProductParams3->set_hasbias(false);
    ip3->set_name("ip3");

    // Make a loop

    ip1->add_input("A");
    ip1->add_output("B");

    ip2->add_input("B");
    ip2->add_output("C");

    ip3->add_input("C");
    ip3->add_output("A");

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}


// No input description
int testNNValidatorBadInputs() {

    Specification::Model m1;

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();

    innerProductParams->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}

int testRNNLayer() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();
    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->set_name("rnn");
    layer->add_input("A");
    layer->add_output("B");
    Specification::SimpleRecurrentLayerParams *params = layer->mutable_simplerecurrent();
    params->set_hasbiasvector(false);
    params->set_sequenceoutput(false);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;

}

int testRNNLayer2() {

    Specification::Model m1;

    // recurrent layers don't appear in the interface
    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();

    auto *nn = m1.mutable_neuralnetwork();
    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->set_name("rnn");
    layer->add_input("input");
    layer->add_input("hin");

    layer->add_output("output");
    layer->add_output("hout");

    Specification::SimpleRecurrentLayerParams *params = layer->mutable_simplerecurrent();
    params->set_hasbiasvector(false);
    params->set_sequenceoutput(false);
    params->set_inputvectorsize(1);
    params->set_outputvectorsize(2);
    params->mutable_activation()->mutable_sigmoid();

    params->mutable_weightmatrix()->add_floatvalue(1.0);
    params->mutable_weightmatrix()->add_floatvalue(1.0);

    params->mutable_recursionmatrix()->add_floatvalue(1.0);
    params->mutable_recursionmatrix()->add_floatvalue(1.0);
    params->mutable_recursionmatrix()->add_floatvalue(1.0);
    params->mutable_recursionmatrix()->add_floatvalue(1.0);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;

}

int testNNValidatorReshape3D() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *reshapeLayer = nn->add_layers();
    reshapeLayer->add_input("input");
    reshapeLayer->add_output("output");
    Specification::ReshapeLayerParams *reshapeParams = reshapeLayer->mutable_reshape();

    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);

    reshapeParams->set_mode(::CoreML::Specification::ReshapeLayerParams_ReshapeOrder::ReshapeLayerParams_ReshapeOrder_CHANNEL_FIRST); Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    return 0;
}

int testNNValidatorReshape4D() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *reshapeLayer = nn->add_layers();
    reshapeLayer->add_input("input");
    reshapeLayer->add_output("output");
    Specification::ReshapeLayerParams *reshapeParams = reshapeLayer->mutable_reshape();

    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);

    reshapeParams->set_mode(::CoreML::Specification::ReshapeLayerParams_ReshapeOrder::ReshapeLayerParams_ReshapeOrder_CHANNEL_FIRST); Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    return 0;
}

int testNNValidatorReshapeBad() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);

    auto *nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *reshapeLayer = nn->add_layers();
    reshapeLayer->add_input("input");
    reshapeLayer->add_output("output");
    Specification::ReshapeLayerParams *reshapeParams = reshapeLayer->mutable_reshape();

    // 5 entries here instead of 3/4
    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);
    reshapeParams->add_targetshape(1);

    reshapeParams->set_mode(::CoreML::Specification::ReshapeLayerParams_ReshapeOrder::ReshapeLayerParams_ReshapeOrder_CHANNEL_FIRST);
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}


int testNNCompilerValidation() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    topIn->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("middle");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);
    out->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_DOUBLE);


    auto *out2 = m1.mutable_description()->add_output();
    out2->set_name("features");
    out2->mutable_type()->mutable_stringtype();

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_dictionarytype();
    out3->mutable_type()->mutable_dictionarytype()->mutable_stringkeytype();

    std::string featureName = "features";
    m1.mutable_description()->set_predictedfeaturename(featureName);
    std::string probsName = "probs";
    m1.mutable_description()->set_predictedprobabilitiesname(probsName);


    const auto nn = m1.mutable_neuralnetworkclassifier();
    auto labels = nn->mutable_stringclasslabels();
    labels->add_vector("label1");

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("middle");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_inputchannels(1);
    innerProductParams->set_outputchannels(1);
    innerProductParams->mutable_weights()->add_floatvalue(1.0);
    innerProductParams->set_hasbias(false);

    Specification::NeuralNetworkLayer *innerProductLayer2 = nn->add_layers();
    innerProductLayer2->add_input("middle");
    innerProductLayer2->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams2 = innerProductLayer2->mutable_innerproduct();
    innerProductParams2->set_hasbias(false);
    innerProductParams2->set_inputchannels(1);
    innerProductParams2->set_outputchannels(1);
    innerProductParams2->mutable_weights()->add_floatvalue(1.0);

    Result res = validate<MLModelType_neuralNetworkClassifier>(m1);
    ML_ASSERT_GOOD(res);
    return 0;

}

int testNNCompilerValidationGoodProbBlob() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    topIn->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("middle");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);
    out->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);

    auto *out2 = m1.mutable_description()->add_output();
    out2->set_name("features");
    out2->mutable_type()->mutable_stringtype();

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_dictionarytype();
    out3->mutable_type()->mutable_dictionarytype()->mutable_stringkeytype();

    std::string featureName = "features";
    m1.mutable_description()->set_predictedfeaturename(featureName);
    std::string probsName = "probs";
    m1.mutable_description()->set_predictedprobabilitiesname(probsName);


    const auto nn = m1.mutable_neuralnetworkclassifier();
    auto labels = nn->mutable_stringclasslabels();
    labels->add_vector("label1");
    nn->set_labelprobabilitylayername("middle");

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("middle");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_inputchannels(1);
    innerProductParams->set_outputchannels(1);
    innerProductParams->mutable_weights()->add_floatvalue(1.0);
    innerProductParams->set_hasbias(false);

    Specification::NeuralNetworkLayer *innerProductLayer2 = nn->add_layers();
    innerProductLayer2->add_input("middle");
    innerProductLayer2->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams2 = innerProductLayer2->mutable_innerproduct();
    innerProductParams2->set_hasbias(false);
    innerProductParams2->set_inputchannels(1);
    innerProductParams2->set_outputchannels(1);
    innerProductParams2->mutable_weights()->add_floatvalue(1.0);

    Result res = validate<MLModelType_neuralNetworkClassifier>(m1);
    ML_ASSERT_GOOD(res);
    return 0;

}

int testNNCompilerValidationBadProbBlob() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    topIn->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("middle");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);
    out->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);

    auto *out2 = m1.mutable_description()->add_output();
    out2->set_name("features");
    out2->mutable_type()->mutable_stringtype();

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_dictionarytype();
    out3->mutable_type()->mutable_dictionarytype()->mutable_stringkeytype();

    std::string featureName = "features";
    m1.mutable_description()->set_predictedfeaturename(featureName);
    std::string probsName = "probs";
    m1.mutable_description()->set_predictedprobabilitiesname(probsName);


    const auto nn = m1.mutable_neuralnetworkclassifier();
    auto labels = nn->mutable_stringclasslabels();
    labels->add_vector("label1");
    nn->set_labelprobabilitylayername("not_here");

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("middle");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    innerProductParams->set_hasbias(false);

    Specification::NeuralNetworkLayer *innerProductLayer2 = nn->add_layers();
    innerProductLayer2->add_input("middle");
    innerProductLayer2->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams2 = innerProductLayer2->mutable_innerproduct();
    innerProductParams2->set_hasbias(false);

    Result res = validate<MLModelType_neuralNetworkClassifier>(m1);
    ML_ASSERT_BAD(res);
    return 0;

}

int testInvalidPooling() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(100);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *poolingLayer = nn->add_layers();
    poolingLayer->add_input("input");
    poolingLayer->add_output("probs");
    poolingLayer->mutable_pooling();

    // not specifying a padding type should be invalid
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;

}

int testInvalidConvolutionNoPadding() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(3);
    shape->add_shape(100);
    shape->add_shape(100);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("probs");
    auto *params = convLayer->mutable_convolution();
    params->set_outputchannels(5);
    params->set_kernelchannels(3);

    params->set_hasbias(false);

    // not specifying a padding type should be invalid
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;

}

int testInvalidConvolutionNoWeights() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(3);
    shape->add_shape(100);
    shape->add_shape(100);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("probs");
    auto *params = convLayer->mutable_convolution();
    params->set_outputchannels(5);
    params->set_kernelchannels(3);

    params->set_hasbias(false);

    (void)params->mutable_valid();

    // Not specifying the right number of weights should be invalid
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;

}

int testInvalidConvolutionNoBias() {

    Specification::Model m1;

    int output_channels = 5;
    int kernel_channels = 3;
    int kernel_height = 2;
    int kernel_width = 5;
    int nGroups = 1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(3);
    shape->add_shape(100);
    shape->add_shape(100);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("probs");
    auto *params = convLayer->mutable_convolution();
    params->set_outputchannels(5);
    params->set_kernelchannels(3);
    params->add_kernelsize(kernel_height);
    params->add_kernelsize(kernel_width);

    params->set_hasbias(true);

    (void)params->mutable_valid();

    for (int i = 0; i < output_channels * (kernel_channels / nGroups) * kernel_height * kernel_width; i++) {
        params->mutable_weights()->add_floatvalue(1.0);
    }

    // Not specifying the right number of weights should be invalid

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;

}

int testValidConvolution() {

    Specification::Model m1;

    int output_channels = 5;
    int kernel_channels = 3;
    int kernel_height = 2;
    int kernel_width = 5;
    int nGroups = 1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(3);
    shape->add_shape(100);
    shape->add_shape(100);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("probs");
    auto *params = convLayer->mutable_convolution();
    params->set_outputchannels(5);
    params->set_kernelchannels(3);
    params->add_kernelsize(kernel_height);
    params->add_kernelsize(kernel_width);

    params->set_hasbias(true);

    (void)params->mutable_valid();

    for (int i = 0; i < output_channels * (kernel_channels / nGroups) * kernel_height * kernel_width; i++) {
        params->mutable_weights()->add_floatvalue(1.0);
    }

    for (int i = 0; i < output_channels; i++) {
        params->mutable_bias()->add_floatvalue(1.0);
    }

    // Not specifying the right number of weights should be invalid
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;

}


int testValidDeconvolution() {

    Specification::Model m1;

    int output_channels = 5;
    int kernel_channels = 3;
    int kernel_height = 2;
    int kernel_width = 5;
    int nGroups = 1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(3);
    shape->add_shape(100);
    shape->add_shape(100);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("probs");
    auto *params = convLayer->mutable_convolution();
    params->set_outputchannels(5);
    params->set_kernelchannels(3);
    params->add_kernelsize(kernel_height);
    params->add_kernelsize(kernel_width);

    params->set_hasbias(true);

    params->set_isdeconvolution(true);
    params->add_outputshape(110);
    params->add_outputshape(110);

    (void)params->mutable_valid();

    for (int i = 0; i < output_channels * (kernel_channels / nGroups) * kernel_height * kernel_width; i++) {
        params->mutable_weights()->add_floatvalue(1.0);
    }

    for (int i = 0; i < output_channels; i++) {
        params->mutable_bias()->add_floatvalue(1.0);
    }

    // Not specifying the right number of weights should be invalid
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidEmbedding() {

    Specification::Model m1;

    int num_inputs = 5;
    int num_outputs = 3;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *embeddingLayer = nn->add_layers();
    embeddingLayer->add_input("input");
    embeddingLayer->add_output("probs");
    auto *params = embeddingLayer->mutable_embedding();
    params->set_inputdim(num_inputs);
    params->set_outputchannels(num_outputs);

    params->set_hasbias(false);

    // Not specifying the right number of weights should be invalid
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidEmbeddingBias() {

    Specification::Model m1;

    int num_inputs = 5;
    int num_outputs = 3;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *embeddingLayer = nn->add_layers();
    embeddingLayer->add_input("input");
    embeddingLayer->add_output("probs");
    auto *params = embeddingLayer->mutable_embedding();
    params->set_inputdim(num_inputs);
    params->set_outputchannels(num_outputs);

    params->set_hasbias(true);

    for (int i = 0; i < num_inputs * num_outputs; i++) {
        params->mutable_weights()->add_floatvalue(1.0);
    }

    // Not specifying the right number of bias weights should be invalid

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidEmbedding() {

    Specification::Model m1;

    int num_inputs = 5;
    int num_outputs = 3;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *embeddingLayer = nn->add_layers();
    embeddingLayer->add_input("input");
    embeddingLayer->add_output("probs");
    auto *params = embeddingLayer->mutable_embedding();
    params->set_inputdim(num_inputs);
    params->set_outputchannels(num_outputs);

    params->set_hasbias(true);

    for (int i = 0; i < num_inputs * num_outputs; i++) {
        params->mutable_weights()->add_floatvalue(1.0);
    }

    for (int i = 0; i < num_outputs; i++) {
        params->mutable_bias()->add_floatvalue(1.0);
    }

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidBatchnorm() {

    Specification::Model m1;

    int num_inputs = 5;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *batchnormLayer = nn->add_layers();
    batchnormLayer->add_input("input");
    batchnormLayer->add_output("probs");
    auto *params = batchnormLayer->mutable_batchnorm();
    params->set_channels(num_inputs);

    for (int i = 0; i < num_inputs; i++) {
        params->mutable_beta()->add_floatvalue(1.0);
        params->mutable_gamma()->add_floatvalue(1.0);
    }
    // Invalid because the mean and variance should be provided

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidComputeMeanVarBatchnorm() {

    Specification::Model m1;

    int num_inputs = 5;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *batchnormLayer = nn->add_layers();
    batchnormLayer->add_input("input");
    batchnormLayer->add_output("probs");
    auto *params = batchnormLayer->mutable_batchnorm();
    params->set_channels(num_inputs);

    params->set_computemeanvar(true);

    for (int i = 0; i < num_inputs; i++) {
        params->mutable_beta()->add_floatvalue(1.0);
        params->mutable_gamma()->add_floatvalue(1.0);
    }
    // Valid because the mean and variance will be computed

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidPaddingBorder() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *paddingLayer = nn->add_layers();
    paddingLayer->add_input("input");
    paddingLayer->add_output("probs");
    auto *params = paddingLayer->mutable_padding();

    // If border amounts set, they need to be set in both directions
    params->mutable_paddingamounts()->add_borderamounts();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidPaddingNoType() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *paddingLayer = nn->add_layers();
    paddingLayer->add_input("input");
    paddingLayer->add_output("probs");
    auto *params = paddingLayer->mutable_padding();

    // If border amounts set, they need to be set in both directions
    params->mutable_paddingamounts()->add_borderamounts();
    params->mutable_paddingamounts()->add_borderamounts();

    // There is no padding type set
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidPadding() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *paddingLayer = nn->add_layers();
    paddingLayer->add_input("input");
    paddingLayer->add_output("probs");
    auto *params = paddingLayer->mutable_padding();

    // If border amounts set, they need to be set in both directions
    params->mutable_paddingamounts()->add_borderamounts();
    params->mutable_paddingamounts()->add_borderamounts();

    (void)params->mutable_constant();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidUpsample() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *upsampleLayer = nn->add_layers();
    upsampleLayer->add_input("input");
    upsampleLayer->add_output("probs");
    auto *params = upsampleLayer->mutable_upsample();

    // Scaling factor needs to be 2D
    params->add_scalingfactor(1.0);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidUpsample() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *upsampleLayer = nn->add_layers();
    upsampleLayer->add_input("input");
    upsampleLayer->add_output("probs");
    auto *params = upsampleLayer->mutable_upsample();

    // Scaling factor needs to be 2D
    params->add_scalingfactor(1.0);
    params->add_scalingfactor(1.0);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testValidScale() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *scaleLayer = nn->add_layers();
    scaleLayer->add_input("input");
    scaleLayer->add_output("probs");
    auto *params = scaleLayer->mutable_scale();
    int num_channel = 5;
    params->add_shapescale(num_channel);

    for (int i = 0; i < num_channel; i++) {
        params->mutable_scale()->add_floatvalue(1.0);
    }

    int num_bias = 3;
    params->add_shapebias(num_bias);
    params->set_hasbias(true);
    for (int i = 0; i < num_bias; i++) {
        params->mutable_bias()->add_floatvalue(1.0);
    }

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testValidScaleNoBias() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *scaleLayer = nn->add_layers();
    scaleLayer->add_input("input");
    scaleLayer->add_output("probs");
    auto *params = scaleLayer->mutable_scale();
    int num_channel = 5;
    params->add_shapescale(num_channel);

    for (int i = 0; i < num_channel; i++) {
        params->mutable_scale()->add_floatvalue(1.0);
    }

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}


int testInvalidScaleLength() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *scaleLayer = nn->add_layers();
    scaleLayer->add_input("input");
    scaleLayer->add_output("probs");
    auto *params = scaleLayer->mutable_scale();
    int num_channel = 5;
    // shape scale needs length 1 or 3
    params->add_shapescale(num_channel);
    params->add_shapescale(num_channel);

    for (int i = 0; i < num_channel; i++) {
        params->mutable_scale()->add_floatvalue(1.0);
    }

    int num_bias = 3;
    params->add_shapebias(num_bias);
    params->set_hasbias(true);
    for (int i = 0; i < num_bias; i++) {
        params->mutable_bias()->add_floatvalue(1.0);
    }

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidScaleBiasLength() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *scaleLayer = nn->add_layers();
    scaleLayer->add_input("input");
    scaleLayer->add_output("probs");
    auto *params = scaleLayer->mutable_scale();
    int num_channel = 5;
    params->add_shapescale(num_channel);

    for (int i = 0; i < num_channel; i++) {
        params->mutable_scale()->add_floatvalue(1.0);
    }

    int num_bias = 3;
    // shape scale needs length 1 or 3
    params->add_shapebias(num_bias);
    params->add_shapebias(num_bias);
    params->add_shapebias(num_bias);
    params->add_shapebias(num_bias);

    params->set_hasbias(true);
    for (int i = 0; i < num_bias; i++) {
        params->mutable_bias()->add_floatvalue(1.0);
    }

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidScaleWeights() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *scaleLayer = nn->add_layers();
    scaleLayer->add_input("input");
    scaleLayer->add_output("probs");
    auto *params = scaleLayer->mutable_scale();
    int num_channel = 5;
    params->add_shapescale(num_channel);

    for (int i = 0; i < num_channel - 1; i++) {
        params->mutable_scale()->add_floatvalue(1.0);
    }

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidScaleBiasWeights() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *scaleLayer = nn->add_layers();
    scaleLayer->add_input("input");
    scaleLayer->add_output("probs");
    auto *params = scaleLayer->mutable_scale();
    int num_channel = 5;
    params->add_shapescale(num_channel);

    for (int i = 0; i < num_channel - 1; i++) {
        params->mutable_scale()->add_floatvalue(1.0);
    }

    int num_bias = 3;
    // shape scale needs length 1 or 3
    params->add_shapebias(num_bias);
    params->add_shapebias(num_bias);
    params->add_shapebias(num_bias);

    params->set_hasbias(true);
    for (int i = 0; i < num_bias*num_bias*num_bias - 1; i++) {
        params->mutable_bias()->add_floatvalue(1.0);
    }

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidCrop1() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *cropLayer = nn->add_layers();
    cropLayer->add_input("input");
    cropLayer->add_output("probs");
    auto *params = cropLayer->mutable_crop();
    auto *amounts = params->mutable_cropamounts();
    (void)amounts->add_borderamounts();
    (void)amounts->add_borderamounts();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidCrop1() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *cropLayer = nn->add_layers();
    cropLayer->add_input("input");
    cropLayer->add_output("probs");
    auto *params = cropLayer->mutable_crop();
    auto *amounts = params->mutable_cropamounts();
    (void)amounts->add_borderamounts();
    (void)amounts->add_borderamounts();
    (void)amounts->add_borderamounts();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidCrop2() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    shape->add_shape(234);
    shape->add_shape(332);

    auto *topIn2 = m1.mutable_description()->add_input();
    topIn2->set_name("input2");
    auto* shape2 = topIn2->mutable_type()->mutable_multiarraytype();
    shape2->add_shape(2);
    shape2->add_shape(10);
    shape2->add_shape(11);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *cropLayer = nn->add_layers();
    cropLayer->add_input("input");
    cropLayer->add_input("input2");
    cropLayer->add_output("probs");
    auto *params = cropLayer->mutable_crop();
    params->add_offset(1);
    params->add_offset(2);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    NeuralNetworkShaper shaper(m1);
    ML_ASSERT(shaper.isValid());

    ML_ASSERT(shaper.shape("probs").channelRange().minimum().value() == 1);
    ML_ASSERT(shaper.shape("probs").channelRange().maximum().value() == 1);

    ML_ASSERT(shaper.shape("probs").heightRange().minimum().value() == 10);
    ML_ASSERT(shaper.shape("probs").heightRange().maximum().value() == 10);

    ML_ASSERT(shaper.shape("probs").widthRange().minimum().value() == 11);
    ML_ASSERT(shaper.shape("probs").widthRange().maximum().value() == 11);

    return 0;
}

int testInvalidCrop2() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *topIn2 = m1.mutable_description()->add_input();
    topIn2->set_name("input2");
    auto* shape2 = topIn2->mutable_type()->mutable_multiarraytype();
    shape2->add_shape(2);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *cropLayer = nn->add_layers();
    cropLayer->add_input("input");
    cropLayer->add_input("input2");
    cropLayer->add_output("probs");
    auto *params = cropLayer->mutable_crop();
    params->add_offset(1);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidCrop3() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    auto *chanShape = shape->mutable_shaperange()->add_sizeranges();
    chanShape->set_lowerbound(6);
    chanShape->set_upperbound(6);
    auto *heightRange = shape->mutable_shaperange()->add_sizeranges();
    heightRange->set_lowerbound(100);
    heightRange->set_upperbound(1000);
    auto *widthRange = shape->mutable_shaperange()->add_sizeranges();
    widthRange->set_lowerbound(5);
    widthRange->set_upperbound(15);


    shape->add_shape(234);
    shape->add_shape(332);

    auto *topIn2 = m1.mutable_description()->add_input();
    topIn2->set_name("input2");
    auto* shape2 = topIn2->mutable_type()->mutable_multiarraytype();
    shape2->add_shape(2);
    shape2->add_shape(10);
    shape2->add_shape(11);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *cropLayer = nn->add_layers();
    cropLayer->add_input("input");
    cropLayer->add_input("input2");
    cropLayer->add_output("probs");
    auto *params = cropLayer->mutable_crop();
    params->add_offset(1);
    params->add_offset(2);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);

    return 0;
}

int testInvalidSlice() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    shape->add_shape(10);
    shape->add_shape(11);
    shape->add_shape(12);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    auto *params = layer->mutable_slice();

    // Invalid because the end is before the start
    params->set_startindex(5);
    params->set_endindex(4);
    params->set_stride(2);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidSlice1() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    shape->add_shape(17);
    shape->add_shape(11);
    shape->add_shape(12);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    auto *params = layer->mutable_slice();

    params->set_startindex(5);
    params->set_endindex(17);
    params->set_stride(2);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}


int testValidSlice2() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    shape->add_shape(10);
    shape->add_shape(11);
    shape->add_shape(12);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    auto *params = layer->mutable_slice();

    params->set_startindex(5);
    // The validator can't know if the input is big enough for this or not
    params->set_endindex(-3);
    params->set_stride(2);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}


int testValidCustom() {

    Specification::Model m1;
    m1.set_specificationversion(MLMODEL_SPECIFICATION_VERSION);

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    shape->add_shape(10);
    shape->add_shape(11);
    shape->add_shape(12);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    auto *params = layer->mutable_custom();

    params->set_classname("CustomClassName");

    auto *weights = params->add_weights();
    weights->set_float16value("somebitshere");

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    // We'll also test that the spec version is correct here
    Model mlmodel = Model(m1);
    ML_ASSERT(mlmodel.getProto().specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS11_2);

    return 0;
}

int testInvalidCustomNoName() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    shape->add_shape(10);
    shape->add_shape(11);
    shape->add_shape(12);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    auto *params = layer->mutable_custom();

    // No name, should be invalids
//    params->set_classname("CustomClassName");

    auto *weights = params->add_weights();
    weights->set_float16value("somebitshere");

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidCustomMultipleWeights() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    shape->add_shape(10);
    shape->add_shape(11);
    shape->add_shape(12);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    auto *params = layer->mutable_custom();

    params->set_classname("CustomClassName");

    auto *weights = params->add_weights();
    weights->set_float16value("somebitshere");

    auto *weights2 = params->add_weights();
    weights2->set_float16value("bitsbits");
    weights2->set_rawvalue("morebits");

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testSpecDowngrade() {

    Specification::Model m1;
    m1.set_specificationversion(MLMODEL_SPECIFICATION_VERSION);

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    shape->add_shape(10);
    shape->add_shape(11);
    shape->add_shape(12);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    auto *params = layer->mutable_slice();

    params->set_startindex(5);
    // The validator can't know if the input is big enough for this or not
    params->set_endindex(-3);
    params->set_stride(2);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    Model mlmodel = Model(m1);

    ML_ASSERT(mlmodel.getProto().specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS11);

    return 0;

}

int testSpecDowngradefp16() {

    Specification::Model m1;
    m1.set_specificationversion(MLMODEL_SPECIFICATION_VERSION);

    int output_channels = 5;
    int kernel_channels = 3;
    int kernel_height = 2;
    int kernel_width = 5;
    int nGroups = 1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(3);
    shape->add_shape(100);
    shape->add_shape(100);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *convLayer = nn->add_layers();
    convLayer->add_input("input");
    convLayer->add_output("probs");
    auto *params = convLayer->mutable_convolution();
    params->set_outputchannels(5);
    params->set_kernelchannels(3);
    params->add_kernelsize(kernel_height);
    params->add_kernelsize(kernel_width);

    params->set_hasbias(true);

    (void)params->mutable_valid();

    int num_weights = output_channels * (kernel_channels / nGroups) * kernel_height * kernel_width;
    for (int i = 0; i < num_weights; i++) {
        params->mutable_weights()->set_float16value(std::string(num_weights*2, 'a'));
    }

    for (int i = 0; i < output_channels; i++) {
        params->mutable_bias()->set_float16value(std::string(output_channels*2, 'b'));
    }

    // Not specifying the right number of weights should be invalid
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    Model mlmodel = Model(m1);

    ML_ASSERT(mlmodel.getProto().specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS11_2);

    return 0;

}

int testSpecDowngradeFlexibleShapes() {

    Specification::Model m1;
    m1.set_specificationversion(MLMODEL_SPECIFICATION_VERSION_IOS12);

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *array_type = topIn->mutable_type()->mutable_multiarraytype();
    array_type->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);

    auto *array_shape1_range = array_type->mutable_shaperange()->add_sizeranges();
    array_shape1_range->set_lowerbound(10);
    array_shape1_range->set_upperbound(10);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");

    auto *params = layer->mutable_unary();
    params->set_type(Specification::UnaryFunctionLayerParams::ABS);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    Model mlmodel = Model(m1);

    ML_ASSERT(mlmodel.getProto().specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS12);

    return 0;

}


int testSpecDowngradeFlexibleShapes2() {

    Specification::Model m1;
    m1.set_specificationversion(MLMODEL_SPECIFICATION_VERSION_IOS12);

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *array_type = topIn->mutable_type()->mutable_multiarraytype();
    array_type->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);

    array_type->add_shape(10);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("probs");
    auto *outvec = out->mutable_type()->mutable_multiarraytype();
    outvec->set_datatype(Specification::ArrayFeatureType_ArrayDataType_DOUBLE);

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");

    auto *params = layer->mutable_unary();
    params->set_type(Specification::UnaryFunctionLayerParams::ABS);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);

    Model mlmodel = Model(m1);

    ML_ASSERT(mlmodel.getProto().specificationversion() == MLMODEL_SPECIFICATION_VERSION_IOS11);

    return 0;

}




#pragma clang diagnostic pop
