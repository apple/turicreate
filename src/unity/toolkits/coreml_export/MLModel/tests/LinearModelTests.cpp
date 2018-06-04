#include "MLModelTests.hpp"

// TODO -- Fix these headers.
#include "../src/Model.hpp"
#include "../src/transforms/OneHotEncoder.hpp"
#include "../src/transforms/LinearModel.hpp"
#include "../src/transforms/TreeEnsemble.hpp"


#include "framework/TestUtils.hpp"
#include <cassert>
#include <cstdio>

using namespace CoreML;

int testLinearModelBasic() {
    LinearModel lr("foo", "Linear regression model to predict Foo");
    ML_ASSERT_GOOD(lr.addInput("x", FeatureType::Int64()));
    ML_ASSERT_GOOD(lr.addInput("y", FeatureType::Double()));
    ML_ASSERT_GOOD(lr.addInput("z", FeatureType::Int64()));
    ML_ASSERT_GOOD(lr.addOutput("foo", FeatureType::Double()));
    return 0;
}
