#define BOOST_TEST_MODULE image_classification_annotation_tests
#include <unity/lib/annotation/image_classification.hpp>

#include <unity/lib/gl_sarray.hpp>

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <image/image_type.hpp>

#include "utils.cpp"

struct object_detection_test {
  
  void test_pass_through() {
    // TODO: plumb through `test_pass_through`
  }

  void test_get_items() {
    // TODO: plumb through `test_get_items`
  }

  void test_get_items_out_of_index() {
    // TODO: plumb through `test_get_items_out_of_index`
  }

  void test_set_annotations_pass() {
    // TODO: plumb through `test_set_annotations_pass`
  }

  void test_set_annotations_out_of_index() {
    // TODO: plumb through `test_set_annotations_out_of_index`
  }

  void test_set_annotations_wrong_type() {
    // TODO: plumb through `test_set_annotations_wrong_type`
  }

  void test_set_annotations_empty() {
    // TODO: plumb through `test_set_annotations_empty`
  }

  void test_return_annotations() {
    // TODO: plumb through `test_return_annotations`
  }

  void test_return_annotations_drop_na() {
    // TODO: plumb through `test_return_annotations_drop_na`
  }

  void test_annotation_registry() {
    // TODO: plumb through `test_annotation_registry`
  }

};

BOOST_FIXTURE_TEST_SUITE(_object_detection_test, object_detection_test)
BOOST_AUTO_TEST_CASE(test_pass_through) {
  object_detection_test::test_pass_through();
}
BOOST_AUTO_TEST_CASE(test_get_items_out_of_index) {
  object_detection_test::test_get_items_out_of_index();
}
BOOST_AUTO_TEST_CASE(test_get_items) {
  object_detection_test::test_get_items();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_pass) {
  object_detection_test::test_set_annotations_pass();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_out_of_index) {
  object_detection_test::test_set_annotations_out_of_index();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_wrong_type) {
  object_detection_test::test_set_annotations_wrong_type();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_empty) {
  object_detection_test::test_set_annotations_empty();
}
BOOST_AUTO_TEST_CASE(test_return_annotations) {
  object_detection_test::test_return_annotations();
}
BOOST_AUTO_TEST_CASE(test_return_annotations_drop_na) {
  object_detection_test::test_return_annotations_drop_na();
}
BOOST_AUTO_TEST_CASE(test_annotation_registry) {
  object_detection_test::test_annotation_registry();
}
BOOST_AUTO_TEST_SUITE_END()