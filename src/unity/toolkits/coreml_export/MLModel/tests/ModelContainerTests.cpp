#include "MLModelTests.hpp"

// TODO -- Fix these headers.
#include "../src/Model.hpp"
#include "../src/Utils.hpp"

#include "framework/TestUtils.hpp"

#include <cassert>
#include <cstdio>
#include <sys/stat.h>

using namespace CoreML;

// uncomment this to test a large model (>64 MB)
// it will take about 30 seconds
int testLargeModel() {
    /*
    const size_t numModels = 1000000;

    Specification::Model model;
    model.set_specificationversion(MLMODEL_SPECIFICATION_VERSION);
    auto* pipeline = model.mutable_pipeline();

    auto* input = model.mutable_description()->add_input();
    input->set_name(std::to_string(0));
    input->set_shortdescription("pipeline input description");
    auto* inputType = input->mutable_type();
    inputType->mutable_int64type();

    auto* output = model.mutable_description()->add_output();
    output->set_name(std::to_string(numModels));
    output->set_shortdescription("pipeline output description");
    auto* outputType = output->mutable_type();
    outputType->mutable_int64type();

    for (size_t i=0; i<numModels; i++) {
        auto* model = pipeline->add_models();
        (void) model->mutable_pipeline();
        model->set_specificationversion(MLMODEL_SPECIFICATION_VERSION);
        input = model->mutable_description()->add_input();
        input->set_name(std::to_string(i));
        input->set_shortdescription("model " + std::to_string(i) + " input");
        inputType = input->mutable_type();
        inputType->mutable_int64type();
        output = model->mutable_description()->add_output();
        output->set_name(std::to_string(i+1));
        output->set_shortdescription("model " + std::to_string(i) + " output");
        outputType = output->mutable_type();
        outputType->mutable_int64type();
    }

    // TODO don't write to /tmp
    const char *path = "/tmp/largeModel.mlmodel";
    struct stat result;
    ML_ASSERT_GOOD(saveSpecificationPath(model, path));
    ML_ASSERT_EQ(stat(path, &result), 0);

    // > 64 MB, < 128 MB is larger than protobuf normally supports,
    // but small enough to unit test fairly quickly
    ML_ASSERT_GT(result.st_size, 64 * 1024 * 1024);
    ML_ASSERT_LT(result.st_size, 128 * 1024 * 1024);

    // make sure we can read it back in
    Specification::Model model2;
    ML_ASSERT_GOOD(loadSpecificationPath(model2, path));
    */

    return 0;
}

// uncomment this to test a VERY large model (>1.5GB)
// it will take a VERY long time.
int testVeryLargeModel() {
    /*
    const size_t numModels = 20000000;

    Specification::Model model;
    model.set_specificationversion(MLMODEL_SPECIFICATION_VERSION);
    auto* pipeline = model.mutable_pipeline();

    auto* input = model.mutable_description()->add_input();
    input->set_name(std::to_string(0));
    input->set_shortdescription("pipeline input description");
    auto* inputType = input->mutable_type();
    inputType->mutable_int64type();

    auto* output = model.mutable_description()->add_output();
    output->set_name(std::to_string(numModels));
    output->set_shortdescription("pipeline output description");
    auto* outputType = output->mutable_type();
    outputType->mutable_int64type();

    for (size_t i=0; i<numModels; i++) {
        auto* model = pipeline->add_models();
        (void) model->mutable_pipeline();
        model->set_specificationversion(MLMODEL_SPECIFICATION_VERSION);
        input = model->mutable_description()->add_input();
        input->set_name(std::to_string(i));
        input->set_shortdescription("model " + std::to_string(i) + " input");
        inputType = input->mutable_type();
        inputType->mutable_int64type();
        output = model->mutable_description()->add_output();
        output->set_name(std::to_string(i+1));
        output->set_shortdescription("model " + std::to_string(i) + " output");
        outputType = output->mutable_type();
        outputType->mutable_int64type();
    }

    // TODO don't write to /tmp
    const char *path = "/tmp/veryLargeModel.mlmodel";
    struct stat result;
    ML_ASSERT_GOOD(saveSpecificationPath(model, path));
    ML_ASSERT_EQ(stat(path, &result), 0);

    // > 1 GB, < 2 GB is a good test case
    ML_ASSERT_GT(result.st_size, 1024 * 1024 * 1024);
    ML_ASSERT_LT(result.st_size, (2048 * 1024 * 1024)-1);

    // make sure we can read it back in
    Specification::Model model2;
    ML_ASSERT_GOOD(loadSpecificationPath(model2, path));
    */

    return 0;
}
