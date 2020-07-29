//
//  VisionFeaturePrintValidatorTests.cpp
//  CoreML_framework
//
//  Created by Tao Jia on 3/20/20.
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

int testVisionFeatureScenePrintBasic() {

    Specification::ImageFeatureType* inputImageFeatureType = new Specification::ImageFeatureType();
    Specification::FeatureType* inputFeatureType = new Specification::FeatureType();
    inputFeatureType->set_allocated_imagetype(inputImageFeatureType);

    Specification::ArrayFeatureType* outputArrayFeatureType = new Specification::ArrayFeatureType();
    Specification::FeatureType* outputFeatureType = new Specification::FeatureType();
    outputFeatureType->set_allocated_multiarraytype(outputArrayFeatureType);

    Specification::ModelDescription* description = new Specification::ModelDescription();
    Specification::FeatureDescription* input = description->add_input();
    Specification::FeatureDescription* output = description->add_output();
    input->set_allocated_type(inputFeatureType);
    output->set_allocated_type(outputFeatureType);

    Specification::Model model;
    model.set_allocated_description(description);

    Result result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);

    auto *preprocessing = model.mutable_visionfeatureprint();
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);

    preprocessing->mutable_scene();
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);

    preprocessing->mutable_scene()->set_version(Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion_SCENE_VERSION_1);
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_GOOD(result);
    
    return 0;
}

int testVisionFeatureObjectPrintBasic() {

    Specification::ImageFeatureType* inputImageFeatureType = new Specification::ImageFeatureType();
    Specification::FeatureType* inputFeatureType = new Specification::FeatureType();
    inputFeatureType->set_allocated_imagetype(inputImageFeatureType);

    Specification::ArrayFeatureType* output1ArrayFeatureType = new Specification::ArrayFeatureType();
    Specification::FeatureType* output1FeatureType = new Specification::FeatureType();
    output1FeatureType->set_allocated_multiarraytype(output1ArrayFeatureType);
    
    Specification::ArrayFeatureType* output2ArrayFeatureType = new Specification::ArrayFeatureType();
    Specification::FeatureType* output2FeatureType = new Specification::FeatureType();
    output2FeatureType->set_allocated_multiarraytype(output2ArrayFeatureType);
    
    Specification::ModelDescription* description = new Specification::ModelDescription();
    Specification::FeatureDescription* input = description->add_input();
    Specification::FeatureDescription* output1 = description->add_output();
    Specification::FeatureDescription* output2 = description->add_output();
    input->set_allocated_type(inputFeatureType);
    output1->set_allocated_type(output1FeatureType);
    output2->set_allocated_type(output2FeatureType);

    output1->set_name("a");
    output2->set_name("b");

    Specification::Model model;
    model.set_allocated_description(description);

    Result result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);

    auto *preprocessing = model.mutable_visionfeatureprint();
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);

    auto objects = preprocessing->mutable_objects();
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);

    objects->set_version(Specification::CoreMLModels::VisionFeaturePrint_Objects_ObjectsVersion_OBJECTS_VERSION_1);
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);

    objects->add_output("a");
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_BAD(result);
    objects->add_output("b");
    result = validate<MLModelType_visionFeaturePrint>(model);
    ML_ASSERT_GOOD(result);

    return 0;
}

