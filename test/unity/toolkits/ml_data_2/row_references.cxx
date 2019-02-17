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
#include <unity/toolkits/ml_data_2/ml_data_entry.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data_iterator
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <unity/toolkits/ml_data_2/testing_utils.hpp>

#include <globals/globals.hpp>


using namespace turi;

struct test_rr  {
 public:

  void test_basic_storage() {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 7);

    sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} } );

    v2::ml_data data;

    data.fill(X);

    // Get row references

    std::vector<v2::ml_data_row_reference> rows(data.num_rows());

    for(auto it = data.get_iterator(); !it.done(); ++it) {
      rows[it.row_index()] = it.get_reference();
    }

    // Now go through and make sure that each of these hold the
    // correct answers.

    std::vector<v2::ml_data_entry> x;

    for(size_t i = 0; i < rows.size(); ++i) {
      ASSERT_TRUE(rows[i].metadata().get() == data.metadata().get());

      rows[i].fill(x);

      ASSERT_EQ(x.size(), 2);
      ASSERT_EQ(x[0].column_index, 0);
      ASSERT_EQ(x[0].index, 0);
      ASSERT_EQ(x[0].value, i);

      ASSERT_EQ(x[1].column_index, 1);
      ASSERT_EQ(x[1].index, 0);
      ASSERT_EQ(x[1].value, i);
    }
  }

  void test_untranslated_column_info() {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 7);

    sframe X = make_integer_testing_sframe( {"C1", "C2"}, { {0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4} } );

    v2::ml_data data;

    data.set_data(X, "", {}, { {"C1", v2::ml_column_mode::UNTRANSLATED} });
    data.fill();

    // Get row references
    std::vector<v2::ml_data_row_reference> rows(data.num_rows());

    for(auto it = data.get_iterator(); !it.done(); ++it) {
      rows[it.row_index()] = it.get_reference();
    }

    // Now go through and make sure that each of these hold the
    // correct answers.

    std::vector<v2::ml_data_entry> x;
    std::vector<flexible_type> xf;

    for(size_t i = 0; i < rows.size(); ++i) {
      ASSERT_TRUE(rows[i].metadata().get() == data.metadata().get());

      rows[i].fill(x);

      ASSERT_EQ(x.size(), 1);
      ASSERT_EQ(x[0].column_index, 1);
      ASSERT_EQ(x[0].index, 0);
      ASSERT_EQ(x[0].value, i);

      rows[i].fill_untranslated_values(xf);

      ASSERT_EQ(xf.size(), 1);
      ASSERT_TRUE(xf[0] == i);
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_test_rr, test_rr)
BOOST_AUTO_TEST_CASE(test_basic_storage) {
  test_rr::test_basic_storage();
}
BOOST_AUTO_TEST_CASE(test_untranslated_column_info) {
  test_rr::test_untranslated_column_info();
}
BOOST_AUTO_TEST_SUITE_END()
