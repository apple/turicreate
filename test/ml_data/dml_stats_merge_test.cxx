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
using namespace turi::ml_data_internal;

struct test_stats_merge  {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  enum class target_column_type {NONE, NUMERICAL, CATEGORICAL};

  void run_reindexing_check_test(size_t n1, size_t n2, const std::string& run_string, target_column_type target_type) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);
    globals::set_global("TURI_ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD", 7);

    random::seed(0);

    sframe raw_data_v[3];
    ml_data data_v[3];

    std::shared_ptr<ml_metadata> mv[3];

    bool target_column;

    if(target_type == target_column_type::CATEGORICAL) {
      target_column = true;
    } else if(target_type == target_column_type::NUMERICAL) {
      target_column = true;
    } else {
      target_column = false;
    }

    bool target_column_categorical = (target_type == target_column_type::CATEGORICAL);

    std::tie(raw_data_v[0], data_v[0]) = make_random_sframe_and_ml_data(
        n1, run_string, target_column, target_column_categorical);

    std::tie(raw_data_v[1], data_v[1]) = make_random_sframe_and_ml_data(
        n2, run_string, target_column, target_column_categorical);

    raw_data_v[2] = raw_data_v[0].append(raw_data_v[1]);

    // Override so integer columns are categorical
    std::map<std::string, turi::ml_column_mode> mode_control;

    mv[0] = data_v[0].metadata();
    mv[1] = data_v[1].metadata();

    for(size_t i = 0; i < mv[0]->num_columns(); ++i) {
      mode_control[mv[0]->column_name(i)] = mv[0]->column_mode(i);
    }

    if(target_column)
      mode_control[mv[0]->target_column_name()] = mv[0]->target_column_mode();

    data_v[2].fill(raw_data_v[2], target_column ? "target" : "", mode_control);

    DASSERT_EQ(data_v[2].num_rows(), data_v[0].num_rows() + data_v[1].num_rows());

    mv[2] = data_v[2].metadata();

    size_t num_columns = mv[0]->num_columns();

    // Build reindex maps for data_v[1], as it differs from data_v[2].

    std::vector<std::vector<size_t> > reindex_maps(num_columns + (target_column ? 1 : 0));

    auto get_reindex_map = [](std::shared_ptr<column_indexer> idxr_new,
                              std::shared_ptr<column_indexer> idxr_old) {

      size_t n = idxr_old->indexed_column_size();

      std::vector<size_t> reindex_map(n);

      for(size_t i = 0; i < n; ++i) {
        reindex_map[i] =
            idxr_new->immutable_map_value_to_index(
                idxr_old->map_index_to_value(i));
      }

      return reindex_map;
    };


    // Reindex to be against 2, which has the authoritative index.

    for(size_t didx : {0, 1}) {

      for(size_t column_idx = 0; column_idx < num_columns; ++column_idx) {

        if(mv[didx]->is_indexed(column_idx) ) {
          auto idxr_new = mv[2]->indexer(column_idx);
          auto idxr_old = mv[didx]->indexer(column_idx);

          reindex_maps[column_idx] = get_reindex_map(idxr_new, idxr_old);

        } else {

          reindex_maps[column_idx].resize(mv[didx]->column_size(column_idx) );
          std::iota(reindex_maps[column_idx].begin(), reindex_maps[column_idx].end(), 0);
        }
      }

      if(target_column) {
        if(mv[didx]->target_is_indexed()) {
          auto idxr_new = mv[2]->target_indexer();
          auto idxr_old = mv[didx]->target_indexer();

          reindex_maps.back() = get_reindex_map(idxr_new, idxr_old);
        } else {
          reindex_maps.back().resize(mv[didx]->target_column_size() );
          std::iota(reindex_maps.back().begin(), reindex_maps.back().end(), 0);
        }
      }

      // Now, reindex the statistics.
      for(size_t column_idx = 0; column_idx < num_columns; ++column_idx) {
        if(mv[didx]->is_indexed(column_idx)) {
          mv[didx]->statistics(column_idx)->reindex(
              reindex_maps[column_idx], mv[2]->column_size(column_idx));
        }
      }

      if(target_column && mv[didx]->target_is_indexed()) {
        mv[didx]->target_statistics()->reindex(
            reindex_maps.back(), mv[2]->target_column_size());
      }
    }

    // Now, we merge statistics.
    for(size_t column_idx = 0; column_idx < num_columns; ++column_idx) {
      mv[1]->statistics(column_idx)->merge_in(
          std::move(*(mv[0]->statistics(column_idx).get())));
    }

    if(target_column) {
      mv[1]->target_statistics()->merge_in(
          std::move(*(mv[0]->target_statistics().get())));
    }

    // Now, we simply test that everything is correct.
    for(size_t column_idx = 0; column_idx < num_columns; ++column_idx) {
      for(size_t i = 0; i < mv[2]->column_size(column_idx); ++i) {
        ASSERT_EQ(mv[2]->statistics(column_idx)->count(i), mv[1]->statistics(column_idx)->count(i));
      }
    }

    // Now, we simply test that everything is correct.
    for(size_t column_idx = 0; column_idx < num_columns; ++column_idx) {
      for(size_t i = 0; i < mv[2]->column_size(column_idx); ++i) {
        double m1 = mv[1]->statistics(column_idx)->mean(i);
        double m2 = mv[2]->statistics(column_idx)->mean(i);
        ASSERT_DELTA(m1, m2, 1e-8);
      }
    }

    // Now, we simply test that everything is correct.
    for(size_t column_idx = 0; column_idx < num_columns; ++column_idx) {
      for(size_t i = 0; i < mv[2]->column_size(column_idx); ++i) {
        double s1 = mv[1]->statistics(column_idx)->stdev(i);
        double s2 = mv[2]->statistics(column_idx)->stdev(i);
        ASSERT_DELTA(s1, s2, 1e-8);
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////


  void test_reindexing_000() {
    // All unique
    run_reindexing_check_test(0, 0, "n", target_column_type::NONE);
  }

  void test_reindexing_0n() {
    // All unique
    run_reindexing_check_test(5, 5, "n", target_column_type::NONE);
  }

  void test_reindexing_0n2() {
    // All unique
    run_reindexing_check_test(5, 1, "n", target_column_type::NONE);
  }

  void test_reindexing_0b() {
    run_reindexing_check_test(5, 5, "b", target_column_type::NONE);
  }

  void test_reindexing_0c() {
    // All unique
    run_reindexing_check_test(5,5, "c", target_column_type::NONE);
  }

  void test_reindexing_0C() {
    // All unique
    run_reindexing_check_test(5,5, "C", target_column_type::NONE);
  }

  void test_reindexing_1b_unsorted() {
    run_reindexing_check_test(13,13, "b", target_column_type::NONE);
  }

  void test_reindexing_1() {
    run_reindexing_check_test(13,13, "bc", target_column_type::NONE);
  }

  void test_reindexing_2() {
    run_reindexing_check_test(13,13, "zc", target_column_type::NONE);
  }

  void test_reindexing_3() {
    run_reindexing_check_test(100,100, "Zc", target_column_type::NONE);
  }

  void test_reindexing_4() {
    // Pretty much gonna be unique
    run_reindexing_check_test(100,100, "Cc", target_column_type::NONE);
  }

  void test_reindexing_5() {
    // 10 blocks of values.
    run_reindexing_check_test(1000,1000, "Zc", target_column_type::NONE);
  }

  void test_reindexing_6() {
    // two large blocks of values
    run_reindexing_check_test(1000,1000, "bc", target_column_type::NONE);
  }

  void test_reindexing_10() {
    // Yeah, a corner case
    run_reindexing_check_test(1,1, "bc", target_column_type::NONE);
  }

  void test_reindexing_11() {
    // One with just a lot of stuff
    run_reindexing_check_test(200,200, "u", target_column_type::NONE);
  }

  void test_reindexing_12() {
    // One with just a lot of stuff
    run_reindexing_check_test(200,200, "d", target_column_type::NONE);
  }

  void test_reindexing_12a() {
    // One with just a lot of stuff
    run_reindexing_check_test(3,3, "zn", target_column_type::NONE);
  }

  void test_reindexing_12b() {
    // One with just a lot of stuff
    run_reindexing_check_test(2,2, "d", target_column_type::NONE);
  }

  void test_reindexing_13() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000,1000, "cnv", target_column_type::NONE);
  }

  void test_reindexing_14() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000,1000, "du", target_column_type::NONE);
  }

  void test_reindexing_15() {
    // One with just a lot of stuff
    run_reindexing_check_test(3,3, "UDccccV", target_column_type::NONE);
  }

  void test_reindexing_100() {
    // One with just a lot of stuff
    run_reindexing_check_test(10,10, "Zcuvd", target_column_type::NONE);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_reindexing_000_tn() {
    // All unique
    run_reindexing_check_test(0,0, "n", target_column_type::NUMERICAL);
  }

  void test_reindexing_0n_tn() {
    // All unique
    run_reindexing_check_test(5,5, "n", target_column_type::NUMERICAL);
  }

  void test_reindexing_0C_tn() {
    // All unique
    run_reindexing_check_test(5,5, "c", target_column_type::NUMERICAL);
  }

  void test_reindexing_1_unsorted_tn() {
    run_reindexing_check_test(5,5, "b", target_column_type::NUMERICAL);
  }

  void test_reindexing_0b_tn() {
    // All unique
    run_reindexing_check_test(13,13, "C", target_column_type::NUMERICAL);
  }

  void test_reindexing_1b_unsorted_tn() {
    run_reindexing_check_test(13,13, "b", target_column_type::NUMERICAL);
  }

  void test_reindexing_1_tn() {
    run_reindexing_check_test(13,13, "bc", target_column_type::NUMERICAL);
  }

  void test_reindexing_2_tn() {
    run_reindexing_check_test(13,13, "zc", target_column_type::NUMERICAL);
  }

  void test_reindexing_3_tn() {
    run_reindexing_check_test(100,100, "Zc", target_column_type::NUMERICAL);
  }

  void test_reindexing_4_tn() {
    // Pretty much gonna be unique
    run_reindexing_check_test(100,100, "Cc", target_column_type::NUMERICAL);
  }

  void test_reindexing_5_tn() {
    // 10 blocks of values.
    run_reindexing_check_test(1000,1000, "Zc", target_column_type::NUMERICAL);
  }

  void test_reindexing_6_tn() {
    // two large blocks of values
    run_reindexing_check_test(1000,1000, "bc", target_column_type::NUMERICAL);
  }

  void test_reindexing_10_tn() {
    // Yeah, a corner case
    run_reindexing_check_test(1,1, "bc", target_column_type::NUMERICAL);
  }

  void test_reindexing_11_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(200,200, "u", target_column_type::NUMERICAL);
  }

  void test_reindexing_12_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(200,200, "d", target_column_type::NUMERICAL);
  }

  void test_reindexing_13_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000,1000, "cnv", target_column_type::NUMERICAL);
  }

  void test_reindexing_14_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000,1000, "du", target_column_type::NUMERICAL);
  }

  void test_reindexing_15_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(3,3, "UDccccV", target_column_type::NUMERICAL);
  }

  void test_reindexing_100_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(10,10, "Zcuvd", target_column_type::NUMERICAL);
  }

  void test_reindexing_16_null_tn() {
    // two large blocks of values
    run_reindexing_check_test(1000,1000, "", target_column_type::NUMERICAL);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_reindexing_000_tc() {
    // All unique
    run_reindexing_check_test(0,0, "n", target_column_type::CATEGORICAL);
  }

  void test_reindexing_0n_tc() {
    // All unique
    run_reindexing_check_test(5,5, "n", target_column_type::CATEGORICAL);
  }

  void test_reindexing_0C_tc() {
    // All unique
    run_reindexing_check_test(5,5, "c", target_column_type::CATEGORICAL);
  }

  void test_reindexing_1_unsorted_tc() {
    run_reindexing_check_test(5,5, "b", target_column_type::CATEGORICAL);
  }

  void test_reindexing_0b_tc() {
    // All unique
    run_reindexing_check_test(13,13, "C", target_column_type::CATEGORICAL);
  }

  void test_reindexing_1b_unsorted_tc() {
    run_reindexing_check_test(13,13, "b", target_column_type::CATEGORICAL);
  }

  void test_reindexing_1_tc() {
    run_reindexing_check_test(13,13, "bc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_2_tc() {
    run_reindexing_check_test(13,13, "zc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_3_tc() {
    run_reindexing_check_test(100,100, "Zc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_4_tc() {
    // Pretty much gonna be unique
    run_reindexing_check_test(100,100, "Cc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_5_tc() {
    // 10 blocks of values.
    run_reindexing_check_test(1000,1000, "Zc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_6_tc() {
    // two large blocks of values
    run_reindexing_check_test(1000,1000, "bc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_10_tc() {
    // Yeah, a corner case
    run_reindexing_check_test(1,1, "bc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_11_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(200,200, "u", target_column_type::CATEGORICAL);
  }

  void test_reindexing_12_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(200,200, "d", target_column_type::CATEGORICAL);
  }

  void test_reindexing_13_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000,1000, "cnv", target_column_type::CATEGORICAL);
  }

  void test_reindexing_14_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000,1000, "du", target_column_type::CATEGORICAL);
  }

  void test_reindexing_15_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(3,3, "UDccccV", target_column_type::CATEGORICAL);
  }

  void test_reindexing_100_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(10,10, "Zcuvd", target_column_type::CATEGORICAL);
  }

  void test_reindexing_16_null_tc() {
    // two large blocks of values
    run_reindexing_check_test(1000,1000, "", target_column_type::CATEGORICAL);
  }

};

BOOST_FIXTURE_TEST_SUITE(_test_stats_merge, test_stats_merge)
BOOST_AUTO_TEST_CASE(test_reindexing_000) {
  test_stats_merge::test_reindexing_000();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0n) {
  test_stats_merge::test_reindexing_0n();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0n2) {
  test_stats_merge::test_reindexing_0n2();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0b) {
  test_stats_merge::test_reindexing_0b();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0c) {
  test_stats_merge::test_reindexing_0c();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0C) {
  test_stats_merge::test_reindexing_0C();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1b_unsorted) {
  test_stats_merge::test_reindexing_1b_unsorted();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1) {
  test_stats_merge::test_reindexing_1();
}
BOOST_AUTO_TEST_CASE(test_reindexing_2) {
  test_stats_merge::test_reindexing_2();
}
BOOST_AUTO_TEST_CASE(test_reindexing_3) {
  test_stats_merge::test_reindexing_3();
}
BOOST_AUTO_TEST_CASE(test_reindexing_4) {
  test_stats_merge::test_reindexing_4();
}
BOOST_AUTO_TEST_CASE(test_reindexing_5) {
  test_stats_merge::test_reindexing_5();
}
BOOST_AUTO_TEST_CASE(test_reindexing_6) {
  test_stats_merge::test_reindexing_6();
}
BOOST_AUTO_TEST_CASE(test_reindexing_10) {
  test_stats_merge::test_reindexing_10();
}
BOOST_AUTO_TEST_CASE(test_reindexing_11) {
  test_stats_merge::test_reindexing_11();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12) {
  test_stats_merge::test_reindexing_12();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12a) {
  test_stats_merge::test_reindexing_12a();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12b) {
  test_stats_merge::test_reindexing_12b();
}
BOOST_AUTO_TEST_CASE(test_reindexing_13) {
  test_stats_merge::test_reindexing_13();
}
BOOST_AUTO_TEST_CASE(test_reindexing_14) {
  test_stats_merge::test_reindexing_14();
}
BOOST_AUTO_TEST_CASE(test_reindexing_15) {
  test_stats_merge::test_reindexing_15();
}
BOOST_AUTO_TEST_CASE(test_reindexing_100) {
  test_stats_merge::test_reindexing_100();
}
BOOST_AUTO_TEST_CASE(test_reindexing_000_tn) {
  test_stats_merge::test_reindexing_000_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0n_tn) {
  test_stats_merge::test_reindexing_0n_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0C_tn) {
  test_stats_merge::test_reindexing_0C_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_unsorted_tn) {
  test_stats_merge::test_reindexing_1_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0b_tn) {
  test_stats_merge::test_reindexing_0b_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1b_unsorted_tn) {
  test_stats_merge::test_reindexing_1b_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_tn) {
  test_stats_merge::test_reindexing_1_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_2_tn) {
  test_stats_merge::test_reindexing_2_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_3_tn) {
  test_stats_merge::test_reindexing_3_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_4_tn) {
  test_stats_merge::test_reindexing_4_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_5_tn) {
  test_stats_merge::test_reindexing_5_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_6_tn) {
  test_stats_merge::test_reindexing_6_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_10_tn) {
  test_stats_merge::test_reindexing_10_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_11_tn) {
  test_stats_merge::test_reindexing_11_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12_tn) {
  test_stats_merge::test_reindexing_12_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_13_tn) {
  test_stats_merge::test_reindexing_13_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_14_tn) {
  test_stats_merge::test_reindexing_14_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_15_tn) {
  test_stats_merge::test_reindexing_15_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_100_tn) {
  test_stats_merge::test_reindexing_100_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_16_null_tn) {
  test_stats_merge::test_reindexing_16_null_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_000_tc) {
  test_stats_merge::test_reindexing_000_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0n_tc) {
  test_stats_merge::test_reindexing_0n_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0C_tc) {
  test_stats_merge::test_reindexing_0C_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_unsorted_tc) {
  test_stats_merge::test_reindexing_1_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0b_tc) {
  test_stats_merge::test_reindexing_0b_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1b_unsorted_tc) {
  test_stats_merge::test_reindexing_1b_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_tc) {
  test_stats_merge::test_reindexing_1_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_2_tc) {
  test_stats_merge::test_reindexing_2_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_3_tc) {
  test_stats_merge::test_reindexing_3_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_4_tc) {
  test_stats_merge::test_reindexing_4_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_5_tc) {
  test_stats_merge::test_reindexing_5_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_6_tc) {
  test_stats_merge::test_reindexing_6_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_10_tc) {
  test_stats_merge::test_reindexing_10_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_11_tc) {
  test_stats_merge::test_reindexing_11_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12_tc) {
  test_stats_merge::test_reindexing_12_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_13_tc) {
  test_stats_merge::test_reindexing_13_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_14_tc) {
  test_stats_merge::test_reindexing_14_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_15_tc) {
  test_stats_merge::test_reindexing_15_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_100_tc) {
  test_stats_merge::test_reindexing_100_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_16_null_tc) {
  test_stats_merge::test_reindexing_16_null_tc();
}
BOOST_AUTO_TEST_SUITE_END()
