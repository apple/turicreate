/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE capi_classifier_evaluations

#include <vector>
#include <boost/test/unit_test.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/util/test_macros.hpp>
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include "capi_utils.hpp"

using namespace turi;

BOOST_AUTO_TEST_CASE(test_confusion_matrix) {

  tc_error* error = nullptr;

  gl_sframe data(
      {{"actual", std::vector<flexible_type>{"a", "a", "b", "b", "b", "b"}},
       {"predicted", std::vector<flexible_type>{"a", "b", "a", "a", "b", "b"}}});


  tc_parameters* p = new_tc_parameters(variant_map_type{
      {"data", data},
      {"target", "actual"},
      {"predicted", "predicted"}});

  tc_variant* v = tc_function_call("_supervised_learning.confusion_matrix", p, &error);

  CAPI_CHECK_ERROR(error);

  tc_sframe* ret = tc_variant_sframe(v, &error);
  CAPI_CHECK_ERROR(error);

  gl_sframe out = ret->value;

  tc_release(ret);
  tc_release(v);
  tc_release(p);

  std::cout << "out = \n" << out << std::endl;


  TS_ASSERT((out["actual"] == gl_sarray({"a", "a",  "b", "b"})).all());
  TS_ASSERT((out["predicted"] == gl_sarray({"a",  "b", "a", "b"})).all());
  TS_ASSERT((out["count"] == gl_sarray({1,  1, 2, 2})).all());
}

BOOST_AUTO_TEST_CASE(test_prediction_report) {

  tc_error* error = nullptr;

  gl_sframe data(
      {{"actual", std::vector<flexible_type>{"a", "a", "b", "b", "b", "b"}},
       {"predicted", std::vector<flexible_type>{"a", "b", "a", "a", "b", "b"}}});


  tc_parameters* p = new_tc_parameters(variant_map_type{
      {"data", data},
      {"target", "actual"},
      {"predicted", "predicted"}});

  tc_variant* v = tc_function_call("_supervised_learning.classifier_report_by_class", p, &error);

  CAPI_CHECK_ERROR(error);

  tc_sframe* ret = tc_variant_sframe(v, &error);
  CAPI_CHECK_ERROR(error);

  gl_sframe out = ret->value;

  tc_release(ret);
  tc_release(v);
  tc_release(p);


  std::cout << "out = \n" << out << std::endl;

  TS_ASSERT((out["class"] == gl_sarray({"a", "b"})).all());
  TS_ASSERT((out["predicted_correctly"] == gl_sarray({1, 2})).all());
  TS_ASSERT((out["predicted_this_incorrectly"] == gl_sarray({2, 1})).all());
  TS_ASSERT((out["missed_predicting_this"] == gl_sarray({1, 2})).all());
  TS_ASSERT_DELTA(out["precision"][0], 0.3333, 0.01);
  TS_ASSERT_DELTA(out["precision"][1], 0.6666, 0.01);
  TS_ASSERT_DELTA(out["recall"][0], 0.5, 0.01);
  TS_ASSERT_DELTA(out["recall"][1], 0.5, 0.01);

}
