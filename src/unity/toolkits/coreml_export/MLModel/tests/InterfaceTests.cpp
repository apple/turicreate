/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "MLModelTests.hpp"
#include "../src/Model.hpp"
#include "../src/Format.hpp"

#include "framework/TestUtils.hpp"

using namespace CoreML;

Specification::Model& addRequiredField(Specification::Model& model, const std::string& name) {
    auto *input = model.mutable_description()->add_input();
    input->set_name(name);
    input->mutable_type()->mutable_int64type();
    return model;
}

Specification::Model& addOptionalField(Specification::Model& model, const std::string& name) {
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
