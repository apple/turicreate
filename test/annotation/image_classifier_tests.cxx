#define BOOST_TEST_MODULE
#include <unity/lib/annotation/image_classification.hpp>

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <image/image_type.hpp>

#include "utils.cpp"

struct image_classification_test {
public:
  /*
   * TODO: add a description of what this test is supposed to check for
   */
  void test_pass_through() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    
    TS_ASSERT(true);
  }

  void test_get_items() {
    // TODO: retrieving items from sframe
    // check if data is streamed properly into protobuf format
    TS_ASSERT(true);
  }

  void test_get_items_out_of_index() {
    // TODO: retrieving items from sframe
    // check if data object is undefined
    TS_ASSERT(true);
  }

  void test_set_annotations_pass() {
    // TODO: adding annotations to the class
    // test whether the annotations get properly added
    TS_ASSERT(true);
  }

  void test_set_annotations_fail() {
    // TODO: add incorrect annotations to the class
    // test whether the incorrect annotations get caught
    TS_ASSERT(true);
  }

  void test_return_annotations() {
    // TODO: add and return the annotation sframe
    // test whether the returned sframe keeps na values
    TS_ASSERT(true);
  }

  void test_return_annotations_drop_na() {
    // TODO: add and return the annotation sframe
    // test whether the returned sframe drops the na values
    TS_ASSERT(true);
  }
};

BOOST_FIXTURE_TEST_SUITE(_image_classification_test, image_classification_test)
BOOST_AUTO_TEST_CASE(test_pass_through) {
  image_classification_test::test_pass_through();
}
BOOST_AUTO_TEST_CASE(test_get_items_out_of_index) {
  image_classification_test::test_get_items_out_of_index();
}
BOOST_AUTO_TEST_CASE(test_get_items) {
  image_classification_test::test_get_items();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_pass) {
  image_classification_test::test_set_annotations_pass();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_fail) {
  image_classification_test::test_set_annotations_fail();
}
BOOST_AUTO_TEST_CASE(test_return_annotations) {
  image_classification_test::test_return_annotations();
}
BOOST_AUTO_TEST_CASE(test_return_annotations_drop_na) {
  image_classification_test::test_return_annotations_drop_na();
}
BOOST_AUTO_TEST_SUITE_END()