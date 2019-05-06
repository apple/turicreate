#define BOOST_TEST_MODULE image_classification_annotation_tests
#include <unity/lib/annotation/image_classification.hpp>

#include <unity/lib/gl_sarray.hpp>

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

    for (int x = 0; x < items.data_size(); x++) {
      TuriCreate::Annotation::Specification::Datum item = items.data(x);
      TS_ASSERT(item.images_size() == 1);

      TuriCreate::Annotation::Specification::ImageDatum image_datum =
          item.images(0);

      size_t datum_width = image_datum.width();
      size_t datum_height = image_datum.height();
      size_t datum_channels = image_datum.channels();

      turi::flex_image image = image_vector.at(x).get<turi::flex_image>();

      TS_ASSERT(image.m_width == datum_width);
      TS_ASSERT(image.m_height == datum_height);
      TS_ASSERT(image.m_channels == datum_channels);
    }
  }

  /*
   * Test Get Items Out Of Index
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

  /*
   * Test Set Annotations Pass
   *
   * This test checks the `setAnnotations` method on the ImageClassification
   * class. It adds an annotation and check whether that annotation is set.
   *
   */

  void test_set_annotations_pass() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    /* Add Annotations */
    TuriCreate::Annotation::Specification::Annotations annotations;
    TuriCreate::Annotation::Specification::Annotation *annotation =
        annotations.add_annotation();

    annotate_spec::Label *label = annotation->add_labels();

    std::string label_value = annotation_testing::random_string();
    label->set_stringlabel(label_value);

    annotation->add_rowindex(10);

    /* Check if the annotations get properly added */
    TS_ASSERT(ic_annotate.setAnnotations(annotations));

    std::shared_ptr<turi::unity_sframe> returned_sf =
        ic_annotate.returnAnnotations();

    std::shared_ptr<turi::unity_sarray> labels_sa =
        std::static_pointer_cast<turi::unity_sarray>(
            returned_sf->select_column(annotation_column_name));

    std::vector<turi::flexible_type> labels_vector = labels_sa->to_vector();

    /* Check that the labels are equal */
    TS_ASSERT(label_value == labels_vector.at(10).to<std::string>());
  }

  /*
   * Test Set Annotations Out Of Index
   *
   * This test checks the `setAnnotations` method on the ImageClassification
   * class. Check the annotation see if an error get's returned when the
   * annotation is set to an index that isn't present in the SFrame.
   *
   */

  void test_set_annotations_out_of_index() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    TuriCreate::Annotation::Specification::Annotations annotations;
    TuriCreate::Annotation::Specification::Annotation *annotation =
        annotations.add_annotation();

    annotate_spec::Label *label = annotation->add_labels();

    std::string label_value = annotation_testing::random_string();
    label->set_stringlabel(label_value);

    annotation->add_rowindex(100);

    /* Check if the annotations get properly handled */
    TS_ASSERT(!ic_annotate.setAnnotations(annotations));
  }

  /*
   * Test Set Annotations Wrong Type
   *
   * This test checks the `setAnnotations` method on the ImageClassification
   * class. Check the annotation see if it errors out when an undefined
   * annotation type gets passed into the method.
   *
   */

  void test_set_annotations_wrong_type() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    TuriCreate::Annotation::Specification::Annotations annotations;
    TuriCreate::Annotation::Specification::Annotation *annotation =
        annotations.add_annotation();

    annotation->add_rowindex(100);

    TS_ASSERT(!ic_annotate.setAnnotations(annotations));
  }

  /*
   * Test Set Annotations Empty
   *
   * This test checks the `setAnnotations` method on the ImageClassification
   * class. Check the annotation see if it errors out when empty set of
   * annotations get passed into the method.
   *
   */

  void test_set_annotations_empty() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    TuriCreate::Annotation::Specification::Annotations annotations;

    TS_ASSERT(ic_annotate.setAnnotations(annotations));
  }

  /*
   * Test Return Annotations
   *
   * This test checks the `returnAnnotations` method on the ImageClassification
   * class. Check if the return value matches (with the Null values) the
   * expected SFrame
   *
   */

  void test_return_annotations() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name, true);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    std::shared_ptr<turi::unity_sframe> returned_sf =
        ic_annotate.returnAnnotations(false);

    TS_ASSERT(annotation_testing::check_equality(annotation_sf, returned_sf));
  }

  /*
   * Test Return Annotations Drop Na
   *
   * This test checks the `returnAnnotations` method on the ImageClassification
   * class. Check if the return value matches (without the Null values) the
   * expected SFrame
   *
   */

  void test_return_annotations_drop_na() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name, true);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    std::shared_ptr<turi::unity_sframe> returned_sf =
        ic_annotate.returnAnnotations(true);

    std::shared_ptr<turi::unity_sarray> labels_sa =
        std::static_pointer_cast<turi::unity_sarray>(
            returned_sf->select_column(annotation_column_name));

    labels_sa = std::static_pointer_cast<turi::unity_sarray>(
        labels_sa->drop_missing_values());

    TS_ASSERT(labels_sa->size() == annotation_sf->size());

    std::shared_ptr<turi::unity_sarray> expected_sa =
        std::static_pointer_cast<turi::unity_sarray>(
            annotation_sf->select_column(annotation_column_name));

    std::vector<turi::flexible_type> flex_data_first = expected_sa->to_vector();
    std::vector<turi::flexible_type> flex_data_second = labels_sa->to_vector();

    for (std::vector<int>::size_type x = 0; x != flex_data_first.size(); x++) {
      TS_ASSERT(flex_data_first[x] == flex_data_second[x]);
    }
  }

  /*
   * Test Annotation Registry
   *
   * This test checks the `get_annotation_registry` method on the
   * ImageClassification. If the user were to forget to save their Annotation to
   * a variable this test show that they can still recover their annotations
   * without fail.
   *
   */

  void test_annotation_registry() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name, true);

    turi::annotate::ImageClassification ic_annotate =
        turi::annotate::ImageClassification(
            annotation_sf, std::vector<std::string>({image_column_name}),
            annotation_column_name);

    std::shared_ptr<turi::unity_sframe> returned_sf =
        ic_annotate.returnAnnotations(false);

    TS_ASSERT(annotation_testing::check_equality(annotation_sf, returned_sf));

    turi::annotate::ImageClassification back_up_annotation =
        turi::annotate::ImageClassification();

    std::shared_ptr<turi::annotate::annotation_global>
        annotation_global_sframe = back_up_annotation.get_annotation_registry();

    std::shared_ptr<turi::unity_sframe> recovered_sf =
        annotation_global_sframe->annotation_sframe;

    TS_ASSERT(annotation_testing::check_equality(annotation_sf, recovered_sf));
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
BOOST_AUTO_TEST_CASE(test_set_annotations_out_of_index) {
  image_classification_test::test_set_annotations_out_of_index();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_wrong_type) {
  image_classification_test::test_set_annotations_wrong_type();
}
BOOST_AUTO_TEST_CASE(test_set_annotations_empty) {
  image_classification_test::test_set_annotations_empty();
}
BOOST_AUTO_TEST_CASE(test_return_annotations) {
  image_classification_test::test_return_annotations();
}
BOOST_AUTO_TEST_CASE(test_return_annotations_drop_na) {
  image_classification_test::test_return_annotations_drop_na();
}
BOOST_AUTO_TEST_CASE(test_annotation_registry) {
  image_classification_test::test_annotation_registry();
}
BOOST_AUTO_TEST_SUITE_END()