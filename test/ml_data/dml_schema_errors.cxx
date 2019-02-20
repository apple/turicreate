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
#include <ml_data/ml_data.hpp>

// Testing utils common to all of ml_data_iterator
#include <sframe/testing_utils.hpp>
#include <ml_data/testing_utils.hpp>

#include <globals/globals.hpp>

using namespace turi;

struct test_schema_errors  {
 public:

  void _test_schema_mismatch(bool target_column) {

    sframe X;
    ml_data mdata;

    std::tie(X, mdata) = make_random_sframe_and_ml_data(
        5, "CCCC", target_column);

    sframe X2 = X;
    X2.set_column_name(0, "My-column-lies-over-the-ocean.");

    ml_data data_2(mdata.metadata());

    TS_ASSERT_THROWS_ANYTHING(data_2.fill(X2));
    
    sframe X3 = X.add_column(X.select_column(0), "My-column-lies-over-the-sea.");
    
    ml_data data_3(mdata.metadata());
    
    // Succeeds, but should print a warning.
    data_3.fill(X3);

    sframe X4 = X.remove_column(3);

    ml_data data_4(mdata.metadata());

    TS_ASSERT_THROWS_ANYTHING(data_4.fill(X4));
  }

  void test_schema_mismatch() {
    _test_schema_mismatch(false);
  }

  void test_schema_mismatch_t() {
    _test_schema_mismatch(true);
  }

 void _test_schema_mismatch_row(bool target_column) {

    sframe X;
    ml_data mdata;
    ml_missing_value_action none_action = ml_missing_value_action::ERROR;
        
    std::tie(X, mdata) = make_random_sframe_and_ml_data(
        5, "nnnn", target_column);

    {
      flex_dict row = { {mdata.metadata()->column_name(0), 0},
                        {mdata.metadata()->column_name(1), 0},
                        {mdata.metadata()->column_name(2), 0} }; 

      // Doesn't have column 3
      TS_ASSERT_THROWS_ANYTHING(ml_data_row_reference::from_row(mdata.metadata(), row, none_action));
      if(target_column) {
        row.push_back({mdata.metadata()->target_column_name(), 0});
        TS_ASSERT_THROWS_ANYTHING(ml_data_row_reference::from_row(mdata.metadata(), row, none_action));
      }
    }
      
    {
      flex_dict row = { {mdata.metadata()->column_name(0), 0},
                        {mdata.metadata()->column_name(1), 0},
                        {mdata.metadata()->column_name(2), 0},
                        {mdata.metadata()->column_name(3), 0} }; 

      // Should succeed
      ml_data_row_reference::from_row(mdata.metadata(), row);
      if(target_column) {
        row.push_back({mdata.metadata()->target_column_name(), 0});
        ml_data_row_reference::from_row(mdata.metadata(), row);
      }
        
    }

    {
      flex_dict row = { {mdata.metadata()->column_name(0), 0},
                        {mdata.metadata()->column_name(1), 0},
                        {mdata.metadata()->column_name(2), 0},
                        {"It's all about that column.", 0} }; 

      // Should succeed
      TS_ASSERT_THROWS_ANYTHING(ml_data_row_reference::from_row(mdata.metadata(), row, none_action));
      if(target_column) {
        row.push_back({mdata.metadata()->target_column_name(), 0});
        TS_ASSERT_THROWS_ANYTHING(ml_data_row_reference::from_row(mdata.metadata(), row, none_action));
      }
    }
  }

  void test_schema_mismatch_row() {
    _test_schema_mismatch_row(false);
  }

  void test_schema_mismatch_row_t() {
    _test_schema_mismatch_row(true);
  }
};

BOOST_FIXTURE_TEST_SUITE(_test_schema_errors, test_schema_errors)
BOOST_AUTO_TEST_CASE(test_schema_mismatch) {
  test_schema_errors::test_schema_mismatch();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_t) {
  test_schema_errors::test_schema_mismatch_t();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_row) {
  test_schema_errors::test_schema_mismatch_row();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_row_t) {
  test_schema_errors::test_schema_mismatch_row_t();
}
BOOST_AUTO_TEST_SUITE_END()
