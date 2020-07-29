#include "MLModelTests.hpp"

#include "Model.hpp"
#include "transforms/OneHotEncoder.hpp"
#include "transforms/LinearModel.hpp"
#include "transforms/TreeEnsemble.hpp"


#include "framework/TestUtils.hpp"
#include <cassert>
#include <cstdio>

using namespace CoreML;

int testBasicSaveLoad () {
    Result r;
    
    const std::string path("/tmp/a.modelasset");
    OneHotEncoder ohe;
    ML_ASSERT_GOOD(ohe.addInput("foo", FeatureType::String()));
    ohe.getProto().mutable_onehotencoder()->mutable_stringcategories()->add_vector()->assign("foo");
    ML_ASSERT_GOOD(ohe.addOutput("bar", FeatureType::Array({})));
    ML_ASSERT_GOOD(ohe.save(path));
    Model a2;
    ML_ASSERT_GOOD(Model::load(path, a2));
    ML_ASSERT_EQ(ohe, a2);
    ML_ASSERT_EQ(std::remove(path.c_str()), 0);
    
    return 0;
}
