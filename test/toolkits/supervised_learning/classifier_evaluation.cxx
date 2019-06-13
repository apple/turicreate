#define BOOST_TEST_MODULE classifier_evaluations

#include <vector>
#include <toolkits/supervised_learning/classifier_evaluations.hpp>
#include <boost/test/unit_test.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/util/test_macros.hpp>

using namespace turi;
using namespace turi::supervised;


BOOST_AUTO_TEST_CASE(test_confusion_matrix) {


  gl_sframe data(
      {{"actual", std::vector<flexible_type>{"a", "a", "b", "b", "b", "b"}},
       {"predicted", std::vector<flexible_type>{"a", "b", "a", "a", "b", "b"}}});


  gl_sframe out = confusion_matrix(
      data, "actual", "predicted");

  std::cout << "out = \n" << out << std::endl;


  TS_ASSERT((out["actual"] == gl_sarray({"a", "a",  "b", "b"})).all());
  TS_ASSERT((out["predicted"] == gl_sarray({"a",  "b", "a", "b"})).all());
  TS_ASSERT((out["count"] == gl_sarray({1,  1, 2, 2})).all());
}

BOOST_AUTO_TEST_CASE(test_prediction_report) {

  gl_sframe data(
      {{"actual", std::vector<flexible_type>{"a", "a", "b", "b", "b", "b"}},
       {"predicted", std::vector<flexible_type>{"a", "b", "a", "a", "b", "b"}}});


  gl_sframe out = classifier_report_by_class(
      data, "actual", "predicted");

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
