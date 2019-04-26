#include "MLModelTests.hpp"

// TODO -- Fix these headers.
#include "../src/Model.hpp"
#include "../src/transforms/OneHotEncoder.hpp"
#include "../src/transforms/TreeEnsemble.hpp"


#include "framework/TestUtils.hpp"
#include <cassert>
#include <cstdio>

using namespace CoreML;

int testOneHotEncoderBasic() {
    OneHotEncoder ohe;

    /* Asserts that the type provided is allowed as a feature type by a new
     one hot encoder appended to a model asset named "a". */
    ohe = OneHotEncoder();
    ML_ASSERT_GOOD(ohe.addInput("Int64", FeatureType::Int64()));

    ohe = OneHotEncoder();
    ML_ASSERT_GOOD(ohe.addInput("String", FeatureType::String()));

    /* Asserts that the type provided is not allowed as a feature type by a new
     one hot encoder. */
    ohe = OneHotEncoder();

    /*
    Result r;
    ML_ASSERT_GOOD(r = ohe.addInput("Double", FeatureType::Double()));
    ML_ASSERT_EQ(r.type(), ResultType::FEATURE_TYPE_INVARIANT_VIOLATION);

    r = ohe.addInput("Image", FeatureType::Image());
    ML_ASSERT_EQ(r.type(), ResultType::FEATURE_TYPE_INVARIANT_VIOLATION);

    r = ohe.addInput("Array", FeatureType::Array({2,3}));
    ML_ASSERT_EQ(r.type(), ResultType::FEATURE_TYPE_INVARIANT_VIOLATION);

    r = ohe.addInput("Dictionary", FeatureType::Dictionary(MLTypeInt64));
    ML_ASSERT_EQ(r.type(), ResultType::FEATURE_TYPE_INVARIANT_VIOLATION);
    */

    /* Asserts that the TYPE provided is generally allowed as a feature type by a
     new one hot encoder, but has a type mismatch if that one hot encoder is added
     to the existing model asset "a". */
    /*
    ohe = OneHotEncoder();
    ML_ASSERT_GOOD(ohe.addInput("String", FeatureType::String()));
    */

    /* however, this one should still work, since it doesn't modify the type
     * (since the underlying data is already in one-hot encoding, it's probably
     * a no-op, but that's a decision for the execution engine and not the model
     * serialization -- at this point in time, it's moot, as long as the types
     * match). */
    /*
    ohe = OneHotEncoder();
    ML_ASSERT_GOOD(ohe.addInput("Int64", FeatureType::Int64()));
    */

    return 0;
}
