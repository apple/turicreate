#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <core/util/cityhash_gl.hpp>
#include <cxxtest/TestSuite.h>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>
#include <core/random/random.hpp>

// ML-Data Utils
#include <ml/ml_data/ml_data.hpp>
#include <ml/ml_data/ml_data_entry.hpp>
#include <ml/ml_data/metadata.hpp>

// Testing utils common to all of ml_data_iterator
#include <core/util/testing_utils.hpp>
#include <ml/ml_data/testing_utils.hpp>
#include <toolkits/ml_data_2/testing_utils.hpp>

#include <core/globals/globals.hpp>

using namespace turi;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

class test_basic_version_compat : public CxxTest::TestSuite {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  enum class target_column_type {NONE, NUMERICAL, CATEGORICAL};

  void run_version_compat_check_test(size_t n, std::string run_string, target_column_type target_type) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);
    globals::set_global("TURI_ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD", 7);

    random::seed(0);

    ml_data data_1;
    v2::ml_data data_2;

    bool target_column;

    std::map<std::string, ml_column_mode> data_1_modes;
    std::map<std::string, v2::ml_column_mode> data_2_modes;

    if(target_type == target_column_type::CATEGORICAL) {
      target_column = true;
      run_string = std::string("Z") + run_string;
      data_1_modes["target"] = ml_column_mode::CATEGORICAL;
      data_2_modes["target"] = v2::ml_column_mode::CATEGORICAL;
    } else if(target_type == target_column_type::NUMERICAL) {
      run_string = std::string("Z") + run_string;
      data_1_modes["target"] = ml_column_mode::NUMERIC;
      data_2_modes["target"] = v2::ml_column_mode::NUMERIC;
      target_column = true;
    } else {
      target_column = false;
    }

    sframe raw_data = make_random_sframe(n, run_string);

    if(target_column)
      raw_data.set_column_name(0, "target");

    for(size_t i = 0; i < raw_data.num_columns(); ++i) {
      if(raw_data.column_type(i) == flex_type_enum::INTEGER) {
        data_1_modes[raw_data.column_name(i)] = ml_column_mode::CATEGORICAL;
        data_2_modes[raw_data.column_name(i)] = v2::ml_column_mode::CATEGORICAL;
      }
    }

    std::string print_str = run_string;

    if(target_column)
      print_str += ":target";

    if(target_column) {
      data_1.fill(raw_data, "target", data_1_modes);
      data_2.set_data(raw_data, "target", {}, data_2_modes);
      data_2.fill();
    } else {
      data_1.fill(raw_data, "", data_1_modes);
      data_2.set_data(raw_data, "", {}, data_2_modes);
      data_2.fill();
    }

    // Now, go through and verify that the metadata loaded between
    std::shared_ptr<ml_metadata> loaded_metadata;

    // Save v2 metadata, load as current.
    save_and_load_object(loaded_metadata, data_2.metadata());

    data_1.metadata()->_debug_is_equal(loaded_metadata);
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_version_compat_0n() {
    // All unique
    run_version_compat_check_test(50, "n", target_column_type::NONE);
  }

  void test_version_compat_0b() {
    run_version_compat_check_test(50, "b", target_column_type::NONE);
  }

  void test_version_compat_0c() {
    // All unique
    run_version_compat_check_test(50, "c", target_column_type::NONE);
  }

  void test_version_compat_0C() {
    // All unique
    run_version_compat_check_test(50, "C", target_column_type::NONE);
  }

  void test_version_compat_1b_unsorted() {
    run_version_compat_check_test(130, "b", target_column_type::NONE);
  }

  void test_version_compat_1() {
    run_version_compat_check_test(130, "bc", target_column_type::NONE);
  }

  void test_version_compat_2() {
    run_version_compat_check_test(130, "zc", target_column_type::NONE);
  }

  void test_version_compat_3() {
    run_version_compat_check_test(1000, "Zc", target_column_type::NONE);
  }

  void test_version_compat_4() {
    // Pretty much gonna be unique
    run_version_compat_check_test(1000, "Cc", target_column_type::NONE);
  }

  void test_version_compat_5() {
    // 10 blocks of values.
    run_version_compat_check_test(1000, "Zc", target_column_type::NONE);
  }

  void test_version_compat_6() {
    // two large blocks of values
    run_version_compat_check_test(1000, "bc", target_column_type::NONE);
  }

  void test_version_compat_10() {
    // Yeah, a corner case
    run_version_compat_check_test(10, "bc", target_column_type::NONE);
  }

  void test_version_compat_11() {
    // One with just a lot of stuff
    run_version_compat_check_test(2000, "u", target_column_type::NONE);
  }

  void test_version_compat_12() {
    // One with just a lot of stuff
    run_version_compat_check_test(2000, "d", target_column_type::NONE);
  }

  void test_version_compat_13() {
    // One with just a lot of stuff
    run_version_compat_check_test(1000, "cnv", target_column_type::NONE);
  }

  void test_version_compat_14() {
    // One with just a lot of stuff
    run_version_compat_check_test(1000, "du", target_column_type::NONE);
  }

  void test_version_compat_15() {
    // One with just a lot of stuff
    run_version_compat_check_test(30, "UDccccV", target_column_type::NONE);
  }

  void test_version_compat_100() {
    // One with just a lot of stuff
    run_version_compat_check_test(100, "Zcuvd", target_column_type::NONE);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_version_compat_0n_tn() {
    // All unique
    run_version_compat_check_test(500, "n", target_column_type::NUMERICAL);
  }

  void test_version_compat_0C_tn() {
    // All unique
    run_version_compat_check_test(500, "c", target_column_type::NUMERICAL);
  }

  void test_version_compat_1_unsorted_tn() {
    run_version_compat_check_test(500, "b", target_column_type::NUMERICAL);
  }

  void test_version_compat_0b_tn() {
    // All unique
    run_version_compat_check_test(130, "C", target_column_type::NUMERICAL);
  }

  void test_version_compat_1b_unsorted_tn() {
    run_version_compat_check_test(130, "b", target_column_type::NUMERICAL);
  }

  void test_version_compat_1_tn() {
    run_version_compat_check_test(130, "bc", target_column_type::NUMERICAL);
  }

  void test_version_compat_2_tn() {
    run_version_compat_check_test(130, "zc", target_column_type::NUMERICAL);
  }

  void test_version_compat_3_tn() {
    run_version_compat_check_test(1000, "Zc", target_column_type::NUMERICAL);
  }

  void test_version_compat_4_tn() {
    // Pretty much gonna be unique
    run_version_compat_check_test(1000, "Cc", target_column_type::NUMERICAL);
  }

  void test_version_compat_5_tn() {
    // 10 blocks of values.
    run_version_compat_check_test(1000, "Zc", target_column_type::NUMERICAL);
  }

  void test_version_compat_6_tn() {
    // two large blocks of values
    run_version_compat_check_test(1000, "bc", target_column_type::NUMERICAL);
  }

  void test_version_compat_10_tn() {
    // Yeah, a corner case
    run_version_compat_check_test(10, "bc", target_column_type::NUMERICAL);
  }

  void test_version_compat_11_tn() {
    // One with just a lot of stuff
    run_version_compat_check_test(2000, "u", target_column_type::NUMERICAL);
  }

  void test_version_compat_12_tn() {
    // One with just a lot of stuff
    run_version_compat_check_test(2000, "d", target_column_type::NUMERICAL);
  }

  void test_version_compat_13_tn() {
    // One with just a lot of stuff
    run_version_compat_check_test(1000, "cnv", target_column_type::NUMERICAL);
  }

  void test_version_compat_14_tn() {
    // One with just a lot of stuff
    run_version_compat_check_test(1000, "du", target_column_type::NUMERICAL);
  }

  void test_version_compat_15_tn() {
    // One with just a lot of stuff
    run_version_compat_check_test(30, "UDccccV", target_column_type::NUMERICAL);
  }

  void test_version_compat_100_tn() {
    // One with just a lot of stuff
    run_version_compat_check_test(100, "Zcuvd", target_column_type::NUMERICAL);
  }

  void test_version_compat_16_null_tn() {
    // two large blocks of values
    run_version_compat_check_test(1000, "", target_column_type::NUMERICAL);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_version_compat_0n_tc() {
    // All unique
    run_version_compat_check_test(50, "n", target_column_type::CATEGORICAL);
  }

  void test_version_compat_0C_tc() {
    // All unique
    run_version_compat_check_test(50, "c", target_column_type::CATEGORICAL);
  }

  void test_version_compat_1_unsorted_tc() {
    run_version_compat_check_test(50, "b", target_column_type::CATEGORICAL);
  }

  void test_version_compat_0b_tc() {
    // All unique
    run_version_compat_check_test(130, "C", target_column_type::CATEGORICAL);
  }

  void test_version_compat_1b_unsorted_tc() {
    run_version_compat_check_test(130, "b", target_column_type::CATEGORICAL);
  }

  void test_version_compat_1_tc() {
    run_version_compat_check_test(130, "bc", target_column_type::CATEGORICAL);
  }

  void test_version_compat_2_tc() {
    run_version_compat_check_test(130, "zc", target_column_type::CATEGORICAL);
  }

  void test_version_compat_3_tc() {
    run_version_compat_check_test(1000, "Zc", target_column_type::CATEGORICAL);
  }

  void test_version_compat_4_tc() {
    // Pretty much gonna be unique
    run_version_compat_check_test(1000, "Cc", target_column_type::CATEGORICAL);
  }

  void test_version_compat_5_tc() {
    // 10 blocks of values.
    run_version_compat_check_test(1000, "Zc", target_column_type::CATEGORICAL);
  }

  void test_version_compat_6_tc() {
    // two large blocks of values
    run_version_compat_check_test(1000, "bc", target_column_type::CATEGORICAL);
  }

  void test_version_compat_10_tc() {
    // Yeah, a corner case
    run_version_compat_check_test(10, "bc", target_column_type::CATEGORICAL);
  }

  void test_version_compat_11_tc() {
    // One with just a lot of stuff
    run_version_compat_check_test(2000, "u", target_column_type::CATEGORICAL);
  }

  void test_version_compat_12_tc() {
    // One with just a lot of stuff
    run_version_compat_check_test(2000, "d", target_column_type::CATEGORICAL);
  }

  void test_version_compat_13_tc() {
    // One with just a lot of stuff
    run_version_compat_check_test(1000, "cnv", target_column_type::CATEGORICAL);
  }

  void test_version_compat_14_tc() {
    // One with just a lot of stuff
    run_version_compat_check_test(1000, "du", target_column_type::CATEGORICAL);
  }

  void test_version_compat_15_tc() {
    // One with just a lot of stuff
    run_version_compat_check_test(30, "UDccccV", target_column_type::CATEGORICAL);
  }

  void test_version_compat_100_tc() {
    // One with just a lot of stuff
    run_version_compat_check_test(100, "Zcuvd", target_column_type::CATEGORICAL);
  }

  void test_version_compat_16_null_tc() {
    // two large blocks of values
    run_version_compat_check_test(1000, "", target_column_type::CATEGORICAL);
  }

};
