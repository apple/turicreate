#include "MLModelTests.hpp"
#include "../src/Model.hpp"
#include "../src/Format.hpp"

#include "framework/TestUtils.hpp"

using namespace CoreML;

static Specification::Model& addRequiredField(Specification::Model& model, const std::string& name) {
    auto *input = model.mutable_description()->add_input();
    input->set_name(name);
    input->mutable_type()->mutable_int64type();
    return model;
}

static Specification::Model& addOptionalField(Specification::Model& model, const std::string& name) {
    auto *input = model.mutable_description()->add_input();
    input->set_name(name);
    input->mutable_type()->mutable_int64type();
    input->mutable_type()->set_isoptional(true);
    return model;
}

int testOptionalInputs() {
    // Test that all fields are required on a random model (normalizer)
    Specification::Model m1;
    m1.mutable_normalizer()->set_normtype(Specification::Normalizer_NormType::Normalizer_NormType_L2);
    addRequiredField(m1, "x");
    ML_ASSERT_GOOD(validateOptional(m1));

    addOptionalField(m1, "y");
    ML_ASSERT_BAD(validateOptional(m1));

    // Test that at least one optional field is required on an imputer
    // (more than one allowed)
    Specification::Model m2;
    m2.mutable_imputer()->set_imputeddoublevalue(3.14);
    addRequiredField(m2, "x");
    addOptionalField(m2, "y");
    ML_ASSERT_GOOD(validateOptional(m2));

    addOptionalField(m2, "z");
    ML_ASSERT_GOOD(validateOptional(m2));

    // Test that any fields can be optional or required for trees
    Specification::Model m3;
    (void) m3.mutable_treeensembleregressor();
    addRequiredField(m3, "x");
    ML_ASSERT_GOOD(validateOptional(m3));

    Specification::Model m4;
    (void) m4.mutable_treeensembleregressor();
    addOptionalField(m4, "x");
    ML_ASSERT_GOOD(validateOptional(m4));

    return 0;
}


int testFeatureDescriptions() {

    Specification::Model m;

    auto *feature = m.mutable_description()->add_input();
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));

    // Just with a name as invalid
    feature->set_name("test_input");
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));

    // Empty type, still invalid
    feature->mutable_type();
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));

    // Int64 typ, now its valid
    feature->mutable_type()->mutable_int64type();
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));

    // String type, valid
    feature->mutable_type()->mutable_stringtype();
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));

    // Double type, valid
    feature->mutable_type()->mutable_doubletype();
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));

    // Multiarray type, with no params, invalid
    feature->mutable_type()->mutable_multiarraytype();
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));

    // Multiarray type, double with no shape, invalid as input, valid as output
    feature->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));
    feature->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_DOUBLE);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, false));

    feature->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));
    feature->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, false));

    feature->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_INT32);
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));
    feature->mutable_type()->mutable_multiarraytype()->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_INT32);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, false));

    // Zero length shape is invalid for inputs, but valid for outputs
    feature->mutable_type()->mutable_multiarraytype()->mutable_shape();
    ML_ASSERT_BAD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, false));

    // Non-zero length shape, valid
    feature->mutable_type()->mutable_multiarraytype()->mutable_shape()->Add(128);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, true));
    ML_ASSERT_GOOD(validateFeatureDescription(*feature, MLMODEL_SPECIFICATION_VERSION_IOS11_2, false));

    // Dictionary, with no params, invalid
    feature->mutable_type()->mutable_dictionarytype();
    ML_ASSERT_BAD(validateFeatureDescription(*feature,true));

    // With key type, valid
    feature->mutable_type()->mutable_dictionarytype()->mutable_stringkeytype();
    ML_ASSERT_GOOD(validateFeatureDescription(*feature,true));

    feature->mutable_type()->mutable_dictionarytype()->mutable_int64keytype();
    ML_ASSERT_GOOD(validateFeatureDescription(*feature,true));

    // Image, with no params, invalid
    feature->mutable_type()->mutable_imagetype();
    ML_ASSERT_BAD(validateFeatureDescription(*feature,true));

    // With just width, invalid
    feature->mutable_type()->mutable_imagetype()->set_width(10);
    ML_ASSERT_BAD(validateFeatureDescription(*feature,true));

    // With both width and height, still invalid because no colorspace
    feature->mutable_type()->mutable_imagetype()->set_height(20);
    ML_ASSERT_BAD(validateFeatureDescription(*feature,true));

    // Now with colorspace, valid
    feature->mutable_type()->mutable_imagetype()->set_colorspace(::CoreML::Specification::ImageFeatureType_ColorSpace_BGR);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature,true));
    feature->mutable_type()->mutable_imagetype()->set_colorspace(::CoreML::Specification::ImageFeatureType_ColorSpace_RGB);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature,true));
    feature->mutable_type()->mutable_imagetype()->set_colorspace(::CoreML::Specification::ImageFeatureType_ColorSpace_GRAYSCALE);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature,true));
    feature->mutable_type()->mutable_imagetype()->set_colorspace(::CoreML::Specification::ImageFeatureType_ColorSpace_INVALID_COLOR_SPACE);
    ML_ASSERT_BAD(validateFeatureDescription(*feature,true));

    //////////////////////////////////
    // Test more recent shape constraints
    Specification::Model m2;

    auto *feature2 = m2.mutable_description()->add_input();
    feature2->set_name("feature2");
    feature2->mutable_type()->mutable_imagetype()->set_colorspace(::CoreML::Specification::ImageFeatureType_ColorSpace_BGR);

    /// Fixed Size
    // Make fixed size  6 x 5
    feature2->mutable_type()->mutable_imagetype()->set_width(6);
    feature2->mutable_type()->mutable_imagetype()->set_height(5);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    /// Enumerated
    // Add flexibility of a single enumerated size 6 x 5
    auto *shape = feature2->mutable_type()->mutable_imagetype()->mutable_enumeratedsizes()->add_sizes();
    shape->set_width(6);
    shape->set_height(5);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Reset that to a single 10 x 5 which would make the 6 x 5 invalid!
    shape->set_width(10);
    shape->set_height(5);
    ML_ASSERT_BAD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Add 6 x 5 to the list so its now [10x5, 6 x 5] which should make it valid again
    shape = feature2->mutable_type()->mutable_imagetype()->mutable_enumeratedsizes()->add_sizes();
    shape->set_width(6);
    shape->set_height(5);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    /// Range
    // Now make it a range that inclues 6 x 5
    auto* size_range = feature2->mutable_type()->mutable_imagetype()->mutable_imagesizerange();
    size_range->mutable_widthrange()->set_lowerbound(1);
    size_range->mutable_widthrange()->set_upperbound(-1); // unbounded

    size_range->mutable_heightrange()->set_lowerbound(2);
    size_range->mutable_heightrange()->set_upperbound(5);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Now make the range not include 6 x 5
    size_range->mutable_widthrange()->set_lowerbound(7);
    ML_ASSERT_BAD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Fix it to include it again
    size_range->mutable_widthrange()->set_lowerbound(2);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Fail due to upper bound can't be larger than lower
    size_range->mutable_widthrange()->set_upperbound(1);
    ML_ASSERT_BAD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));


    /////////////

    auto *array_type = feature2->mutable_type()->mutable_multiarraytype();
    array_type->set_datatype(::CoreML::Specification::ArrayFeatureType_ArrayDataType_FLOAT32);

    // 10 x 5 default size
    array_type->add_shape(10);
    array_type->add_shape(5);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Range
    // Now specify ranges (>1 x [5...20])
    auto rangeForDim0 = array_type->mutable_shaperange()->add_sizeranges();
    rangeForDim0->set_lowerbound(1);
    rangeForDim0->set_upperbound(-1);

    auto rangeForDim1 = array_type->mutable_shaperange()->add_sizeranges();
    rangeForDim1->set_lowerbound(5);
    rangeForDim1->set_upperbound(20);
    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Change to (>1 x [6..20]) which is not consistent with 10 x 5
    rangeForDim1->set_lowerbound(6);
    ML_ASSERT_BAD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Enumerated
    auto eshape1 = array_type->mutable_enumeratedshapes()->add_shapes();
    eshape1->add_shape(6);
    eshape1->add_shape(2);

    // Now allow [ 6x2 ] which is inconsistent with default 10 x 5
    ML_ASSERT_BAD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    // Add another one to make the set [6x2 , 10x5] which is consistent
    auto eshape2 = array_type->mutable_enumeratedshapes()->add_shapes();
    eshape2->add_shape(10);
    eshape2->add_shape(5);

    ML_ASSERT_GOOD(validateFeatureDescription(*feature2, MLMODEL_SPECIFICATION_VERSION, true));

    return 0;
}
