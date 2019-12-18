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
#include "../src/NeuralNetwork/NeuralNetworkShapes.hpp"

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

// Fuzzing creates a NN model with *no* layers. Guard against this.
int testNNMissingLayer() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);

    auto *out = m1.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();

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

int testValidSoftmax() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);
    shape->add_shape(5);
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    layer->add_inputtensor()->set_rank(3);
    (void) layer->mutable_softmax();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidSoftmax() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->set_name("softmax");
    layer->add_input("input");
    layer->add_output("probs");
    layer->add_inputtensor()->set_rank(2); // rank must be at least 3
    (void) layer->mutable_softmax();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidSoftmax2() {
    
    Specification::Model m1;
    
    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    // rank must be at least length 3
    shape->add_shape(5);
    shape->add_shape(5);
    
    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();
    
    const auto nn = m1.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->set_name("softmax");
    layer->add_input("input");
    layer->add_output("probs");
    (void) layer->mutable_softmax();
    
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testValidReduce() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);
    shape->add_shape(5);
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->set_name("reduce");
    layer->add_input("input");
    layer->add_output("probs");
    layer->add_inputtensor()->set_rank(3);
    auto params = layer->mutable_reduce();
    params->set_mode(Specification::ReduceLayerParams::SUM);
    params->set_axis(Specification::ReduceLayerParams::CHW);

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidReduce() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->set_name("reduce");
    layer->add_input("input");
    layer->add_output("probs");
    layer->add_inputtensor()->set_rank(2);
    auto params = layer->mutable_reduce();
    params->set_mode(Specification::ReduceLayerParams::SUM);
    params->set_axis(Specification::ReduceLayerParams::CHW); // rank must be at least 3

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidRank() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);
    shape->add_shape(5);
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->set_name("softmax");
    layer->add_input("input");
    layer->add_output("probs");
    layer->add_inputtensor()->set_rank(2); // this is incorrect, rank must be 3 since shape is (5,5,5)
    (void) layer->mutable_softmax();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
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

int testValidTransposeND() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    layer->add_inputtensor()->set_rank(2);
    layer->add_outputtensor()->set_rank(2);
    auto* params = layer->mutable_transpose();
    params->add_axes(1);
    params->add_axes(0);
    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidTransposeNdNoAxis() {

    Specification::Model m1;

    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(5);
    shape->add_shape(5);

    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_multiarraytype();

    const auto nn = m1.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    Specification::NeuralNetworkLayer *layer = nn->add_layers();
    layer->add_input("input");
    layer->add_output("probs");
    layer->add_inputtensor()->set_rank(2);
    layer->add_outputtensor()->set_rank(2);

    layer->mutable_transpose();

    Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("required") != std::string::npos);

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

int testValidBranch() {
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    // "If" net
    Specification::NeuralNetwork nnIf;
    auto *l1 = nnIf.add_layers();
    (void)l1->mutable_activation()->mutable_relu();
    l1->set_name("if_relu");
    l1->add_input("A");
    l1->add_output("B");
    
    // "else" net
    Specification::NeuralNetwork nnElse;
    auto *l2 = nnElse.add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("else_relu");
    l2->add_input("A");
    l2->add_output("B");
    
    // Main network
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l3 = nnMain->add_layers();
    (void)l3->mutable_activation()->mutable_relu();
    l3->set_name("condition_producing_layer");
    l3->add_input("A");
    l3->add_output("cond");
    
    auto *l4 = nnMain->add_layers();
    auto *branch_layer = l4->mutable_branch();
    l4->set_name("branch_layer");
    l4->add_input("cond");
    branch_layer->mutable_ifbranch()->CopyFrom(nnIf);
    branch_layer->mutable_elsebranch()->CopyFrom(nnElse);
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_GOOD(res);
    return 0;
}

int testInvalidBranchOutputNotProduced1() {
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    // "If" net
    Specification::NeuralNetwork nnIf;
    auto *l1 = nnIf.add_layers();
    (void)l1->mutable_activation()->mutable_relu();
    l1->set_name("if_relu");
    l1->add_input("A");
    l1->add_output("B");
    
    // "else" net
    Specification::NeuralNetwork nnElse;
    auto *l2 = nnElse.add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("else_relu");
    l2->add_input("A");
    l2->add_output("B2");
    
    // Main network
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l3 = nnMain->add_layers();
    (void)l3->mutable_activation()->mutable_relu();
    l3->set_name("condition_producing_layer");
    l3->add_input("A");
    l3->add_output("cond");
    
    auto *l4 = nnMain->add_layers();
    auto *branch_layer = l4->mutable_branch();
    l4->set_name("branch_layer");
    l4->add_input("cond");
    branch_layer->mutable_ifbranch()->CopyFrom(nnIf);
    branch_layer->mutable_elsebranch()->CopyFrom(nnElse);
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidBranchOutputNotProduced2() {
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    // "If" net
    Specification::NeuralNetwork nnIf;
    auto *l1 = nnIf.add_layers();
    (void)l1->mutable_activation()->mutable_relu();
    l1->set_name("if_relu");
    l1->add_input("A");
    l1->add_output("B");
    
    // Main network
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l3 = nnMain->add_layers();
    (void)l3->mutable_activation()->mutable_relu();
    l3->set_name("condition_producing_layer");
    l3->add_input("A");
    l3->add_output("cond");
    
    auto *l4 = nnMain->add_layers();
    auto *branch_layer = l4->mutable_branch();
    l4->set_name("branch_layer");
    l4->add_input("cond");
    branch_layer->mutable_ifbranch()->CopyFrom(nnIf);
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidBranchBlobOverwrite() {
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    // "If" net
    Specification::NeuralNetwork nnIf;
    auto *l1 = nnIf.add_layers();
    (void)l1->mutable_activation()->mutable_relu();
    l1->set_name("if_relu");
    l1->add_input("A");
    l1->add_output("cond");
    
    // "else" net
    Specification::NeuralNetwork nnElse;
    auto *l2 = nnElse.add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("else_relu");
    l2->add_input("A");
    l2->add_output("B");
    
    // Main network
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l3 = nnMain->add_layers();
    (void)l3->mutable_activation()->mutable_relu();
    l3->set_name("condition_producing_layer");
    l3->add_input("A");
    l3->add_output("cond");
    
    auto *l4 = nnMain->add_layers();
    auto *branch_layer = l4->mutable_branch();
    l4->set_name("branch_layer");
    l4->add_input("cond");
    branch_layer->mutable_ifbranch()->CopyFrom(nnIf);
    branch_layer->mutable_elsebranch()->CopyFrom(nnElse);
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidCopy() {
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    auto *nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l = nn->add_layers();
    (void)l->mutable_copy();
    l->set_name("copy");
    l->add_input("A");
    l->add_output("A");
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidLoop1() {
    /*
     no input, no condition network, 0 max loop
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    
    Specification::NeuralNetwork nnBody;
    auto *l1 = nnBody.add_layers();
    (void)l1->mutable_activation()->mutable_relu();
    l1->set_name("relu");
    l1->add_input("A");
    l1->add_output("B");
    
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l2 = nnMain->add_layers();
    l2->set_name("for_loop");
    auto *loop_params = l2->mutable_loop();
    loop_params->mutable_bodynetwork()->CopyFrom(nnBody);
    
    auto *l3 = nnMain->add_layers();
    l3->set_name("copy");
    l3->add_input("A");
    l3->add_output("B");
    (void) l3->mutable_copy();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidLoop2() {
    /*
     condition network present but no condition variable
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    Specification::NeuralNetwork nnCondition;
    auto *l1 = nnCondition.add_layers();
    l1->mutable_greaterthan()->set_alpha(1.0);
    l1->set_name("cond");
    l1->add_input("A");
    l1->add_output("cond");
    
    Specification::NeuralNetwork nnBody;
    auto *l2 = nnBody.add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("relu");
    l2->add_input("A");
    l2->add_output("B");
    
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l3 = nnMain->add_layers();
    l3->set_name("for_loop");
    auto *loop_params = l3->mutable_loop();
    loop_params->mutable_bodynetwork()->CopyFrom(nnBody);
    loop_params->mutable_conditionnetwork()->CopyFrom(nnCondition);
    
    auto *l4 = nnMain->add_layers();
    l4->set_name("copy");
    l4->add_input("A");
    l4->add_output("B");
    (void) l4->mutable_copy();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidLoop3() {
    /*
     condition variable present but no condition network
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    Specification::NeuralNetwork nnBody;
    
    auto *l2 = nnBody.add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("relu");
    l2->add_input("A");
    l2->add_output("B");
    
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l3 = nnMain->add_layers();
    l3->set_name("for_loop");
    auto *loop_params = l3->mutable_loop();
    loop_params->mutable_bodynetwork()->CopyFrom(nnBody);
    loop_params->set_conditionvar("cond");
    
    auto *l4 = nnMain->add_layers();
    l4->set_name("copy");
    l4->add_input("A");
    l4->add_output("B");
    (void) l4->mutable_copy();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidLoop4() {
    /*
     condition network present, condition variable present, but condition var not in condition network
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    Specification::NeuralNetwork nnCondition;
    auto *l1 = nnCondition.add_layers();
    l1->mutable_greaterthan()->set_alpha(1.0);
    l1->set_name("cond2");
    l1->add_input("A");
    l1->add_output("cond2");
    
    Specification::NeuralNetwork nnBody;
    auto *l2 = nnBody.add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("relu");
    l2->add_input("A");
    l2->add_output("B");
    
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    auto *l3 = nnMain->add_layers();
    l3->set_name("for_loop");
    auto *loop_params = l3->mutable_loop();
    loop_params->mutable_bodynetwork()->CopyFrom(nnBody);
    loop_params->mutable_conditionnetwork()->CopyFrom(nnCondition);
    loop_params->set_conditionvar("cond");
    
    auto *l4 = nnMain->add_layers();
    l4->set_name("copy");
    l4->add_input("A");
    l4->add_output("B");
    (void) l4->mutable_copy();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidLoop5() {
    /*
     output blob not generated outside the loop
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    Specification::NeuralNetwork nnCondition;
    auto *l1 = nnCondition.add_layers();
    l1->mutable_greaterthan()->set_alpha(1.0);
    l1->set_name("cond");
    l1->add_input("A");
    l1->add_output("cond");
    
    Specification::NeuralNetwork nnBody;
    auto *l2 = nnBody.add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("relu");
    l2->add_input("A");
    l2->add_output("B");
    
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    auto *l3 = nnMain->add_layers();
    l3->set_name("for_loop");
    auto *loop_params = l3->mutable_loop();
    loop_params->mutable_bodynetwork()->CopyFrom(nnBody);
    loop_params->mutable_conditionnetwork()->CopyFrom(nnCondition);
    loop_params->set_conditionvar("cond");
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidLoopBreak() {
    /*
     loop break layer not inside a loop
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    auto *l1 = nnMain->add_layers();
    l1->set_name("copy");
    l1->add_input("A");
    l1->add_output("B");
    (void) l1->mutable_copy();
    
    auto *l2 = nnMain->add_layers();
    l2->set_name("break");
    (void) l2->mutable_loopbreak();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidLoopContinue() {
    /*
     loop continue layer not inside a loop
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    auto *nnMain = m.mutable_neuralnetwork();
    nnMain->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    auto *l1 = nnMain->add_layers();
    l1->set_name("copy");
    l1->add_input("A");
    l1->add_output("B");
    (void) l1->mutable_copy();
    
    auto *l2 = nnMain->add_layers();
    l2->set_name("continue");
    (void) l2->mutable_loopcontinue();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidRankInconsistency() {
    /*
     A -> relu1 -> B -> relu2 -> C
     rank of B when output of relu1 : 1
     rank of B when input of relu2: 2 (makes the model invalid)
     */
    
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("C");
    out->mutable_type()->mutable_multiarraytype();
    
    auto *nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    auto *l = nn->add_layers();
    (void)l->mutable_activation()->mutable_relu();
    l->set_name("relu1");
    l->add_input("A");
    l->add_output("B");
    l->add_outputtensor()->set_rank(1);
    
    auto *l2 = nn->add_layers();
    (void)l2->mutable_activation()->mutable_relu();
    l2->set_name("relu2");
    l2->add_input("B");
    l2->add_output("C");
    l2->add_inputtensor()->set_rank(2);
    
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidExpandDims1() {
    /*
     A -> expand dims -> B
     shape of A: (2)
     shape of B: (2, 1, 1)
     axes parameter = [-1]
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(2);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    auto *shape_out = out->mutable_type()->mutable_multiarraytype();
    shape_out->add_shape(2);
    shape_out->add_shape(1);
    shape_out->add_shape(1);
    
    auto *nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    
    auto *l = nn->add_layers();
    l->set_name("ED");
    l->add_input("A");
    l->add_output("B");
    l->add_inputtensor()->set_rank(1);
    l->add_outputtensor()->set_rank(3);
    auto *params = l->mutable_expanddims();
    params->add_axes(-1);
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidExpandDims2() {
    /*
     A -> expand dims -> B
     shape of A: (2)
     shape of B: (2, 1, 1)
     axes parameter = [2,-4]
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(2);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    auto *shape_out = out->mutable_type()->mutable_multiarraytype();
    shape_out->add_shape(2);
    shape_out->add_shape(1);
    shape_out->add_shape(1);
    
    auto *nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l = nn->add_layers();
    l->set_name("ED");
    l->add_input("A");
    l->add_output("B");
    l->add_inputtensor()->set_rank(1);
    l->add_outputtensor()->set_rank(3);
    auto *params = l->mutable_expanddims();
    params->add_axes(2);
    params->add_axes(-4);
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidSqueeze1() {
    /*
     A -> expand dims -> B
     shape of A: (2, 1, 1)
     shape of B: (2)
     axes parameter = [1,1]
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(2);
    shape->add_shape(1);
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    auto *shape_out = out->mutable_type()->mutable_multiarraytype();
    shape_out->add_shape(2);
    
    auto *nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l = nn->add_layers();
    l->set_name("squeeze");
    l->add_input("A");
    l->add_output("B");
    l->add_inputtensor()->set_rank(3);
    l->add_outputtensor()->set_rank(1);
    auto *params = l->mutable_squeeze();
    params->add_axes(1);
    params->add_axes(1);
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidPoolingRank1() {
    /*
     A -> pooling -> B
     invalid since the rank of the input is 3, in the proto, whereas it should be at least 4.
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(2);
    shape->add_shape(1);
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    auto *nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l = nn->add_layers();
    l->set_name("pool_layer");
    l->add_input("A");
    l->add_output("B");
    l->add_inputtensor()->set_rank(3);
    auto *params = l->mutable_pooling();
    params->set_type(::Specification::PoolingLayerParams::AVERAGE);
    params->set_globalpooling(true);
    params->mutable_valid();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidPoolingRank2() {
    /*
     A -> pooling -> B
     invalid since the rank of the input and output is not same, in the proto
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    shape->add_shape(2);
    shape->add_shape(1);
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    auto *nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);
    auto *l = nn->add_layers();
    l->set_name("pool_layer");
    l->add_input("A");
    l->add_output("B");
    l->add_inputtensor()->set_rank(4);
    l->add_outputtensor()->set_rank(5);
    auto *params = l->mutable_pooling();
    params->set_type(::Specification::PoolingLayerParams::AVERAGE);
    params->set_globalpooling(true);
    params->mutable_valid();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidIOS13LayerOldRank() {
    /*
     a new layer, added in iOS 13 is used along with the old rank 5 mapping: not allowed.
     */
    Specification::Model m;
    auto *topIn = m.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    shape->add_shape(2);
    shape->add_shape(1);
    shape->add_shape(1);
    
    auto *out = m.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    auto *nn = m.mutable_neuralnetwork();
   
    auto *l = nn->add_layers();
    l->set_name("erf");
    l->add_input("A");
    l->add_output("B");
    l->mutable_erf();
    
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    return 0;
}

int testInvalidConstantPad() {

    // if padToGivenOutputSizeMode is True,
    // only one of 2*i-th and 2*i+1-th index can be non zero.

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(6);
    outShape->add_shape(5);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->set_name("constant_pad");
    layers->add_input("input");
    layers->add_output("output");

    auto *params = layers->mutable_constantpad();
    params->set_padtogivenoutputsizemode(true);
    params->add_padamounts(7);
    params->add_padamounts(6);
    params->add_padamounts(0);
    params->add_padamounts(0);

    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("padToGivenOutputSizeMode") != std::string::npos);

    return 0;
}

int testInvalidConcatNdWrongAxis() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(1);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    auto *params = layers->mutable_concatnd();
    params->set_axis(-4);

    // axis should be in range [-rank, rank)
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("axis") != std::string::npos);

    return 0;

}

int testInvalidSoftmaxNdWrongAxis() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(1);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    auto *params = layers->mutable_softmaxnd();
    params->set_axis(4);

    // axis should be in range [-rank, rank)
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("axis") != std::string::npos);

    return 0;

}

int testInvalidSlidingWindowWrongAxis() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    auto* params = layers->mutable_slidingwindows();
    params->set_axis(-5);

    // axis should be in range [-rank, rank)
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("axis") != std::string::npos);

    return 0;

}

int testInvalidFillStaticNoTargetShape() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    // missing required parameters
    layers->mutable_fillstatic();

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("Target shape") != std::string::npos);

    return 0;

}

int testInvalidBroadcastToStaticNoTargetShape() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    // missing required parameters
    layers->mutable_broadcasttostatic();

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("Target shape") != std::string::npos);

    return 0;

}

int testInvalidReverseWrongDimLength() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    auto* params = layers->mutable_reverse();
    params->add_reversedim(true);
    params->add_reversedim(true);

    // length of reverse_dim not euqual to input tensor rank
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("reverse_dim") != std::string::npos);

    return 0;

}

int testInvalidStackWrongAxis() {

    Specification::Model m;

    auto *in1 = m.mutable_description()->add_input();
    in1->set_name("input1");
    auto *inShape1 = in1->mutable_type()->mutable_multiarraytype();
    inShape1->add_shape(3);
    inShape1->add_shape(5);
    inShape1->add_shape(2);

    auto *in2 = m.mutable_description()->add_input();
    in2->set_name("input2");
    auto *inShape2 = in2->mutable_type()->mutable_multiarraytype();
    inShape2->add_shape(3);
    inShape2->add_shape(5);
    inShape2->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(1);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input1");
    layers->add_input("input2");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);
    layers->add_inputtensor()->set_rank(3);

    auto *params = layers->mutable_stack();
    params->set_axis(4);

    // axis should be in range [-(rank + 1), rank + 1)
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("axis") != std::string::npos);

    return 0;

}

int testInvalidSplitNdNoSplitSizesAndNumSplits() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    // missing required parameters
    layers->mutable_splitnd();

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("provided") != std::string::npos);

    return 0;

}

int testInvalidSplitNdWrongNumSplits() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    // missing required parameters
    auto *params = layers->mutable_splitnd();
    params->set_numsplits(5);

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("match") != std::string::npos);

    return 0;

}

int testInvalidSplitNdWrongAxis() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(1);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    auto *params = layers->mutable_splitnd();
    params->set_numsplits(2);
    params->set_axis(-5);

    // axis should be in range [-rank, rank)
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("axis") != std::string::npos);

    return 0;

}

int testInvalidSliceStaticNoParams() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    // missing required parameters

    auto* params = layers->mutable_slicestatic();
    params->add_endmasks(true);

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("Begin IDs") != std::string::npos);

    params->add_beginids(0);

    Result res2 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res2);
    ML_ASSERT(res2.message().find("End IDs") != std::string::npos);

    params->add_endids(5);

    Result res3 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res3);
    ML_ASSERT(res3.message().find("Strides") != std::string::npos);

    params->add_strides(1);

    Result res4 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res4);
    ML_ASSERT(res4.message().find("Begin masks") != std::string::npos);

    params->add_beginmasks(true);
    params->clear_endmasks();

    Result res5 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res5);
    ML_ASSERT(res5.message().find("End masks") != std::string::npos);

    return 0;

}

int testInvalidClipWrongMinMax() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    // value of minval should be smaller than value of maxval

    auto* params1 = layers->mutable_clip();
    params1->set_minval(1.2f);
    params1->set_maxval(0.4f);

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("smaller") != std::string::npos);

    return 0;

}

int testInvalidFlattenTo2dWrongAxis() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    auto* params = layers->mutable_flattento2d();
    params->set_axis(-5);

    // axis should be in range [-rank, rank)
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("axis") != std::string::npos);

    return 0;

}

int testInvalidReshapeStaticNoTargetShape() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    // missing required parameters
    layers->mutable_reshapestatic();

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("Target shape") != std::string::npos);

    return 0;

}

int testInvalidRandomUniformDistributionWrongMinMax() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    // value of minval should be smaller than value of maxval

    auto* params1 = layers->mutable_randomuniformlike();
    params1->set_minval(1.2f);
    params1->set_maxval(0.4f);

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("smaller") != std::string::npos);

    auto* params2 = layers->mutable_randomuniformstatic();
    params2->add_outputshape(3);
    params2->set_minval(1.2f);
    params2->set_maxval(0.4f);

    Result res2 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res2);
    ML_ASSERT(res2.message().find("smaller") != std::string::npos);

    auto* params3 = layers->mutable_randomuniformdynamic();
    params3->set_minval(1.2f);
    params3->set_maxval(0.4f);

    Result res3 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res3);
    ML_ASSERT(res3.message().find("smaller") != std::string::npos);

    return 0;
}

int testInvalidRandomBernoulliDistributionWrongProb() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    // value of prob should be in range [0.0, 1.0]

    auto* params1 = layers->mutable_randombernoullilike();
    params1->set_prob(1.0001f);

    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("prob") != std::string::npos);

    auto* params2 = layers->mutable_randombernoullistatic();
    params2->add_outputshape(3);
    params2->set_prob(-2037.63f);

    Result res2 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res2);
    ML_ASSERT(res2.message().find("prob") != std::string::npos);

    auto* params3 = layers->mutable_randombernoullidynamic();
    params3->set_prob(1024.2f);

    Result res3 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res3);
    ML_ASSERT(res3.message().find("prob") != std::string::npos);

    return 0;
}

int testInvalidReductionTypeWrongAxis() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");
    layers->add_inputtensor()->set_rank(3);

    auto* params = layers->mutable_reducemean();
    params->set_reduceall(false);
    params->add_axes(-5);

    // axis should be in range [-rank, rank)
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("axis") != std::string::npos);

    return 0;

}

int testInvalidLayerNormalizationNoNormalizedShape() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    auto* params = layers->mutable_layernormalization();
    params->mutable_gamma()->add_floatvalue(1.0);
    params->mutable_beta()->add_floatvalue(0.0);

    // not specifying the value for normalized shape
    Result res = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res);
    ML_ASSERT(res.message().find("Normalized shape") != std::string::npos);

    return 0;

}

int testInvalidLayerNormalizationNoGammaOrBeta() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    auto* params = layers->mutable_layernormalization();
    params->add_normalizedshape(1);

    // not specifying the gamma parameter
    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("Gamma") != std::string::npos);

    // not specifying the beta parameter
    params->mutable_gamma()->add_floatvalue(1.0);

    Result res2 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res2);
    ML_ASSERT(res2.message().find("Beta") != std::string::npos);

    return 0;

}

int testInvalidLayerNormalizationWrongGammaOrBeta() {

    Specification::Model m;

    auto *in = m.mutable_description()->add_input();
    in->set_name("input");
    auto *inShape = in->mutable_type()->mutable_multiarraytype();
    inShape->add_shape(3);
    inShape->add_shape(5);
    inShape->add_shape(2);

    auto *out = m.mutable_description()->add_output();
    out->set_name("output");
    auto *outShape = out->mutable_type()->mutable_multiarraytype();
    outShape->add_shape(3);
    outShape->add_shape(5);
    outShape->add_shape(2);

    const auto nn = m.mutable_neuralnetwork();
    nn->set_arrayinputshapemapping(Specification::NeuralNetworkMultiArrayShapeMapping::EXACT_ARRAY_MAPPING);

    auto *layers = nn->add_layers();
    layers->add_input("input");
    layers->add_output("output");

    auto* params = layers->mutable_layernormalization();
    params->add_normalizedshape(1);

    params->mutable_gamma()->add_floatvalue(1.0);
    params->mutable_beta()->add_floatvalue(1.0);
    params->mutable_gamma()->mutable_quantization();

    // value of gamma and beta should be unquantized
    Result res1 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res1);
    ML_ASSERT(res1.message().find("quantized") != std::string::npos);

    params->mutable_gamma()->clear_floatvalue();
    params->mutable_gamma()->add_floatvalue(1.0);
    params->mutable_beta()->add_floatvalue(1.0);
    params->mutable_beta()->mutable_quantization();

    Result res2 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res2);
    ML_ASSERT(res2.message().find("quantized") != std::string::npos);

    // shape of gamma and beta should be match normalized shape

    params->mutable_gamma()->clear_quantization();
    params->mutable_beta()->clear_quantization();
    params->mutable_gamma()->add_floatvalue(1.0);

    Result res3 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res3);
    ML_ASSERT(res3.message().find("Shape of gamma") != std::string::npos);

    params->mutable_gamma()->clear_floatvalue();
    params->mutable_gamma()->add_floatvalue(1.0);
    params->mutable_beta()->add_floatvalue(1.0);
    params->mutable_beta()->add_floatvalue(1.0);

    Result res4 = validate<MLModelType_neuralNetwork>(m);
    ML_ASSERT_BAD(res4);
    ML_ASSERT(res4.message().find("Shape of beta") != std::string::npos);

    return 0;
}


#pragma clang diagnostic pop
