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

struct test_reindexing  {
 public:

  // Test the block iterator by stress-testing a large number of
  // combinations of bounds, threads, sizes, and types

  enum class target_column_type {NONE, NUMERICAL, CATEGORICAL};

  void run_reindexing_check_test(size_t n, const std::string& run_string, target_column_type target_type) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);
    globals::set_global("TURI_ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD", 7);

    random::seed(0);

    sframe raw_data;
    ml_data data;

    bool target_column;

    if(target_type == target_column_type::CATEGORICAL) {
      target_column = true;
    } else if(target_type == target_column_type::NUMERICAL) {
      target_column = true;
    } else {
      target_column = false;
    }

    bool target_column_categorical = (target_type == target_column_type::CATEGORICAL);

    std::tie(raw_data, data) = make_random_sframe_and_ml_data(
        n, run_string, target_column, target_column_categorical);

    ASSERT_EQ(data.size(), raw_data.size());

    ml_data reindexed_data(data.metadata());
    reindexed_data.fill(raw_data);

    // Build reindex tables.
    std::vector<std::vector<size_t> > reindex_tables(data.num_columns() + (data.has_target() ? 1 : 0));

    for(size_t i = 0; i < data.metadata()->num_columns(); ++i) {
      reindex_tables[i].resize(data.metadata()->column_size(i));
      std::iota(reindex_tables[i].begin(), reindex_tables[i].end(), 0);

      if(data.metadata()->is_indexed(i))
        random::shuffle(reindex_tables[i]);
    }

    if(target_column) {
      reindex_tables.back().resize(data.metadata()->target_column_size());
      std::iota(reindex_tables.back().begin(), reindex_tables.back().end(), 0);

      if(data.metadata()->target_is_indexed())
        random::shuffle(reindex_tables.back());
    }

    // Do the reindexing.  It will be wrong, but we'll check it that way
    reindexed_data._reindex_blocks(reindex_tables);

    // Now,
    std::vector<ml_data_entry> x1, x2;

    ////////////////////////////////////////////////////////////////////////////////
    // Run the actual tests

    auto it_1 = data.get_iterator();
    auto it_2 = reindexed_data.get_iterator();

    for(; !it_1.done(); ++it_1, ++it_2) {

      it_1->fill(x1);
      it_2->fill(x2);

      DASSERT_EQ(x1.size(), x2.size());

      for(size_t ri = 0; ri < x1.size(); ++ri) {
        ASSERT_EQ(x2[ri].column_index, x1[ri].column_index);
        ASSERT_EQ(x2[ri].index, reindex_tables[x1[ri].column_index][x1[ri].index]);
      }

      if(target_column && target_type == target_column_type::CATEGORICAL) {
        ASSERT_EQ(it_2->target_index(), reindex_tables.back()[it_1->target_index()]);
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////


  void test_reindexing_000() {
    // All unique
    run_reindexing_check_test(0, "n", target_column_type::NONE);
  }

  void test_reindexing_0n() {
    // All unique
    run_reindexing_check_test(5, "n", target_column_type::NONE);
  }

  void test_reindexing_0b() {
    run_reindexing_check_test(5, "b", target_column_type::NONE);
  }

  void test_reindexing_0c() {
    // All unique
    run_reindexing_check_test(5, "c", target_column_type::NONE);
  }

  void test_reindexing_0C() {
    // All unique
    run_reindexing_check_test(5, "C", target_column_type::NONE);
  }

  void test_reindexing_1b_unsorted() {
    run_reindexing_check_test(13, "b", target_column_type::NONE);
  }

  void test_reindexing_1() {
    run_reindexing_check_test(13, "bc", target_column_type::NONE);
  }

  void test_reindexing_2() {
    run_reindexing_check_test(13, "zc", target_column_type::NONE);
  }

  void test_reindexing_3() {
    run_reindexing_check_test(100, "Zc", target_column_type::NONE);
  }

  void test_reindexing_4() {
    // Pretty much gonna be unique
    run_reindexing_check_test(100, "Cc", target_column_type::NONE);
  }

  void test_reindexing_5() {
    // 10 blocks of values.
    run_reindexing_check_test(1000, "Zc", target_column_type::NONE);
  }

  void test_reindexing_6() {
    // two large blocks of values
    run_reindexing_check_test(1000, "bc", target_column_type::NONE);
  }

  void test_reindexing_10() {
    // Yeah, a corner case
    run_reindexing_check_test(1, "bc", target_column_type::NONE);
  }

  void test_reindexing_11() {
    // One with just a lot of stuff
    run_reindexing_check_test(200, "u", target_column_type::NONE);
  }

  void test_reindexing_12() {
    // One with just a lot of stuff
    run_reindexing_check_test(200, "d", target_column_type::NONE);
  }

  void test_reindexing_13() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000, "cnv", target_column_type::NONE);
  }

  void test_reindexing_14() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000, "du", target_column_type::NONE);
  }

  void test_reindexing_15() {
    // One with just a lot of stuff
    run_reindexing_check_test(3, "UDccccV", target_column_type::NONE);
  }

  void test_reindexing_100() {
    // One with just a lot of stuff
    run_reindexing_check_test(10, "Zcuvd", target_column_type::NONE);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_reindexing_000_tn() {
    // All unique
    run_reindexing_check_test(0, "n", target_column_type::NUMERICAL);
  }

  void test_reindexing_0n_tn() {
    // All unique
    run_reindexing_check_test(5, "n", target_column_type::NUMERICAL);
  }

  void test_reindexing_0C_tn() {
    // All unique
    run_reindexing_check_test(5, "c", target_column_type::NUMERICAL);
  }

  void test_reindexing_1_unsorted_tn() {
    run_reindexing_check_test(5, "b", target_column_type::NUMERICAL);
  }

  void test_reindexing_0b_tn() {
    // All unique
    run_reindexing_check_test(13, "C", target_column_type::NUMERICAL);
  }

  void test_reindexing_1b_unsorted_tn() {
    run_reindexing_check_test(13, "b", target_column_type::NUMERICAL);
  }

  void test_reindexing_1_tn() {
    run_reindexing_check_test(13, "bc", target_column_type::NUMERICAL);
  }

  void test_reindexing_2_tn() {
    run_reindexing_check_test(13, "zc", target_column_type::NUMERICAL);
  }

  void test_reindexing_3_tn() {
    run_reindexing_check_test(100, "Zc", target_column_type::NUMERICAL);
  }

  void test_reindexing_4_tn() {
    // Pretty much gonna be unique
    run_reindexing_check_test(100, "Cc", target_column_type::NUMERICAL);
  }

  void test_reindexing_5_tn() {
    // 10 blocks of values.
    run_reindexing_check_test(1000, "Zc", target_column_type::NUMERICAL);
  }

  void test_reindexing_6_tn() {
    // two large blocks of values
    run_reindexing_check_test(1000, "bc", target_column_type::NUMERICAL);
  }

  void test_reindexing_10_tn() {
    // Yeah, a corner case
    run_reindexing_check_test(1, "bc", target_column_type::NUMERICAL);
  }

  void test_reindexing_11_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(200, "u", target_column_type::NUMERICAL);
  }

  void test_reindexing_12_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(200, "d", target_column_type::NUMERICAL);
  }

  void test_reindexing_13_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000, "cnv", target_column_type::NUMERICAL);
  }

  void test_reindexing_14_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000, "du", target_column_type::NUMERICAL);
  }

  void test_reindexing_15_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(3, "UDccccV", target_column_type::NUMERICAL);
  }

  void test_reindexing_100_tn() {
    // One with just a lot of stuff
    run_reindexing_check_test(10, "Zcuvd", target_column_type::NUMERICAL);
  }

  void test_reindexing_16_null_tn() {
    // two large blocks of values
    run_reindexing_check_test(1000, "", target_column_type::NUMERICAL);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // All the ones with targets

  void test_reindexing_000_tc() {
    // All unique
    run_reindexing_check_test(0, "n", target_column_type::CATEGORICAL);
  }

  void test_reindexing_0n_tc() {
    // All unique
    run_reindexing_check_test(5, "n", target_column_type::CATEGORICAL);
  }

  void test_reindexing_0C_tc() {
    // All unique
    run_reindexing_check_test(5, "c", target_column_type::CATEGORICAL);
  }

  void test_reindexing_1_unsorted_tc() {
    run_reindexing_check_test(5, "b", target_column_type::CATEGORICAL);
  }

  void test_reindexing_0b_tc() {
    // All unique
    run_reindexing_check_test(13, "C", target_column_type::CATEGORICAL);
  }

  void test_reindexing_1b_unsorted_tc() {
    run_reindexing_check_test(13, "b", target_column_type::CATEGORICAL);
  }

  void test_reindexing_1_tc() {
    run_reindexing_check_test(13, "bc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_2_tc() {
    run_reindexing_check_test(13, "zc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_3_tc() {
    run_reindexing_check_test(100, "Zc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_4_tc() {
    // Pretty much gonna be unique
    run_reindexing_check_test(100, "Cc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_5_tc() {
    // 10 blocks of values.
    run_reindexing_check_test(1000, "Zc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_6_tc() {
    // two large blocks of values
    run_reindexing_check_test(1000, "bc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_10_tc() {
    // Yeah, a corner case
    run_reindexing_check_test(1, "bc", target_column_type::CATEGORICAL);
  }

  void test_reindexing_11_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(200, "u", target_column_type::CATEGORICAL);
  }

  void test_reindexing_12_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(200, "d", target_column_type::CATEGORICAL);
  }

  void test_reindexing_13_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000, "cnv", target_column_type::CATEGORICAL);
  }

  void test_reindexing_14_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(1000, "du", target_column_type::CATEGORICAL);
  }

  void test_reindexing_15_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(3, "UDccccV", target_column_type::CATEGORICAL);
  }

  void test_reindexing_100_tc() {
    // One with just a lot of stuff
    run_reindexing_check_test(10, "Zcuvd", target_column_type::CATEGORICAL);
  }

  void test_reindexing_16_null_tc() {
    // two large blocks of values
    run_reindexing_check_test(1000, "", target_column_type::CATEGORICAL);
  }

};

BOOST_FIXTURE_TEST_SUITE(_test_reindexing, test_reindexing)
BOOST_AUTO_TEST_CASE(test_reindexing_000) {
  test_reindexing::test_reindexing_000();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0n) {
  test_reindexing::test_reindexing_0n();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0b) {
  test_reindexing::test_reindexing_0b();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0c) {
  test_reindexing::test_reindexing_0c();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0C) {
  test_reindexing::test_reindexing_0C();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1b_unsorted) {
  test_reindexing::test_reindexing_1b_unsorted();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1) {
  test_reindexing::test_reindexing_1();
}
BOOST_AUTO_TEST_CASE(test_reindexing_2) {
  test_reindexing::test_reindexing_2();
}
BOOST_AUTO_TEST_CASE(test_reindexing_3) {
  test_reindexing::test_reindexing_3();
}
BOOST_AUTO_TEST_CASE(test_reindexing_4) {
  test_reindexing::test_reindexing_4();
}
BOOST_AUTO_TEST_CASE(test_reindexing_5) {
  test_reindexing::test_reindexing_5();
}
BOOST_AUTO_TEST_CASE(test_reindexing_6) {
  test_reindexing::test_reindexing_6();
}
BOOST_AUTO_TEST_CASE(test_reindexing_10) {
  test_reindexing::test_reindexing_10();
}
BOOST_AUTO_TEST_CASE(test_reindexing_11) {
  test_reindexing::test_reindexing_11();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12) {
  test_reindexing::test_reindexing_12();
}
BOOST_AUTO_TEST_CASE(test_reindexing_13) {
  test_reindexing::test_reindexing_13();
}
BOOST_AUTO_TEST_CASE(test_reindexing_14) {
  test_reindexing::test_reindexing_14();
}
BOOST_AUTO_TEST_CASE(test_reindexing_15) {
  test_reindexing::test_reindexing_15();
}
BOOST_AUTO_TEST_CASE(test_reindexing_100) {
  test_reindexing::test_reindexing_100();
}
BOOST_AUTO_TEST_CASE(test_reindexing_000_tn) {
  test_reindexing::test_reindexing_000_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0n_tn) {
  test_reindexing::test_reindexing_0n_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0C_tn) {
  test_reindexing::test_reindexing_0C_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_unsorted_tn) {
  test_reindexing::test_reindexing_1_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0b_tn) {
  test_reindexing::test_reindexing_0b_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1b_unsorted_tn) {
  test_reindexing::test_reindexing_1b_unsorted_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_tn) {
  test_reindexing::test_reindexing_1_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_2_tn) {
  test_reindexing::test_reindexing_2_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_3_tn) {
  test_reindexing::test_reindexing_3_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_4_tn) {
  test_reindexing::test_reindexing_4_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_5_tn) {
  test_reindexing::test_reindexing_5_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_6_tn) {
  test_reindexing::test_reindexing_6_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_10_tn) {
  test_reindexing::test_reindexing_10_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_11_tn) {
  test_reindexing::test_reindexing_11_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12_tn) {
  test_reindexing::test_reindexing_12_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_13_tn) {
  test_reindexing::test_reindexing_13_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_14_tn) {
  test_reindexing::test_reindexing_14_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_15_tn) {
  test_reindexing::test_reindexing_15_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_100_tn) {
  test_reindexing::test_reindexing_100_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_16_null_tn) {
  test_reindexing::test_reindexing_16_null_tn();
}
BOOST_AUTO_TEST_CASE(test_reindexing_000_tc) {
  test_reindexing::test_reindexing_000_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0n_tc) {
  test_reindexing::test_reindexing_0n_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0C_tc) {
  test_reindexing::test_reindexing_0C_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_unsorted_tc) {
  test_reindexing::test_reindexing_1_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_0b_tc) {
  test_reindexing::test_reindexing_0b_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1b_unsorted_tc) {
  test_reindexing::test_reindexing_1b_unsorted_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_1_tc) {
  test_reindexing::test_reindexing_1_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_2_tc) {
  test_reindexing::test_reindexing_2_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_3_tc) {
  test_reindexing::test_reindexing_3_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_4_tc) {
  test_reindexing::test_reindexing_4_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_5_tc) {
  test_reindexing::test_reindexing_5_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_6_tc) {
  test_reindexing::test_reindexing_6_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_10_tc) {
  test_reindexing::test_reindexing_10_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_11_tc) {
  test_reindexing::test_reindexing_11_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_12_tc) {
  test_reindexing::test_reindexing_12_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_13_tc) {
  test_reindexing::test_reindexing_13_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_14_tc) {
  test_reindexing::test_reindexing_14_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_15_tc) {
  test_reindexing::test_reindexing_15_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_100_tc) {
  test_reindexing::test_reindexing_100_tc();
}
BOOST_AUTO_TEST_CASE(test_reindexing_16_null_tc) {
  test_reindexing::test_reindexing_16_null_tc();
}
BOOST_AUTO_TEST_SUITE_END()
