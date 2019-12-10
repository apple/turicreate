//
//  UpdatableModelValidatorTests.cpp
//  CoreML_framework
//
//  Created by aseem wadhwa on 2/12/19.
//  Copyright © 2019 Apple Inc. All rights reserved.
//

#include "MLModelTests.hpp"
#include "../src/Format.hpp"
#include "../src/Model.hpp"
#include "ParameterTests.hpp"

#include "framework/TestUtils.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"

using namespace CoreML;

int testVggishPreprocessingBasic() {
    
    Specification::ArrayFeatureType* inputArrayFeatureType = new Specification::ArrayFeatureType();
    inputArrayFeatureType->add_shape(15600);
    
    Specification::FeatureType* inputFeatureType = new Specification::FeatureType();
    inputFeatureType->set_allocated_multiarraytype(inputArrayFeatureType);
    
    Specification::ArrayFeatureType* outputArrayFeatureType = new Specification::ArrayFeatureType();
    outputArrayFeatureType->add_shape(1);
    outputArrayFeatureType->add_shape(96);
    outputArrayFeatureType->add_shape(64);
    
    Specification::FeatureType* outputFeatureType = new Specification::FeatureType();
    outputFeatureType->set_allocated_multiarraytype(outputArrayFeatureType);
    
    Specification::ModelDescription* description = new Specification::ModelDescription();
    Specification::FeatureDescription* input = description->add_input();
    Specification::FeatureDescription* output = description->add_output();
    input->set_allocated_type(inputFeatureType);
    output->set_allocated_type(outputFeatureType);
    
    Specification::Model model;
    model.set_allocated_description(description);
    
    Result result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);
    
    auto *preprocessing = model.mutable_soundanalysispreprocessing();
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);
    
    preprocessing->mutable_vggish();
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);

    inputArrayFeatureType->set_shape(0, 15599);
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);
    
    inputArrayFeatureType->set_shape(0, 15600);
    
    inputArrayFeatureType->set_datatype(Specification::ArrayFeatureType_ArrayDataType_FLOAT32);
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);

    outputArrayFeatureType->set_shape(1, 95);
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);

    // restore
    outputArrayFeatureType->set_shape(1, 96);
    
    outputArrayFeatureType->set_shape(2, 65);
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);
    
    // restore
    outputArrayFeatureType->set_shape(2, 64);
    
    outputArrayFeatureType->clear_shape();
    outputArrayFeatureType->add_shape(96);
    outputArrayFeatureType->add_shape(64);
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_BAD(result);
    
    outputArrayFeatureType->clear_shape();
    outputArrayFeatureType->add_shape(1);
    outputArrayFeatureType->add_shape(96);
    outputArrayFeatureType->add_shape(64);
    
    outputArrayFeatureType->set_datatype(Specification::ArrayFeatureType_ArrayDataType_FLOAT32);
    result = validate<MLModelType_soundAnalysisPreprocessing>(model);
    ML_ASSERT_GOOD(result);
    
    return 0;
}

