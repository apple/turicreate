/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_dc_evaluation

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <ml/neural_net/model_spec.hpp>
#include <toolkits/coreml_export/mlmodel_include.hpp>
#include <toolkits/drawing_classifier/drawing_classifier.hpp>

namespace turi {
namespace drawing_classifer {
namespace {

struct mock_evaluate : public turi::drawing_classifier::drawing_classifier {
  // shadow the original predict
  gl_sarray predict(gl_sframe data, std::string output_type) {
    TS_ASSERT_EQUALS(output_type, "probility_vector");
    TS_ASSERT(generate_result_ != nullptr);
    return generate_result_();
  }

  std::function<gl_sarray()> generate_result_;
};

BOOST_AUTO_TEST_CASE(test_dc_evaluation) {

}

}  // anonymous namespace
}  // namespace drawing_classifer
}  // namespace turi