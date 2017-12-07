/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
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
