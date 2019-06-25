#define BOOST_TEST_MODULE annotation_utility_tests
#include <visualization/annotation/image_classification.hpp>

#include <core/data/sframe/gl_sarray.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

#include <core/data/image/image_type.hpp>

#include "utils.cpp"

struct utils_test {
public:
  /*
   * Test Pass Through
   *
   * This test is supposed to check that the data that gets passed into the
   * annotation utility gets passed out in the same manner with the same data
   * format.
   *
   */

  void test_featurize_images() {
    std::string image_column_name = "image";
    std::string annotation_column_name = "annotate";
    std::shared_ptr<turi::unity_sframe> annotation_sf =
        annotation_testing::random_sframe(50, image_column_name,
                                          annotation_column_name);

    std::shared_ptr<turi::unity_sarray> image_sarray =
        std::static_pointer_cast<turi::unity_sarray>(
            annotation_sf->select_column(image_column_name));

    turi::gl_sarray image_gl_sarray = turi::gl_sarray(image_sarray);

    turi::gl_sarray feature_sarray =
        turi::annotate::featurize_images(image_gl_sarray);

    TS_ASSERT(image_gl_sarray.dtype() == turi::flex_type_enum::IMAGE);
    TS_ASSERT(feature_sarray.dtype() == turi::flex_type_enum::VECTOR);

    size_t first_vec_size = feature_sarray[0].get<turi::flex_vec>().size();
    
    for (const auto& i: feature_sarray.range_iterator()) {
      size_t vec_size = i.get<turi::flex_vec>().size();
      TS_ASSERT(first_vec_size == vec_size);
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_utils_test, utils_test)
BOOST_AUTO_TEST_CASE(test_featurize_images) {
  utils_test::test_featurize_images();
}
BOOST_AUTO_TEST_SUITE_END()
