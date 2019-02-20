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
#include <ml_data/ml_data_entry.hpp>
#include <ml_data/metadata.hpp>

// Testing utils common to all of ml_data_iterator
#include <sframe/testing_utils.hpp>
#include <ml_data/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <globals/globals.hpp>

using namespace turi;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

struct test_sorted_columns  {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  enum class target_column_type {NONE, NUMERICAL, CATEGORICAL};

  void run_sorted_columns_check_test(size_t n, std::string run_string, target_column_type target_type,
                                     std::set<size_t> untranslated_columns = {}) {

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
      mode_overrides[raw_data.column_name(c_idx + (target_column ? 1 : 0))] = ml_column_mode::UNTRANSLATED;

    for(size_t c_idx = 0; c_idx < raw_data.num_columns(); ++c_idx) {
      if(raw_data.column_type(c_idx) == flex_type_enum::INTEGER ||
         raw_data.column_type(c_idx) == flex_type_enum::STRING) {
        mode_overrides[raw_data.column_name(c_idx)] = ml_column_mode::CATEGORICAL_SORTED;
      }
    }

    std::string print_str = run_string;

    if(target_column)
      print_str += ":target";

    bool target_column_categorical = (target_type == target_column_type::CATEGORICAL);

    if(target_column_categorical)
      mode_overrides["target"] = ml_column_mode::CATEGORICAL_SORTED;
    else
      mode_overrides["target"] = ml_column_mode::NUMERIC;

    ml_data data;

    data.fill(raw_data, (target_column ? "target" : ""), mode_overrides);

    for(size_t i = 0; i < data.metadata()->num_columns(); ++i) {
      if(data.metadata()->column_mode(i) == ml_column_mode::CATEGORICAL) {
        auto m = data.metadata()->indexer(i);
        m->debug_check_is_internally_consistent();
        for(size_t i = 0; i + 1 < m->indexed_column_size(); ++i) {
          ASSERT_TRUE(!(m->map_index_to_value(i) > m->map_index_to_value(i+1)));
        }
      }
      if(target_type == target_column_type::CATEGORICAL) {
        auto m = data.metadata()->target_indexer();
        m->debug_check_is_internally_consistent();
        for(size_t i = 0; i + 1 < m->indexed_column_size(); ++i) {
          ASSERT_TRUE(!(m->map_index_to_value(i) > m->map_index_to_value(i+1)));
        }
      }
    }

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        std::vector<ml_data_entry> x;
        std::vector<ml_data_entry_global_index> x_gi;
        DenseVector xd;
        Eigen::MatrixXd xdr;

        SparseVector xs;

        std::vector<flexible_type> row_x;

        xd.resize(data.metadata()->num_dimensions());
        xs.resize(data.metadata()->num_dimensions());

        xdr.resize(3, data.metadata()->num_dimensions());
        xdr.setZero();

        ////////////////////////////////////////////////////////////////////////////////
        // Run the actual tests

        for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
          size_t it_idx = it.row_index();

          for(size_t type_idx : {0, 1, 2, 3, 4} ) {

            switch(type_idx) {
              case 0: {
                it->fill(x);

                // std::cerr << "x = " << x << std::endl;
                row_x = translate_row_to_original(data.metadata(), x);
                break;
              }
              case 1: {
                it->fill(xd);

                row_x = translate_row_to_original(data.metadata(), xd);
                // std::cerr << "xd = " << xd.transpose() << std::endl;
                break;
              }
              case 2: {
                it->fill(xs);

                row_x = translate_row_to_original(data.metadata(), xs);
                break;
              }
              case 3: {
                it->fill(x_gi);

                row_x = translate_row_to_original(data.metadata(), x_gi);
                break;
              }
              case 4: {
                it->fill(xdr.row(1));

                xd = xdr.row(1);

                row_x = translate_row_to_original(data.metadata(), xd);
                break;
              }
            }

            if(target_column && target_type == target_column_type::NUMERICAL) {
              row_x.insert(row_x.begin(), it->target_value());
            } else if(target_column && target_type == target_column_type::CATEGORICAL) {
              row_x.insert(row_x.begin(),
                           data.metadata()->target_indexer()->map_index_to_value(it->target_index()));
            }

            ASSERT_EQ(row_x.size(), raw_data.num_columns());
            ASSERT_EQ(row_x.size(), ref_data.at(it_idx).size());

            for(size_t ri = 0; ri < row_x.size(); ++ri) {  //
              if(untranslated_columns.count(ri - (target_column ? 1 : 0)) == 0) {
                ASSERT_TRUE(ml_testing_equals(row_x.at(ri), ref_data.at(it_idx).at(ri)));
              }
            }
          }
        }
      });
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_sorted_columns_000() {
    // All unique
    run_sorted_columns_check_test(0, "n", target_column_type::NONE);
  }

  void test_sorted_columns_0n() {
    // All unique
    run_sorted_columns_check_test(5, "n", target_column_type::NONE);
  }

  void test_sorted_columns_0b() {
    run_sorted_columns_check_test(5, "b", target_column_type::NONE);
  }

  void test_sorted_columns_0c() {
    // All unique
    run_sorted_columns_check_test(5, "c", target_column_type::NONE);
  }

  void test_sorted_columns_0C() {
    // All unique
    run_sorted_columns_check_test(5, "C", target_column_type::NONE);
  }

  void test_sorted_columns_1b_unsorted() {
    run_sorted_columns_check_test(13, "b", target_column_type::NONE);
  }

  void test_sorted_columns_1() {
    run_sorted_columns_check_test(13, "bc", target_column_type::NONE);
  }

  void test_sorted_columns_2() {
    run_sorted_columns_check_test(13, "zc", target_column_type::NONE);
  }

  void test_sorted_columns_3() {
    run_sorted_columns_check_test(100, "Zc", target_column_type::NONE);
  }

  void test_sorted_columns_4() {
    // Pretty much gonna be unique
    run_sorted_columns_check_test(100, "Cc", target_column_type::NONE);
  }

  void test_sorted_columns_5() {
    // 10 blocks of values.
    run_sorted_columns_check_test(1000, "Zc", target_column_type::NONE);
  }

  void test_sorted_columns_6() {
    // two large blocks of values
    run_sorted_columns_check_test(1000, "bc", target_column_type::NONE);
  }

  void test_sorted_columns_10() {
    // Yeah, a corner case
    run_sorted_columns_check_test(1, "bc", target_column_type::NONE);
  }

  void test_sorted_columns_11() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(200, "u", target_column_type::NONE);
  }

  void test_sorted_columns_12() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(200, "d", target_column_type::NONE);
  }

  void test_sorted_columns_13() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(1000, "cnv", target_column_type::NONE);
  }

  void test_sorted_columns_14() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(1000, "du", target_column_type::NONE);
  }

  void test_sorted_columns_15() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(3, "UDccccV", target_column_type::NONE);
  }

  void test_sorted_columns_100() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(10, "Zcuvd", target_column_type::NONE);
  }

  void test_untranslated_columns_nn_1() {
    run_sorted_columns_check_test(109, "nn", target_column_type::NONE, {1});
  }

  void test_untranslated_columns_nn_2() {
    run_sorted_columns_check_test(109, "nn", target_column_type::NONE, {0});
  }

  void test_untranslated_columns_nn_3() {
    run_sorted_columns_check_test(109, "nn", target_column_type::NONE, {0, 1});
  }

  void test_untranslated_columns_ssss_1() {
    run_sorted_columns_check_test(109, "ssss", target_column_type::NONE, {1,3});
  }

  void test_untranslated_columns_ssss_2() {
    run_sorted_columns_check_test(109, "ssss", target_column_type::NONE, {0,1,2,3});
  }

  void test_untranslated_columns_dd_1() {
    run_sorted_columns_check_test(109, "dd", target_column_type::NONE, {1});
  }

  void test_untranslated_columns_dd_2() {
    run_sorted_columns_check_test(109, "dd", target_column_type::NONE, {0});
  }

  void test_untranslated_columns_dd_3() {
    run_sorted_columns_check_test(109, "dd", target_column_type::NONE, {0,1});
  }

  void test_untranslated_columns_v_1() {
    run_sorted_columns_check_test(109, "v", target_column_type::NONE, {0});
  }

  void test_untranslated_columns_many_1() {
    run_sorted_columns_check_test(109, "cnsnscsnccccccccncss", target_column_type::NONE,
                              {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});
  }

  void test_untranslated_columns_many_2() {
    run_sorted_columns_check_test(109, "cnsnscsnccccccccncss", target_column_type::NONE, {19});
  }


  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_sorted_columns_000_tn() {
    // All unique
    run_sorted_columns_check_test(0, "n", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_0n_tn() {
    // All unique
    run_sorted_columns_check_test(5, "n", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_0C_tn() {
    // All unique
    run_sorted_columns_check_test(5, "c", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_1_unsorted_tn() {
    run_sorted_columns_check_test(5, "b", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_0b_tn() {
    // All unique
    run_sorted_columns_check_test(13, "C", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_1b_unsorted_tn() {
    run_sorted_columns_check_test(13, "b", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_1_tn() {
    run_sorted_columns_check_test(13, "bc", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_2_tn() {
    run_sorted_columns_check_test(13, "zc", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_3_tn() {
    run_sorted_columns_check_test(100, "Zc", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_4_tn() {
    // Pretty much gonna be unique
    run_sorted_columns_check_test(100, "Cc", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_5_tn() {
    // 10 blocks of values.
    run_sorted_columns_check_test(1000, "Zc", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_6_tn() {
    // two large blocks of values
    run_sorted_columns_check_test(1000, "bc", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_10_tn() {
    // Yeah, a corner case
    run_sorted_columns_check_test(1, "bc", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_11_tn() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(200, "u", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_12_tn() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(200, "d", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_13_tn() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(1000, "cnv", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_14_tn() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(1000, "du", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_15_tn() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(3, "UDccccV", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_100_tn() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(10, "Zcuvd", target_column_type::NUMERICAL);
  }

  void test_sorted_columns_16_null_tn() {
    // two large blocks of values
    run_sorted_columns_check_test(1000, "", target_column_type::NUMERICAL);
  }

  void test_untranslated_columns_nn_1_num() {
    run_sorted_columns_check_test(109, "nn", target_column_type::NUMERICAL, {1});
  }

  void test_untranslated_columns_nn_2_num() {
    run_sorted_columns_check_test(109, "nn", target_column_type::NUMERICAL, {0});
  }

  void test_untranslated_columns_nn_3_num() {
    run_sorted_columns_check_test(109, "nn", target_column_type::NUMERICAL, {0, 1});
  }

  void test_untranslated_columns_ssss_1_num() {
    run_sorted_columns_check_test(109, "ssss", target_column_type::NUMERICAL, {1,3});
  }

  void test_untranslated_columns_ssss_2_num() {
    run_sorted_columns_check_test(109, "ssss", target_column_type::NUMERICAL, {0,1,2,3});
  }

  void test_untranslated_columns_dd_1_num() {
    run_sorted_columns_check_test(109, "dd", target_column_type::NUMERICAL, {1});
  }

  void test_untranslated_columns_dd_2_num() {
    run_sorted_columns_check_test(109, "dd", target_column_type::NUMERICAL, {0});
  }

  void test_untranslated_columns_dd_3_num() {
    run_sorted_columns_check_test(109, "dd", target_column_type::NUMERICAL, {0,1});
  }

  void test_untranslated_columns_v_1_num() {
    run_sorted_columns_check_test(109, "v", target_column_type::NUMERICAL, {0});
  }

  void test_untranslated_columns_many_1_num() {
    run_sorted_columns_check_test(109, "cnsnscsnccccccccncss", target_column_type::NUMERICAL,
                              {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});
  }

  void test_untranslated_columns_many_2_num() {
    run_sorted_columns_check_test(109, "cnsnscsnccccccccncss", target_column_type::NUMERICAL, {19});
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_sorted_columns_000_tc() {
    // All unique
    run_sorted_columns_check_test(0, "n", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_0n_tc() {
    // All unique
    run_sorted_columns_check_test(5, "n", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_0C_tc() {
    // All unique
    run_sorted_columns_check_test(5, "c", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_1_unsorted_tc() {
    run_sorted_columns_check_test(5, "b", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_0b_tc() {
    // All unique
    run_sorted_columns_check_test(13, "C", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_1b_unsorted_tc() {
    run_sorted_columns_check_test(13, "b", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_1_tc() {
    run_sorted_columns_check_test(13, "bc", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_2_tc() {
    run_sorted_columns_check_test(13, "zc", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_3_tc() {
    run_sorted_columns_check_test(100, "Zc", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_4_tc() {
    // Pretty much gonna be unique
    run_sorted_columns_check_test(100, "Cc", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_5_tc() {
    // 10 blocks of values.
    run_sorted_columns_check_test(1000, "Zc", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_6_tc() {
    // two large blocks of values
    run_sorted_columns_check_test(1000, "bc", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_10_tc() {
    // Yeah, a corner case
    run_sorted_columns_check_test(1, "bc", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_11_tc() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(200, "u", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_12_tc() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(200, "d", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_13_tc() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(1000, "cnv", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_14_tc() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(1000, "du", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_15_tc() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(3, "UDccccV", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_100_tc() {
    // One with just a lot of stuff
    run_sorted_columns_check_test(10, "Zcuvd", target_column_type::CATEGORICAL);
  }

  void test_sorted_columns_16_null_tc() {
    // two large blocks of values
    run_sorted_columns_check_test(1000, "", target_column_type::CATEGORICAL);
  }

  void test_untranslated_columns_nn_1_cat() {
    run_sorted_columns_check_test(109, "nn", target_column_type::CATEGORICAL, {1});
  }

  void test_untranslated_columns_nn_2_cat() {
    run_sorted_columns_check_test(109, "nn", target_column_type::CATEGORICAL, {0});
  }

  void test_untranslated_columns_nn_3_cat() {
    run_sorted_columns_check_test(109, "nn", target_column_type::CATEGORICAL, {0, 1});
  }

  void test_untranslated_columns_ssss_1_cat() {
    run_sorted_columns_check_test(109, "ssss", target_column_type::CATEGORICAL, {1,3});
  }

  void test_untranslated_columns_ssss_2_cat() {
    run_sorted_columns_check_test(109, "ssss", target_column_type::CATEGORICAL, {0,1,2,3});
  }

  void test_untranslated_columns_dd_1_cat() {
    run_sorted_columns_check_test(109, "dd", target_column_type::CATEGORICAL, {1});
  }

  void test_untranslated_columns_dd_2_cat() {
    run_sorted_columns_check_test(109, "dd", target_column_type::CATEGORICAL, {0});
  }

  void test_untranslated_columns_dd_3_cat() {
    run_sorted_columns_check_test(109, "dd", target_column_type::CATEGORICAL, {0,1});
  }

  void test_untranslated_columns_v_1_cat() {
    run_sorted_columns_check_test(109, "v", target_column_type::CATEGORICAL, {0});
  }

  void test_untranslated_columns_many_1_cat() {
    run_sorted_columns_check_test(109, "cnsnscsnccccccccncss", target_column_type::CATEGORICAL,
                              {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});
  }

  void test_untranslated_columns_many_2_cat() {
    run_sorted_columns_check_test(109, "cnsnscsnccccccccncss", target_column_type::CATEGORICAL, {19});
  }

};

BOOST_FIXTURE_TEST_SUITE(_test_sorted_columns, test_sorted_columns)
BOOST_AUTO_TEST_CASE(test_sorted_columns_000) {
  test_sorted_columns::test_sorted_columns_000();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0n) {
  test_sorted_columns::test_sorted_columns_0n();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0b) {
  test_sorted_columns::test_sorted_columns_0b();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0c) {
  test_sorted_columns::test_sorted_columns_0c();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0C) {
  test_sorted_columns::test_sorted_columns_0C();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1b_unsorted) {
  test_sorted_columns::test_sorted_columns_1b_unsorted();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1) {
  test_sorted_columns::test_sorted_columns_1();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_2) {
  test_sorted_columns::test_sorted_columns_2();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_3) {
  test_sorted_columns::test_sorted_columns_3();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_4) {
  test_sorted_columns::test_sorted_columns_4();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_5) {
  test_sorted_columns::test_sorted_columns_5();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_6) {
  test_sorted_columns::test_sorted_columns_6();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_10) {
  test_sorted_columns::test_sorted_columns_10();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_11) {
  test_sorted_columns::test_sorted_columns_11();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_12) {
  test_sorted_columns::test_sorted_columns_12();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_13) {
  test_sorted_columns::test_sorted_columns_13();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_14) {
  test_sorted_columns::test_sorted_columns_14();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_15) {
  test_sorted_columns::test_sorted_columns_15();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_100) {
  test_sorted_columns::test_sorted_columns_100();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_1) {
  test_sorted_columns::test_untranslated_columns_nn_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_2) {
  test_sorted_columns::test_untranslated_columns_nn_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_3) {
  test_sorted_columns::test_untranslated_columns_nn_3();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_1) {
  test_sorted_columns::test_untranslated_columns_ssss_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_2) {
  test_sorted_columns::test_untranslated_columns_ssss_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_1) {
  test_sorted_columns::test_untranslated_columns_dd_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_2) {
  test_sorted_columns::test_untranslated_columns_dd_2();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_3) {
  test_sorted_columns::test_untranslated_columns_dd_3();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_v_1) {
  test_sorted_columns::test_untranslated_columns_v_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_1) {
  test_sorted_columns::test_untranslated_columns_many_1();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_2) {
  test_sorted_columns::test_untranslated_columns_many_2();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_000_tn) {
  test_sorted_columns::test_sorted_columns_000_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0n_tn) {
  test_sorted_columns::test_sorted_columns_0n_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0C_tn) {
  test_sorted_columns::test_sorted_columns_0C_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1_unsorted_tn) {
  test_sorted_columns::test_sorted_columns_1_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0b_tn) {
  test_sorted_columns::test_sorted_columns_0b_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1b_unsorted_tn) {
  test_sorted_columns::test_sorted_columns_1b_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1_tn) {
  test_sorted_columns::test_sorted_columns_1_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_2_tn) {
  test_sorted_columns::test_sorted_columns_2_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_3_tn) {
  test_sorted_columns::test_sorted_columns_3_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_4_tn) {
  test_sorted_columns::test_sorted_columns_4_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_5_tn) {
  test_sorted_columns::test_sorted_columns_5_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_6_tn) {
  test_sorted_columns::test_sorted_columns_6_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_10_tn) {
  test_sorted_columns::test_sorted_columns_10_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_11_tn) {
  test_sorted_columns::test_sorted_columns_11_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_12_tn) {
  test_sorted_columns::test_sorted_columns_12_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_13_tn) {
  test_sorted_columns::test_sorted_columns_13_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_14_tn) {
  test_sorted_columns::test_sorted_columns_14_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_15_tn) {
  test_sorted_columns::test_sorted_columns_15_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_100_tn) {
  test_sorted_columns::test_sorted_columns_100_tn();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_16_null_tn) {
  test_sorted_columns::test_sorted_columns_16_null_tn();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_1_num) {
  test_sorted_columns::test_untranslated_columns_nn_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_2_num) {
  test_sorted_columns::test_untranslated_columns_nn_2_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_3_num) {
  test_sorted_columns::test_untranslated_columns_nn_3_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_1_num) {
  test_sorted_columns::test_untranslated_columns_ssss_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_2_num) {
  test_sorted_columns::test_untranslated_columns_ssss_2_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_1_num) {
  test_sorted_columns::test_untranslated_columns_dd_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_2_num) {
  test_sorted_columns::test_untranslated_columns_dd_2_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_3_num) {
  test_sorted_columns::test_untranslated_columns_dd_3_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_v_1_num) {
  test_sorted_columns::test_untranslated_columns_v_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_1_num) {
  test_sorted_columns::test_untranslated_columns_many_1_num();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_2_num) {
  test_sorted_columns::test_untranslated_columns_many_2_num();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_000_tc) {
  test_sorted_columns::test_sorted_columns_000_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0n_tc) {
  test_sorted_columns::test_sorted_columns_0n_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0C_tc) {
  test_sorted_columns::test_sorted_columns_0C_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1_unsorted_tc) {
  test_sorted_columns::test_sorted_columns_1_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_0b_tc) {
  test_sorted_columns::test_sorted_columns_0b_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1b_unsorted_tc) {
  test_sorted_columns::test_sorted_columns_1b_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_1_tc) {
  test_sorted_columns::test_sorted_columns_1_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_2_tc) {
  test_sorted_columns::test_sorted_columns_2_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_3_tc) {
  test_sorted_columns::test_sorted_columns_3_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_4_tc) {
  test_sorted_columns::test_sorted_columns_4_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_5_tc) {
  test_sorted_columns::test_sorted_columns_5_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_6_tc) {
  test_sorted_columns::test_sorted_columns_6_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_10_tc) {
  test_sorted_columns::test_sorted_columns_10_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_11_tc) {
  test_sorted_columns::test_sorted_columns_11_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_12_tc) {
  test_sorted_columns::test_sorted_columns_12_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_13_tc) {
  test_sorted_columns::test_sorted_columns_13_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_14_tc) {
  test_sorted_columns::test_sorted_columns_14_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_15_tc) {
  test_sorted_columns::test_sorted_columns_15_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_100_tc) {
  test_sorted_columns::test_sorted_columns_100_tc();
}
BOOST_AUTO_TEST_CASE(test_sorted_columns_16_null_tc) {
  test_sorted_columns::test_sorted_columns_16_null_tc();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_1_cat) {
  test_sorted_columns::test_untranslated_columns_nn_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_2_cat) {
  test_sorted_columns::test_untranslated_columns_nn_2_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_nn_3_cat) {
  test_sorted_columns::test_untranslated_columns_nn_3_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_1_cat) {
  test_sorted_columns::test_untranslated_columns_ssss_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_ssss_2_cat) {
  test_sorted_columns::test_untranslated_columns_ssss_2_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_1_cat) {
  test_sorted_columns::test_untranslated_columns_dd_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_2_cat) {
  test_sorted_columns::test_untranslated_columns_dd_2_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_dd_3_cat) {
  test_sorted_columns::test_untranslated_columns_dd_3_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_v_1_cat) {
  test_sorted_columns::test_untranslated_columns_v_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_1_cat) {
  test_sorted_columns::test_untranslated_columns_many_1_cat();
}
BOOST_AUTO_TEST_CASE(test_untranslated_columns_many_2_cat) {
  test_sorted_columns::test_untranslated_columns_many_2_cat();
}
BOOST_AUTO_TEST_SUITE_END()
