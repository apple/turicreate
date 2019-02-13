#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <unity/lib/flex_dict_view.hpp>
#include <random/random.hpp>

// ML-Data Utils
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/row_slicing_utilities.hpp>

// Testing utils common to all of ml_data_iterator
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <unity/toolkits/ml_data_2/testing_utils.hpp>

#include <globals/globals.hpp>

using namespace turi;

struct test_row_slicing  {
 public:

  void test_basic_1() {

    for(bool column_0_is_untranslated : {true, false} ) {

      sframe X = make_integer_testing_sframe( {"C0", "C1", "C2"}, { {1,2,3}, {4,5,6} } );

      v2::ml_data data;

      if(column_0_is_untranslated) {
        data.set_data(X, "", {}, { {"C0", v2::ml_column_mode::UNTRANSLATED} });
      } else {
        data.set_data(X);
      }

      data.fill();

      std::vector<v2::ml_data_entry> x_t;
      std::vector<flexible_type> x_u;

      v2::row_slicer _s_c1_c2(data.metadata(), {1, 2} );

      // Load it from the serialization to test that part
      v2::row_slicer s_c1_c2;
      save_and_load_object(s_c1_c2, _s_c1_c2);

      ASSERT_EQ(s_c1_c2.num_dimensions(), 2);

      v2::dense_vector vd;
      v2::sparse_vector vs;
      std::vector<flexible_type> vu;

      ////////////////////////////////////////

      auto it = data.get_iterator();

      it.fill_observation(x_t);
      it.fill_untranslated_values(x_u);

      s_c1_c2.slice(vd, x_t, x_u);

      // There are 2 numerical columns included in this test
      ASSERT_EQ(vd.size(), 2);
      ASSERT_EQ(size_t(vd[0]), 2);  // First row, 2nd column, by the slicer
      ASSERT_EQ(size_t(vd[1]), 3);  // First row, 3nd column, by the slicer

      s_c1_c2.slice(vs, x_t, x_u);

      ASSERT_EQ(vd.nonZeros(), 2);
      ASSERT_EQ(size_t(vd.coeffRef(0)), 2);  // First row, 2nd column, by the slicer
      ASSERT_EQ(size_t(vd.coeffRef(1)), 3);  // First row, 2nd column, by the slicer

      ++it;


      it.fill_observation(x_t);
      it.fill_untranslated_values(x_u);

      s_c1_c2.slice(vd, x_t, x_u);

      // There are 2 numerical columns included in this test
      ASSERT_EQ(vd.size(), 2);
      ASSERT_EQ(size_t(vd[0]), 5);  // Second row, 2nd column, by the slicer
      ASSERT_EQ(size_t(vd[1]), 6);  // Second row, 3nd column, by the slicer

      s_c1_c2.slice(vs, x_t, x_u);

      ASSERT_EQ(vd.nonZeros(), 2);
      ASSERT_EQ(size_t(vd.coeffRef(0)), 5);  // Second row, 2nd column, by the slicer
      ASSERT_EQ(size_t(vd.coeffRef(1)), 6);  // Second row, 2nd column, by the slicer

      ++it;
      ASSERT_TRUE(it.done());
    }
  }

  void test_with_untranslated_columns_1() {

    for(bool column_0_is_untranslated : {true, false} ) {

      sframe X = make_integer_testing_sframe( {"C0", "C1", "C2"}, { {1,2,3}, {4,5,6} } );

      v2::ml_data data;

      if(column_0_is_untranslated) {
        data.set_data(X, "", {},
                      { {"C0", v2::ml_column_mode::UNTRANSLATED},
                        {"C1", v2::ml_column_mode::UNTRANSLATED},
                        {"C2", v2::ml_column_mode::UNTRANSLATED} });
      } else {
        data.set_data(X, "", {},
                      { {"C1", v2::ml_column_mode::UNTRANSLATED},
                        {"C2", v2::ml_column_mode::UNTRANSLATED} });
      }

      data.fill();

      std::vector<v2::ml_data_entry> x_t;
      std::vector<flexible_type> x_u;

      v2::row_slicer _s_c1_c2(data.metadata(), {1, 2} );

      // Load it from the serialization to test that part
      v2::row_slicer s_c1_c2;
      save_and_load_object(s_c1_c2, _s_c1_c2);


      ASSERT_EQ(s_c1_c2.num_dimensions(), 0);

      v2::dense_vector vd;
      v2::sparse_vector vs;
      std::vector<flexible_type> vu;

      ////////////////////////////////////////

      auto it = data.get_iterator();

      it.fill_observation(x_t);
      it.fill_untranslated_values(x_u);

      s_c1_c2.slice(vu, x_t, x_u);

      // There are 2 numerical columns included in this test
      ASSERT_EQ(vu.size(), 2);
      ASSERT_EQ(size_t(vu[0]), 2);  // First row, 2nd column, by the slicer
      ASSERT_EQ(size_t(vu[1]), 3);  // First row, 3nd column, by the slicer

      ++it;


      it.fill_observation(x_t);
      it.fill_untranslated_values(x_u);

      s_c1_c2.slice(vu, x_t, x_u);

      // There are 2 numerical columns included in this test
      ASSERT_EQ(vu.size(), 2);
      ASSERT_EQ(size_t(vu[0]), 5);  // Second row, 2nd column, by the slicer
      ASSERT_EQ(size_t(vu[1]), 6);  // Second row, 3nd column, by the slicer

      ++it;
      ASSERT_TRUE(it.done());
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_test_row_slicing, test_row_slicing)
BOOST_AUTO_TEST_CASE(test_basic_1) {
  test_row_slicing::test_basic_1();
}
BOOST_AUTO_TEST_CASE(test_with_untranslated_columns_1) {
  test_row_slicing::test_with_untranslated_columns_1();
}
BOOST_AUTO_TEST_SUITE_END()
