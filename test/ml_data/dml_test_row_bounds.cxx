#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <core/util/cityhash_tc.hpp>

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
#include <core/storage/sframe_data/testing_utils.hpp>
#include <ml/ml_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

#include <core/globals/globals.hpp>

using namespace turi;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

struct test_row_bounds  {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  enum class target_column_type {NONE, NUMERICAL, CATEGORICAL};
  
  void run_row_bounds_check_test(size_t n, std::string run_string, target_column_type target_type,
                                 std::vector<size_t> untranslated_columns = {}) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);
    globals::set_global("TURI_ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD", 7);

    // This must hold if we are going to have deterministic indexing
    ASSERT_LE(n, 10000); 
    

    random::seed(0);

    bool target_column;

    if(target_type == target_column_type::CATEGORICAL) {
      target_column = true;
      run_string = std::string("Z") + run_string;
    } else if(target_type == target_column_type::NUMERICAL) {
      target_column = true;
      run_string = std::string("n") + run_string;
    } else {
      target_column = false;
    }

    sframe raw_data = make_random_sframe(n, run_string, false);

    if(target_column) 
      raw_data.set_column_name(0, "target");
    
    std::vector<std::vector<flexible_type> > ref_data = testing_extract_sframe_data(raw_data);

    std::map<std::string, ml_column_mode> mode_overrides;

    for(size_t c_idx : untranslated_columns)
      mode_overrides[raw_data.column_name(c_idx)] = ml_column_mode::UNTRANSLATED;

    for(size_t c_idx = target_column ? 1 : 0; c_idx < raw_data.num_columns(); ++c_idx) {
      if(raw_data.column_type(c_idx) == flex_type_enum::INTEGER)
        mode_overrides[raw_data.column_name(c_idx)] = ml_column_mode::CATEGORICAL;
    }
    
    std::string print_str = run_string;

    if(target_column)
      print_str += ":target";

    bool target_column_categorical = (target_type == target_column_type::CATEGORICAL);

    if(target_column_categorical)
      mode_overrides["target"] = ml_column_mode::CATEGORICAL;
    else
      mode_overrides["target"] = ml_column_mode::NUMERIC;

    ml_data data;

    data.fill(raw_data, (target_column ? "target" : ""), mode_overrides); 
    
    ////////////////////////////////////////////////////////////////////////////////

    std::vector<std::pair<size_t, size_t> > row_segments
    { {0, n},
      {0, n / 3},
      {n / 3, 2*n / 3},
      {2*n / 3, n} };

    for(const std::pair<size_t, size_t>& row_bounds : row_segments) {

      sframe sliced_raw_data = slice_sframe(raw_data, row_bounds.first, row_bounds.second); 

      ml_data data_row_sliced;
      data_row_sliced.fill(raw_data, row_bounds, (target_column ? "target" : ""), mode_overrides);

      ml_data data_true;
      data_true.fill(sliced_raw_data, (target_column ? "target" : ""), mode_overrides);

      ml_data data_sliced = data.slice(row_bounds.first, row_bounds.second);
      
      // Now go through and make sure they are equal.
      data_row_sliced.metadata()->_debug_is_equal(data_true.metadata());

      ASSERT_EQ(data_row_sliced.num_rows(), data_true.num_rows()); 
      ASSERT_EQ(data_sliced.num_rows(), data_true.num_rows()); 

      in_parallel([&](size_t thread_idx, size_t num_threads) { 
      
          std::vector<ml_data_entry> x1, x2, x3;
          std::vector<flexible_type> xf1, xf2, xf3;
          std::vector<flexible_type> row_x1, row_x2, row_x3;
      
          auto it_1 = data_row_sliced.get_iterator(thread_idx, num_threads); 
          auto it_2 = data_true.get_iterator(thread_idx, num_threads); 
          auto it_3 = data_sliced.get_iterator(thread_idx, num_threads);

          for(;!it_1.done();++it_1, ++it_2, ++it_3) {
            ASSERT_FALSE(it_2.done());
            ASSERT_FALSE(it_3.done());

            ASSERT_EQ(it_1.row_index(), it_2.row_index());
            ASSERT_EQ(it_1.row_index(), it_3.row_index());
            
            ////////////////////////////////////////////////////////////////////////////////
            // Test that the sliced creation and the creation from sliced are identical
        
            it_1->fill(x1);
            it_2->fill(x2);
            it_3->fill(x3);

            ASSERT_TRUE(x1 == x2); 
        
            ASSERT_TRUE(it_1->target_index() == it_2->target_index()); 
            ASSERT_TRUE(it_1->target_value() == it_2->target_value());
        
            ASSERT_TRUE(it_1->target_value() == it_3->target_value()); 

            ////////////////////////////////////////////////////////////////////////////////
            // Test reference to the original data, including the sliced data.
        
            row_x1 = translate_row_to_original(data_row_sliced.metadata(), x1);
            row_x2 = translate_row_to_original(data_true.metadata(), x2);
            row_x3 = translate_row_to_original(data_sliced.metadata(), x3);
        
            ASSERT_EQ(row_x1.size(), row_x2.size());
            ASSERT_EQ(row_x1.size(), row_x3.size());
        
            for(size_t ri = 0; ri < row_x1.size(); ++ri) {
              ASSERT_TRUE(ml_testing_equals(row_x1.at(ri), row_x2.at(ri)));
              ASSERT_TRUE(ml_testing_equals(row_x1.at(ri), row_x3.at(ri)));
            }

            ////////////////////////////////////////////////////////////////////////////////
            // Test the unobserved values

            it_1->fill_untranslated_values(xf1);
            it_2->fill_untranslated_values(xf2);
            it_3->fill_untranslated_values(xf3);

            ASSERT_TRUE(xf1 == xf2); 
            ASSERT_TRUE(xf1 == xf3);
          }
          ASSERT_TRUE(it_2.done());
          ASSERT_TRUE(it_3.done());
        });
    }
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_row_bounds_000() {
    // All unique
    run_row_bounds_check_test(0, "n", target_column_type::NONE);
  }

  void test_row_bounds_0n() {
    // All unique
    run_row_bounds_check_test(5, "n", target_column_type::NONE);
  }

  void test_row_bounds_0b() {
    run_row_bounds_check_test(5, "b", target_column_type::NONE);
  }

  void test_row_bounds_0c() {
    // All unique
    run_row_bounds_check_test(5, "c", target_column_type::NONE);
  }

  void test_row_bounds_0C() {
    // All unique
    run_row_bounds_check_test(5, "C", target_column_type::NONE);
  }

  void test_row_bounds_1b_unsorted() {
    run_row_bounds_check_test(13, "b", target_column_type::NONE);
  }

  void test_row_bounds_1() {
    run_row_bounds_check_test(13, "bc", target_column_type::NONE);
  }

  void test_row_bounds_2() {
    run_row_bounds_check_test(13, "zc", target_column_type::NONE);
  }

  void test_row_bounds_3() {
    run_row_bounds_check_test(100, "Zc", target_column_type::NONE);
  }

  void test_row_bounds_4() {
    // Pretty much gonna be unique
    run_row_bounds_check_test(100, "Cc", target_column_type::NONE);
  }

  void test_row_bounds_5() {
    // 10 blocks of values.
    run_row_bounds_check_test(1000, "Zc", target_column_type::NONE);
  }

  void test_row_bounds_6() {
    // two large blocks of values
    run_row_bounds_check_test(1000, "bc", target_column_type::NONE);
  }

  void test_row_bounds_10() {
    // Yeah, a corner case
    run_row_bounds_check_test(1, "bc", target_column_type::NONE);
  }

  void test_row_bounds_11() {
    // One with just a lot of stuff
    run_row_bounds_check_test(200, "u", target_column_type::NONE);
  }

  void test_row_bounds_12() {
    // One with just a lot of stuff
    run_row_bounds_check_test(200, "d", target_column_type::NONE);
  }

  void test_row_bounds_13() {
    // One with just a lot of stuff
    run_row_bounds_check_test(1000, "cnv", target_column_type::NONE);
  }

  void test_row_bounds_14() {
    // One with just a lot of stuff
    run_row_bounds_check_test(1000, "du", target_column_type::NONE);
  }

  void test_row_bounds_15() {
    // One with just a lot of stuff
    run_row_bounds_check_test(3, "UDccccV", target_column_type::NONE);
  }

  void test_row_bounds_100() {
    // One with just a lot of stuff
    run_row_bounds_check_test(10, "Zcuvd", target_column_type::NONE);
  }
  
  void test_untranslated_columns_nn_1() {
    run_row_bounds_check_test(109, "nn", target_column_type::NONE, {1});
  }

  void test_untranslated_columns_nn_2() {
    run_row_bounds_check_test(109, "nn", target_column_type::NONE, {0});
  }

  void test_untranslated_columns_nn_3() {
    run_row_bounds_check_test(109, "nn", target_column_type::NONE, {0, 1});
  }

  void test_untranslated_columns_ssss_1() {
    run_row_bounds_check_test(109, "ssss", target_column_type::NONE, {1,3});
  }

  void test_untranslated_columns_ssss_2() {
    run_row_bounds_check_test(109, "ssss", target_column_type::NONE, {0,1,2,3});
  }

  void test_untranslated_columns_dd_1() {
    run_row_bounds_check_test(109, "dd", target_column_type::NONE, {1});
  }

  void test_untranslated_columns_dd_2() {
    run_row_bounds_check_test(109, "dd", target_column_type::NONE, {0});
  }

  void test_untranslated_columns_dd_3() {
    run_row_bounds_check_test(109, "dd", target_column_type::NONE, {0,1});
  }

  void test_untranslated_columns_v_1() {
    run_row_bounds_check_test(109, "v", target_column_type::NONE, {0});
  }

  void test_untranslated_columns_many_1() {
    run_row_bounds_check_test(109, "cnsnscsnccccccccncss", target_column_type::NONE,
                              {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});
  }

  void test_untranslated_columns_many_2() {
    run_row_bounds_check_test(109, "cnsnscsnccccccccncss", target_column_type::NONE, {19});
  }

  
  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_row_bounds_000_tn() {
    // All unique
    run_row_bounds_check_test(0, "n", target_column_type::NUMERICAL);
  }

  void test_row_bounds_0n_tn() {
    // All unique
    run_row_bounds_check_test(5, "n", target_column_type::NUMERICAL);
  }

  void test_row_bounds_0C_tn() {
    // All unique
    run_row_bounds_check_test(5, "c", target_column_type::NUMERICAL);
  }

  void test_row_bounds_1_unsorted_tn() {
    run_row_bounds_check_test(5, "b", target_column_type::NUMERICAL);
  }

  void test_row_bounds_0b_tn() {
    // All unique
    run_row_bounds_check_test(13, "C", target_column_type::NUMERICAL);
  }

  void test_row_bounds_1b_unsorted_tn() {
    run_row_bounds_check_test(13, "b", target_column_type::NUMERICAL);
  }

  void test_row_bounds_1_tn() {
    run_row_bounds_check_test(13, "bc", target_column_type::NUMERICAL);
  }

  void test_row_bounds_2_tn() {
    run_row_bounds_check_test(13, "zc", target_column_type::NUMERICAL);
  }

  void test_row_bounds_3_tn() {
    run_row_bounds_check_test(100, "Zc", target_column_type::NUMERICAL);
  }

  void test_row_bounds_4_tn() {
    // Pretty much gonna be unique
    run_row_bounds_check_test(100, "Cc", target_column_type::NUMERICAL);
  }

  void test_row_bounds_5_tn() {
    // 10 blocks of values.
    run_row_bounds_check_test(1000, "Zc", target_column_type::NUMERICAL);
  }

  void test_row_bounds_6_tn() {
    // two large blocks of values
    run_row_bounds_check_test(1000, "bc", target_column_type::NUMERICAL);
  }

  void test_row_bounds_10_tn() {
    // Yeah, a corner case
    run_row_bounds_check_test(1, "bc", target_column_type::NUMERICAL);
  }

  void test_row_bounds_11_tn() {
    // One with just a lot of stuff
    run_row_bounds_check_test(200, "u", target_column_type::NUMERICAL);
  }

  void test_row_bounds_12_tn() {
    // One with just a lot of stuff
    run_row_bounds_check_test(200, "d", target_column_type::NUMERICAL);
  }

  void test_row_bounds_13_tn() {
    // One with just a lot of stuff
    run_row_bounds_check_test(1000, "cnv", target_column_type::NUMERICAL);
  }

  void test_row_bounds_14_tn() {
    // One with just a lot of stuff
    run_row_bounds_check_test(1000, "du", target_column_type::NUMERICAL);
  }

  void test_row_bounds_15_tn() {
    // One with just a lot of stuff
    run_row_bounds_check_test(3, "UDccccV", target_column_type::NUMERICAL);
  }

  void test_row_bounds_100_tn() {
    // One with just a lot of stuff
    run_row_bounds_check_test(10, "Zcuvd", target_column_type::NUMERICAL);
  }
  
  void test_row_bounds_16_null_tn() {
    // two large blocks of values
    run_row_bounds_check_test(1000, "", target_column_type::NUMERICAL);
  }
  
  void test_untranslated_columns_nn_1_num() {
    run_row_bounds_check_test(109, "nn", target_column_type::NUMERICAL, {1});
  }

  void test_untranslated_columns_nn_2_num() {
    run_row_bounds_check_test(109, "nn", target_column_type::NUMERICAL, {0});
  }

  void test_untranslated_columns_nn_3_num() {
    run_row_bounds_check_test(109, "nn", target_column_type::NUMERICAL, {0, 1});
  }

  void test_untranslated_columns_ssss_1_num() {
    run_row_bounds_check_test(109, "ssss", target_column_type::NUMERICAL, {1,3});
  }

  void test_untranslated_columns_ssss_2_num() {
    run_row_bounds_check_test(109, "ssss", target_column_type::NUMERICAL, {0,1,2,3});
  }

  void test_untranslated_columns_dd_1_num() {
    run_row_bounds_check_test(109, "dd", target_column_type::NUMERICAL, {1});
  }

  void test_untranslated_columns_dd_2_num() {
    run_row_bounds_check_test(109, "dd", target_column_type::NUMERICAL, {0});
  }

  void test_untranslated_columns_dd_3_num() {
    run_row_bounds_check_test(109, "dd", target_column_type::NUMERICAL, {0,1});
  }

  void test_untranslated_columns_v_1_num() {
    run_row_bounds_check_test(109, "v", target_column_type::NUMERICAL, {0});
  }

  void test_untranslated_columns_many_1_num() {
    run_row_bounds_check_test(109, "cnsnscsnccccccccncss", target_column_type::NUMERICAL,
                              {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});
  }

  void test_untranslated_columns_many_2_num() {
    run_row_bounds_check_test(109, "cnsnscsnccccccccncss", target_column_type::NUMERICAL, {19});
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_row_bounds_000_tc() {
    // All unique
    run_row_bounds_check_test(0, "n", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_0n_tc() {
    // All unique
    run_row_bounds_check_test(5, "n", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_0C_tc() {
    // All unique
    run_row_bounds_check_test(5, "c", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_1_unsorted_tc() {
    run_row_bounds_check_test(5, "b", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_0b_tc() {
    // All unique
    run_row_bounds_check_test(13, "C", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_1b_unsorted_tc() {
    run_row_bounds_check_test(13, "b", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_1_tc() {
    run_row_bounds_check_test(13, "bc", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_2_tc() {
    run_row_bounds_check_test(13, "zc", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_3_tc() {
    run_row_bounds_check_test(100, "Zc", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_4_tc() {
    // Pretty much gonna be unique
    run_row_bounds_check_test(100, "Cc", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_5_tc() {
    // 10 blocks of values.
    run_row_bounds_check_test(1000, "Zc", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_6_tc() {
    // two large blocks of values
    run_row_bounds_check_test(1000, "bc", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_10_tc() {
    // Yeah, a corner case
    run_row_bounds_check_test(1, "bc", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_11_tc() {
    // One with just a lot of stuff
    run_row_bounds_check_test(200, "u", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_12_tc() {
    // One with just a lot of stuff
    run_row_bounds_check_test(200, "d", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_13_tc() {
    // One with just a lot of stuff
    run_row_bounds_check_test(1000, "cnv", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_14_tc() {
    // One with just a lot of stuff
    run_row_bounds_check_test(1000, "du", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_15_tc() {
    // One with just a lot of stuff
    run_row_bounds_check_test(3, "UDccccV", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_100_tc() {
    // One with just a lot of stuff
    run_row_bounds_check_test(10, "Zcuvd", target_column_type::CATEGORICAL);
  }

  void test_row_bounds_16_null_tc() {
    // two large blocks of values
    run_row_bounds_check_test(1000, "", target_column_type::CATEGORICAL);
  }
  
  void test_untranslated_columns_nn_1_cat() {
    run_row_bounds_check_test(109, "nn", target_column_type::CATEGORICAL, {1});
  }

  void test_untranslated_columns_nn_2_cat() {
    run_row_bounds_check_test(109, "nn", target_column_type::CATEGORICAL, {0});
  }

  void test_untranslated_columns_nn_3_cat() {
    run_row_bounds_check_test(109, "nn", target_column_type::CATEGORICAL, {0, 1});
  }

  void test_untranslated_columns_ssss_1_cat() {
    run_row_bounds_check_test(109, "ssss", target_column_type::CATEGORICAL, {1,3});
  }

  void test_untranslated_columns_ssss_2_cat() {
    run_row_bounds_check_test(109, "ssss", target_column_type::CATEGORICAL, {0,1,2,3});
  }

  void test_untranslated_columns_dd_1_cat() {
    run_row_bounds_check_test(109, "dd", target_column_type::CATEGORICAL, {1});
  }

  void test_untranslated_columns_dd_2_cat() {
    run_row_bounds_check_test(109, "dd", target_column_type::CATEGORICAL, {0});
  }

  void test_untranslated_columns_dd_3_cat() {
    run_row_bounds_check_test(109, "dd", target_column_type::CATEGORICAL, {0,1});
  }

  void test_untranslated_columns_v_1_cat() {
    run_row_bounds_check_test(109, "v", target_column_type::CATEGORICAL, {0});
  }

  void test_untranslated_columns_many_1_cat() {
    run_row_bounds_check_test(109, "cnsnscsnccccccccncss", target_column_type::CATEGORICAL,
                              {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});
  }

  void test_untranslated_columns_many_2_cat() {
    run_row_bounds_check_test(109, "cnsnscsnccccccccncss", target_column_type::CATEGORICAL, {19});
  }
  
};

BOOST_FIXTURE_TEST_SUITE(_test_row_bounds, test_row_bounds)
BOOST_AUTO_TEST_CASE(test_row_bounds_000) {
  test_row_bounds::test_row_bounds_000();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0n) {
  test_row_bounds::test_row_bounds_0n();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0b) {
  test_row_bounds::test_row_bounds_0b();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0c) {
  test_row_bounds::test_row_bounds_0c();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0C) {
  test_row_bounds::test_row_bounds_0C();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1b_unsorted) {
  test_row_bounds::test_row_bounds_1b_unsorted();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1) {
  test_row_bounds::test_row_bounds_1();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_2) {
  test_row_bounds::test_row_bounds_2();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_3) {
  test_row_bounds::test_row_bounds_3();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_4) {
  test_row_bounds::test_row_bounds_4();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_5) {
  test_row_bounds::test_row_bounds_5();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_6) {
  test_row_bounds::test_row_bounds_6();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_10) {
  test_row_bounds::test_row_bounds_10();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_11) {
  test_row_bounds::test_row_bounds_11();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_12) {
  test_row_bounds::test_row_bounds_12();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_13) {
  test_row_bounds::test_row_bounds_13();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_14) {
  test_row_bounds::test_row_bounds_14();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_15) {
  test_row_bounds::test_row_bounds_15();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_100) {
  test_row_bounds::test_row_bounds_100();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_1) {
  test_row_bounds::test_untranslated_columns_nn_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_2) {
  test_row_bounds::test_untranslated_columns_nn_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_3) {
  test_row_bounds::test_untranslated_columns_nn_3();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_1) {
  test_row_bounds::test_untranslated_columns_ssss_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_2) {
  test_row_bounds::test_untranslated_columns_ssss_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_1) {
  test_row_bounds::test_untranslated_columns_dd_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_2) {
  test_row_bounds::test_untranslated_columns_dd_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_3) {
  test_row_bounds::test_untranslated_columns_dd_3();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_v_1) {
  test_row_bounds::test_untranslated_columns_v_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_1) {
  test_row_bounds::test_untranslated_columns_many_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_2) {
  test_row_bounds::test_untranslated_columns_many_2();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_000_tn) {
  test_row_bounds::test_row_bounds_000_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0n_tn) {
  test_row_bounds::test_row_bounds_0n_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0C_tn) {
  test_row_bounds::test_row_bounds_0C_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1_unsorted_tn) {
  test_row_bounds::test_row_bounds_1_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0b_tn) {
  test_row_bounds::test_row_bounds_0b_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1b_unsorted_tn) {
  test_row_bounds::test_row_bounds_1b_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1_tn) {
  test_row_bounds::test_row_bounds_1_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_2_tn) {
  test_row_bounds::test_row_bounds_2_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_3_tn) {
  test_row_bounds::test_row_bounds_3_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_4_tn) {
  test_row_bounds::test_row_bounds_4_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_5_tn) {
  test_row_bounds::test_row_bounds_5_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_6_tn) {
  test_row_bounds::test_row_bounds_6_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_10_tn) {
  test_row_bounds::test_row_bounds_10_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_11_tn) {
  test_row_bounds::test_row_bounds_11_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_12_tn) {
  test_row_bounds::test_row_bounds_12_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_13_tn) {
  test_row_bounds::test_row_bounds_13_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_14_tn) {
  test_row_bounds::test_row_bounds_14_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_15_tn) {
  test_row_bounds::test_row_bounds_15_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_100_tn) {
  test_row_bounds::test_row_bounds_100_tn();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_16_null_tn) {
  test_row_bounds::test_row_bounds_16_null_tn();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_1_num) {
  test_row_bounds::test_untranslated_columns_nn_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_2_num) {
  test_row_bounds::test_untranslated_columns_nn_2_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_3_num) {
  test_row_bounds::test_untranslated_columns_nn_3_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_1_num) {
  test_row_bounds::test_untranslated_columns_ssss_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_2_num) {
  test_row_bounds::test_untranslated_columns_ssss_2_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_1_num) {
  test_row_bounds::test_untranslated_columns_dd_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_2_num) {
  test_row_bounds::test_untranslated_columns_dd_2_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_3_num) {
  test_row_bounds::test_untranslated_columns_dd_3_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_v_1_num) {
  test_row_bounds::test_untranslated_columns_v_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_1_num) {
  test_row_bounds::test_untranslated_columns_many_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_2_num) {
  test_row_bounds::test_untranslated_columns_many_2_num();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_000_tc) {
  test_row_bounds::test_row_bounds_000_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0n_tc) {
  test_row_bounds::test_row_bounds_0n_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0C_tc) {
  test_row_bounds::test_row_bounds_0C_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1_unsorted_tc) {
  test_row_bounds::test_row_bounds_1_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_0b_tc) {
  test_row_bounds::test_row_bounds_0b_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1b_unsorted_tc) {
  test_row_bounds::test_row_bounds_1b_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_1_tc) {
  test_row_bounds::test_row_bounds_1_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_2_tc) {
  test_row_bounds::test_row_bounds_2_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_3_tc) {
  test_row_bounds::test_row_bounds_3_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_4_tc) {
  test_row_bounds::test_row_bounds_4_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_5_tc) {
  test_row_bounds::test_row_bounds_5_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_6_tc) {
  test_row_bounds::test_row_bounds_6_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_10_tc) {
  test_row_bounds::test_row_bounds_10_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_11_tc) {
  test_row_bounds::test_row_bounds_11_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_12_tc) {
  test_row_bounds::test_row_bounds_12_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_13_tc) {
  test_row_bounds::test_row_bounds_13_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_14_tc) {
  test_row_bounds::test_row_bounds_14_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_15_tc) {
  test_row_bounds::test_row_bounds_15_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_100_tc) {
  test_row_bounds::test_row_bounds_100_tc();
}
BOOST_AUTO_TEST_CASE(test_row_bounds_16_null_tc) {
  test_row_bounds::test_row_bounds_16_null_tc();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_1_cat) {
  test_row_bounds::test_untranslated_columns_nn_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_2_cat) {
  test_row_bounds::test_untranslated_columns_nn_2_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_3_cat) {
  test_row_bounds::test_untranslated_columns_nn_3_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_1_cat) {
  test_row_bounds::test_untranslated_columns_ssss_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_2_cat) {
  test_row_bounds::test_untranslated_columns_ssss_2_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_1_cat) {
  test_row_bounds::test_untranslated_columns_dd_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_2_cat) {
  test_row_bounds::test_untranslated_columns_dd_2_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_3_cat) {
  test_row_bounds::test_untranslated_columns_dd_3_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_v_1_cat) {
  test_row_bounds::test_untranslated_columns_v_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_1_cat) {
  test_row_bounds::test_untranslated_columns_many_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_2_cat) {
  test_row_bounds::test_untranslated_columns_many_2_cat();
}
BOOST_AUTO_TEST_SUITE_END()
