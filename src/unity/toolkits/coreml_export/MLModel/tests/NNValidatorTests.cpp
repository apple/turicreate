/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
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

#include "framework/TestUtils.hpp"

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
    
    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    innerProductLayer->add_input("input");
    innerProductLayer->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();

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
    
    const auto nn = m1.mutable_neuralnetwork();
    
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
    
    const auto nn = m1.mutable_neuralnetwork();
    
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
    
    const auto nn = m1.mutable_neuralnetwork();
    
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
    
    const auto nn = m1.mutable_neuralnetwork();
    
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
    
    const auto nn = m1.mutable_neuralnetwork();
    
    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    
    innerProductParams->set_hasbias(false);
    
    ML_ASSERT_BAD(validate<MLModelType_neuralNetwork>(m1));
    
    return 0;
}


int testNNValidatorMissingInput() {

    Specification::Model m1;
    
    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("E");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *out = m1.mutable_description()->add_output();
    out->set_name("D");
    out->mutable_type()->mutable_multiarraytype();
    
    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *ip1 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams1 = ip1->mutable_innerproduct();
    ip1->set_name("ip1");

    innerProductParams1->set_hasbias(false);

    Specification::NeuralNetworkLayer *ip2 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams2 = ip2->mutable_innerproduct();

    innerProductParams2->set_hasbias(false);
    
    ip2->set_name("ip2");

    Specification::NeuralNetworkLayer *ip3 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams3 = ip1->mutable_innerproduct();

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
    
    const auto nn = m1.mutable_neuralnetwork();
    
    Specification::NeuralNetworkLayer *ip1 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams1 = ip1->mutable_innerproduct();
    ip1->set_name("ip1");
    
    innerProductParams1->set_hasbias(false);
    
    Specification::NeuralNetworkLayer *ip2 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams2 = ip2->mutable_innerproduct();
    
    innerProductParams2->set_hasbias(false);
    
    ip2->set_name("ip2");
    
    Specification::NeuralNetworkLayer *ip3 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams3 = ip1->mutable_innerproduct();
    
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
    auto *out = m1.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    const auto nn = m1.mutable_neuralnetwork();

    Specification::NeuralNetworkLayer *ip1 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams1 = ip1->mutable_innerproduct();
    ip1->set_name("ip1");

    innerProductParams1->set_hasbias(false);
    
    Specification::NeuralNetworkLayer *ip2 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams2 = ip2->mutable_innerproduct();

    innerProductParams2->set_hasbias(false);
    
    ip2->set_name("ip2");

    Specification::NeuralNetworkLayer *ip3 = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams3 = ip1->mutable_innerproduct();

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
    
    const auto nn = m1.mutable_neuralnetwork();
    
    Specification::NeuralNetworkLayer *innerProductLayer = nn->add_layers();
    Specification::InnerProductLayerParams *innerProductParams = innerProductLayer->mutable_innerproduct();
    
    innerProductParams->set_hasbias(false);
    
    ML_ASSERT_BAD(validate<MLModelType_neuralNetwork>(m1));
    
    return 0;
}

int testRNNLayer() {
    
    Specification::Model m1;
    
    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("A");
    topIn->mutable_type()->mutable_multiarraytype();
    auto *out = m1.mutable_description()->add_output();
    out->set_name("B");
    out->mutable_type()->mutable_multiarraytype();
    
    auto nn = m1.mutable_neuralnetwork();
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
    auto *out = m1.mutable_description()->add_output();
    out->set_name("output");
    out->mutable_type()->mutable_multiarraytype();
    
    auto nn = m1.mutable_neuralnetwork();
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
    
    const auto nn = m1.mutable_neuralnetwork();
    
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
    
    const auto nn = m1.mutable_neuralnetwork();
    
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
    
    const auto nn = m1.mutable_neuralnetwork();
    
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
    
    reshapeParams->set_mode(::CoreML::Specification::ReshapeLayerParams_ReshapeOrder::ReshapeLayerParams_ReshapeOrder_CHANNEL_FIRST); Result res = validate<MLModelType_neuralNetwork>(m1);
    ML_ASSERT_BAD(res);
    
    return 0;
}

int testNNCompilerValidation() {
    
    Specification::Model m1;
    
    auto *topIn = m1.mutable_description()->add_input();
    topIn->set_name("input");
    auto *shape = topIn->mutable_type()->mutable_multiarraytype();
    shape->add_shape(1);
    
    auto *out = m1.mutable_description()->add_output();
    out->set_name("middle");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);
    
    auto *out2 = m1.mutable_description()->add_output();
    out2->set_name("features");
    out2->mutable_type()->mutable_stringtype();
    
    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_dictionarytype();
    
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
    innerProductParams->set_hasbias(false);
    
    Specification::NeuralNetworkLayer *innerProductLayer2 = nn->add_layers();
    innerProductLayer2->add_input("middle");
    innerProductLayer2->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams2 = innerProductLayer2->mutable_innerproduct();
    innerProductParams2->set_hasbias(false);
    
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
    
    auto *out = m1.mutable_description()->add_output();
    out->set_name("middle");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);
    
    auto *out2 = m1.mutable_description()->add_output();
    out2->set_name("features");
    out2->mutable_type()->mutable_stringtype();
    
    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_dictionarytype();
    
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
    innerProductParams->set_hasbias(false);
    
    Specification::NeuralNetworkLayer *innerProductLayer2 = nn->add_layers();
    innerProductLayer2->add_input("middle");
    innerProductLayer2->add_output("output");
    Specification::InnerProductLayerParams *innerProductParams2 = innerProductLayer2->mutable_innerproduct();
    innerProductParams2->set_hasbias(false);
    
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
    
    auto *out = m1.mutable_description()->add_output();
    out->set_name("middle");
    auto *outshape = out->mutable_type()->mutable_multiarraytype();
    outshape->add_shape(1);
    
    auto *out2 = m1.mutable_description()->add_output();
    out2->set_name("features");
    out2->mutable_type()->mutable_stringtype();
    
    auto *out3 = m1.mutable_description()->add_output();
    out3->set_name("probs");
    out3->mutable_type()->mutable_dictionarytype();
    
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

