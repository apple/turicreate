/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "MLModelTests.hpp"
#include "../src/Model.hpp"

#include "framework/TestUtils.hpp"
#include <cassert>
#include <cstdio>

using namespace CoreML;

int testBayesianProbitRegressionValidationBasic () {

    Specification::ArrayFeatureType* inputArrayFeatureType = new Specification::ArrayFeatureType();
    inputArrayFeatureType->add_shape(10);
    inputArrayFeatureType->add_shape(10);

    Specification::FeatureType* inputFeatureType = new Specification::FeatureType();
    inputFeatureType->set_allocated_multiarraytype(inputArrayFeatureType);

    Specification::ArrayFeatureType* outputArrayFeatureType = new Specification::ArrayFeatureType();
    outputArrayFeatureType->add_shape(10);

    Specification::FeatureType* outputFeatureType = new Specification::FeatureType();
    outputFeatureType->set_allocated_multiarraytype(outputArrayFeatureType);

    Specification::ModelDescription* description = new Specification::ModelDescription();
    Specification::FeatureDescription* input = description->add_input();
    Specification::FeatureDescription* output = description->add_output();
    input->set_allocated_type(inputFeatureType);
    output->set_allocated_type(outputFeatureType);

    Specification::Model model;
    model.set_allocated_description(description);

    Result result = validate<MLModelType_bayesianProbitRegressor>(model);
    ML_ASSERT_BAD(result);

    inputArrayFeatureType->set_datatype(Specification::ArrayFeatureType_ArrayDataType_INT32);

    result = validate<MLModelType_bayesianProbitRegressor>(model);
    ML_ASSERT_GOOD(result);

    return 0;
}
