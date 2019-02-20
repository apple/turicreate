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

struct test_schema_errors  {
 public:

  void _test_schema_mismatch(bool target_column, bool ignore_new_columns) {

    sframe X;
    v2::ml_data mdata;

    std::map<std::string, flexible_type> config_options;

    if(ignore_new_columns)
      config_options["ignore_new_columns_after_train"] = true;

    std::tie(X, mdata) = v2::make_random_sframe_and_ml_data(
        5, "CCCC", target_column, config_options);

    sframe X2 = X;

    X2.set_column_name(0, "My-column-lies-over-the-ocean.");

    v2::ml_data data_2(mdata.metadata());

    TS_ASSERT_THROWS_ANYTHING(data_2.fill(X2));

    sframe X3 = X.add_column(X.select_column(0), "My-column-lies-over-the-sea.");

    v2::ml_data data_3(mdata.metadata());

    if(ignore_new_columns) {
      data_3.fill(X3);
    } else {
      TS_ASSERT_THROWS_ANYTHING(data_3.fill(X3));
    }

    sframe X4 = X.remove_column(3);

    v2::ml_data data_4(mdata.metadata());

    TS_ASSERT_THROWS_ANYTHING(data_4.fill(X4));
  }

  void test_schema_mismatch() {
    _test_schema_mismatch(false, false);
  }

  void test_schema_mismatch_t() {
    _test_schema_mismatch(true, false);
  }

  void test_schema_mismatch_ignore() {
    _test_schema_mismatch(false, true);
  }

  void test_schema_mismatch_ignore_t() {
    _test_schema_mismatch(true, true);
  }


  void _test_schema_mismatch_with_side_data(bool target_column, bool ignore_new_columns) {

    std::map<std::string, flexible_type> config_options;

    if(ignore_new_columns)
      config_options["ignore_new_columns_after_train"] = true;

    auto info = v2::make_ml_data_with_side_data(
        5, "cccc", { {5, "cn"}, {5, "cs"}, {5, "cv"} }, target_column, config_options);

    {
      sframe X2 = info.main_sframe;

      X2.set_column_name(0, "My-column-lies-over-the-ocean.");

      v2::ml_data data_2(info.data.metadata());

      TS_ASSERT_THROWS_ANYTHING(data_2.fill(X2));
    }

    {
      sframe X3 = info.main_sframe.add_column(info.main_sframe.select_column(0), "My-column-lies-over-the-sea.");

      v2::ml_data data_3(info.data.metadata());

      if(ignore_new_columns) {
        data_3.fill(X3);
      } else {
        TS_ASSERT_THROWS_ANYTHING(data_3.fill(X3));
      }
    }

    {
      sframe X4 = info.main_sframe.remove_column(3);

      v2::ml_data data_4(info.data.metadata());

      TS_ASSERT_THROWS_ANYTHING(data_4.fill(X4));
    }

    // Test the adding of a wrong row to side columns
    {
      sframe X5 = info.side_sframes[0].add_column(info.side_sframes[0].select_column(0),
                                                  "how-now-scowl-cow");

      v2::ml_data data_5(info.data.metadata());
      data_5.add_side_data(X5);

      if(ignore_new_columns) {
        data_5.fill();
      } else {
        TS_ASSERT_THROWS_ANYTHING(data_5.fill());
      }
    }

    // Test the lack of a wrong row to side columns
    {
      sframe X6 = info.side_sframes[0].remove_column(1);

      v2::ml_data data_6(info.data.metadata());

      data_6.add_side_data(X6);
      TS_ASSERT_THROWS_ANYTHING(data_6.fill());
    }

    // Test the adding of side information onto a column that has none
    // provided at train time.
    {
      sframe X7 = info.side_sframes[0];
      X7.set_column_name(0, info.main_sframe.column_name(3));

      v2::ml_data data_7(info.data.metadata());

      data_7.add_side_data(X7);

      if(ignore_new_columns) {
        data_7.fill();
      } else {
        TS_ASSERT_THROWS_ANYTHING(data_7.fill());
      }
    }
  }

  void test_schema_mismatch_side_0() {
    _test_schema_mismatch_with_side_data(false, false);
  }

  void test_schema_mismatch_side_1() {
    _test_schema_mismatch_with_side_data(true, false);
  }

  void test_schema_mismatch_side_2() {
    _test_schema_mismatch_with_side_data(false, true);
  }

  void test_schema_mismatch_side_3() {
    _test_schema_mismatch_with_side_data(true, true);
  }



};

BOOST_FIXTURE_TEST_SUITE(_test_schema_errors, test_schema_errors)
BOOST_AUTO_TEST_CASE(test_schema_mismatch) {
  test_schema_errors::test_schema_mismatch();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_t) {
  test_schema_errors::test_schema_mismatch_t();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_ignore) {
  test_schema_errors::test_schema_mismatch_ignore();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_ignore_t) {
  test_schema_errors::test_schema_mismatch_ignore_t();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_side_0) {
  test_schema_errors::test_schema_mismatch_side_0();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_side_1) {
  test_schema_errors::test_schema_mismatch_side_1();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_side_2) {
  test_schema_errors::test_schema_mismatch_side_2();
}
BOOST_AUTO_TEST_CASE(test_schema_mismatch_side_3) {
  test_schema_errors::test_schema_mismatch_side_3();
}
BOOST_AUTO_TEST_SUITE_END()
