#include "MLModelTests.hpp"
#include "../src/Model.hpp"
#include "../src/Format.hpp"

#include "framework/TestUtils.hpp"

using namespace CoreML;

int testSpecDowngradePipeline() {

    int32_t latestVersion = MLMODEL_SPECIFICATION_VERSION;

    Specification::Model spec;
    spec.set_specificationversion(latestVersion);
    auto* pipeline = spec.mutable_pipelineclassifier()->mutable_pipeline();

    auto* input = spec.mutable_description()->add_input();
    input->set_name("image");
    input->mutable_type()->mutable_imagetype()->set_width(299);
    input->mutable_type()->mutable_imagetype()->set_height(299);
    input->mutable_type()->mutable_imagetype()->set_colorspace(Specification::ImageFeatureType_ColorSpace_BGR);

    auto* output = spec.mutable_description()->add_output();
    output->set_name("classLabel");
    output->mutable_type()->mutable_stringtype();

    spec.mutable_description()->set_predictedfeaturename("classLabel");

    // VisionFeaturePrint
    auto *featureModel = pipeline->add_models();
    featureModel->set_specificationversion(latestVersion);

    auto* featureInput = featureModel->mutable_description()->add_input();
    featureInput->set_name("image");
    featureInput->mutable_type()->mutable_imagetype()->set_width(299);
    featureInput->mutable_type()->mutable_imagetype()->set_height(299);
    featureInput->mutable_type()->mutable_imagetype()->set_colorspace(Specification::ImageFeatureType_ColorSpace_BGR);

    auto featureOutput = featureModel->mutable_description()->add_output();
    featureOutput->set_name("features");
    featureOutput->mutable_type()->mutable_multiarraytype()->set_datatype(Specification::ArrayFeatureType::FLOAT32);
    featureOutput->mutable_type()->mutable_multiarraytype()->add_shape(2048);


    featureModel->mutable_visionfeatureprint()->mutable_scene()->set_version(Specification::CoreMLModels::VisionFeaturePrint_Scene_SceneVersion_SCENE_VERSION_1);

    // Logistic regression
    auto *classifierModel = pipeline->add_models();
    classifierModel->set_specificationversion(latestVersion);

    auto classifierInput = classifierModel->mutable_description()->add_input();
    classifierInput->set_name("features");
    classifierInput->mutable_type()->mutable_multiarraytype()->set_datatype(Specification::ArrayFeatureType::FLOAT32);
    classifierInput->mutable_type()->mutable_multiarraytype()->add_shape(2048);

    auto classifierOutput = classifierModel->mutable_description()->add_output();
    classifierOutput->set_name("classLabel");
    classifierOutput->mutable_type()->mutable_stringtype();
    classifierModel->mutable_description()->set_predictedfeaturename("classLabel");

    auto glm = classifierModel->mutable_glmclassifier();
    glm->set_postevaluationtransform(::CoreML::Specification::GLMClassifier_PostEvaluationTransform_Logit);
    glm->add_offset(0.0);
    auto weights = glm->add_weights();
    for (int i=0; i<2048; i++) { weights->add_value(0.0); }
    glm->mutable_stringclasslabels()->add_vector("cat");
    glm->mutable_stringclasslabels()->add_vector("dog");

    // Constructing an CoreML::Model should downgrade spec on load
    Model model(spec);

    // Top level should be IOS 12 because it contains vision feature print
    ML_ASSERT_EQ(model.getProto().specificationversion(), MLMODEL_SPECIFICATION_VERSION_IOS12);

    // First model in pipeline is vision feature print and should have IOS12 spec version
    ML_ASSERT_EQ(model.getProto().pipelineclassifier().pipeline().models(0).specificationversion(), MLMODEL_SPECIFICATION_VERSION_IOS12);

    // Second model is just a GLM which has support in IOS 11
    ML_ASSERT_EQ(model.getProto().pipelineclassifier().pipeline().models(1).specificationversion(), MLMODEL_SPECIFICATION_VERSION_IOS11);

    return 0;
}
