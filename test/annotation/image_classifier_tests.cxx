#define BOOST_TEST_MODULE
#include <unity/lib/annotation/image_classification.hpp>

#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>

#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <image/image_type.hpp>

struct image_classification_test {
public:
  void test_pass_through() {
    // 1) make an sframe with an image and annotation column
    //    - make a function to generate image data

    std::string image_column_name = "image";
    std::string annotation_column_name = "annotation";

    std::vector<std::string> column_names = {image_column_name,
                                             annotation_column_name};

    std::vector<turi::flex_image> image_column_data;
    std::vector<turi::flex_image> annotation_column_data;

    char data[] = {static_cast<char>(255), static_cast<char>(0), static_cast<char>(255),
                   static_cast<char>(255), static_cast<char>(0), static_cast<char>(255),
                   static_cast<char>(255), static_cast<char>(0), static_cast<char>(255),
                   static_cast<char>(255), static_cast<char>(0), static_cast<char>(255)};

    char * img_data = data;                 
    size_t height = 2;
    size_t width = 2;
    size_t channels = 3;
    
    size_t data_size = 12;
    size_t image_type_version = IMAGE_TYPE_CURRENT_VERSION;
    size_t format = 2;

    turi::flex_image* img_new = new turi::image_type(img_data, height, width,
                                                    channels, data_size,
                                                    image_type_version, format);

    const unsigned char * image_data = img_new->get_image_data();

    for(int x = 0; x < 12; x++) {
      printf("%d\n", (uint8_t)(image_data[x]));
    }


    // TODO: generate random images
    // TODO: randomly generate annotation labels

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