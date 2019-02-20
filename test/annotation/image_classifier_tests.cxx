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
   * Test Pass Through
   *
   * This test is supposed to check that the data that gets passed into the
   * annotation utility gets passed out in the same manner with the same data
   * format.
   *
   */
  void test_pass_through() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    std::shared_ptr<turi::unity_sframe> returned_sf =
        ic_annotate.returnAnnotations(false);

    TS_ASSERT(annotation_testing::check_equality(annotation_sf, returned_sf));
  }

  /*
   * Test Get Items
   *
   * This test checks that the items that are in the sframe get properly
   * formatted in the data protobuf object.
   *
   */

  void test_get_items() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    TuriCreate::Annotation::Specification::Data items =
        ic_annotate.getItems(0, 10);

    TS_ASSERT(items.data_size() == 10);

    std::shared_ptr<turi::unity_sarray> image_sa =
        std::static_pointer_cast<turi::unity_sarray>(
            annotation_sf->select_column(image_column_name));

    std::vector<turi::flexible_type> image_vector = image_sa->to_vector();

    for (size_t x = 0; x < items.data_size(); x++) {
      TuriCreate::Annotation::Specification::Datum item = items.data(x);
      TS_ASSERT(item.images_size() == 1);

      TuriCreate::Annotation::Specification::ImageDatum image_datum =
          item.images(0);

      int datum_width = image_datum.width();
      int datum_height = image_datum.height();
      int datum_channels = image_datum.channels();

      turi::flex_image image = image_vector.at(x).get<turi::flex_image>();

      TS_ASSERT(image.m_width == datum_width);
      TS_ASSERT(image.m_height == datum_height);
      TS_ASSERT(image.m_channels == datum_channels);
    }
  }

  /*
   * Test Get Items Out of Index
   *
   * This test checks that when an invalid range is passed into the parameters
   * of `getItems` an empty data object gets returned.
   *
   */

  void test_get_items_out_of_index() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    TuriCreate::Annotation::Specification::Data items =
        ic_annotate.getItems(50, 100);

    TS_ASSERT(items.data_size() == 0);
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