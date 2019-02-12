#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <util/cityhash_tc.hpp>
#include <cmath>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <unity/lib/flex_dict_view.hpp>

// ML-Data Utils
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <sframe/sframe_iterators.hpp>
#include <unity/toolkits/ml_data_2/ml_data_entry.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>

// Testing utils common to all of ml_data
#include <unity/toolkits/ml_data_2/testing_utils.hpp>

#include <globals/globals.hpp>


using namespace turi;

namespace std {

// Define this so we can use unordered sets of vectors.
template<>
struct hash<std::vector<turi::flexible_type> > {
  inline size_t operator()(const std::vector<turi::flexible_type>& s) const {
    size_t h = turi::hash64(s.size());
    for(const turi::flexible_type& f : s)
      h = turi::hash64_combine(h, f.hash());
    return h;
  }
};

}


typedef Eigen::Matrix<double,Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

using namespace turi;

////////////////////////////////////////////////////////////////////////////////

struct test_subsample  {

 public:
  ////////////////////////////////////////////////////////////////////////////////

  void _run_subsampling_test(size_t n,
                             const std::string& run_string,
                             bool target_column = false,
                             bool test_sorting = false) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);

    sframe raw_data;
    v2::ml_data data;

    std::map<std::string, flexible_type> creation_options;

    if(test_sorting)
      creation_options["sort_by_first_two_columns"] = true;

    std::tie(raw_data, data) = v2::make_random_sframe_and_ml_data(
        n, run_string, target_column, creation_options);

    // Build a set of all the original ones to make sure nothing is
    // changed or anything
    std::set<std::pair<uint128_t, double> > row_hashes;

    auto row_hash = [&](const v2::ml_data_iterator& it) {
      std::vector<v2::ml_data_entry> x;
      it.fill_observation(x);

      return std::make_pair(
          hash128(data.translate_row_to_original(x)), it.target_value());
    };

    for(auto it = data.get_iterator(); !it.done(); ++it)
      row_hashes.insert(row_hash(it));

    for( size_t n_rows : std::vector<size_t>{0, 1, 7, n / 8, n / 2, n - 1, n}) {

      v2::ml_data ss_data = data.create_subsampled_copy(n_rows, 0);

      ASSERT_EQ(ss_data.size(), n_rows);

      std::vector<v2::ml_data_entry> x;

      size_t count = 0;

      // Just need to verify they are indeed sorted still.

      size_t last_col_1_idx = 0;
      size_t last_col_2_idx = 0;

      for(auto it = ss_data.get_iterator(); !it.done(); ++it) {
        ++count;
        ASSERT_TRUE(row_hashes.count(row_hash(it)) != 0);

        if(test_sorting) {
          it.fill_observation(x);

          size_t col_1_idx = x[0].index;
          size_t col_2_idx = x[1].index;

          ASSERT_LE(last_col_1_idx, col_1_idx);

          if(col_1_idx == last_col_1_idx) {
            ASSERT_LE(last_col_2_idx, col_2_idx);
          }

          last_col_1_idx = col_1_idx;
          last_col_2_idx = col_2_idx;
        }
      }

      ASSERT_EQ(ss_data.size(), count);
    }
  }

  void test_subsampling_1() {
    // Seven is just one complete block size
    _run_subsampling_test(7, "n");
  }

  void test_subsampling_1b() {
    _run_subsampling_test(7, "cDUV");
  }

  void test_subsampling_2() {
    _run_subsampling_test(1000, "cc", false, true);
  }

  void test_subsampling_3() {
    _run_subsampling_test(500, "ccnduv", false, true);
  }

  void test_subsampling_3b() {
    _run_subsampling_test(5000, "ccu", false, true);
  }

  void test_subsampling_4() {
    _run_subsampling_test(10001, "cn");
  }


  ////////////////////////////////////////////////////////////////////////////////
  // With targets

  void test_subsampling_1_t() {
    // Seven is just one complete block size
    _run_subsampling_test(7, "n", true);
  }

  void test_subsampling_1b_t() {
    _run_subsampling_test(7, "cDUV", true);
  }

  void test_subsampling_2_t() {
    _run_subsampling_test(1000, "cc", true, true);
  }

  void test_subsampling_3_t() {
    _run_subsampling_test(500, "ccnduv", true, true);
  }

  void test_subsampling_3b_t() {
    _run_subsampling_test(5000, "ccu", true, true);
  }

  void test_subsampling_4_t() {
    _run_subsampling_test(10001, "cn", true);
  }
};

struct test_shuffle  {

 public:
  ////////////////////////////////////////////////////////////////////////////////

  void _run_shuffling_test(size_t n, const std::string& run_string,
                           bool target_column = false) {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 29);

    sframe raw_data;
    v2::ml_data data_1;

    std::map<std::string, flexible_type>
        creation_options = { {"shuffle_rows", true} };

    std::tie(raw_data, data_1) = v2::make_random_sframe_and_ml_data(
        n, run_string, target_column, creation_options);

    v2::ml_data data_2(data_1.metadata());
    data_2.fill(raw_data);

    // Build a set of all the original ones to make sure nothing is dropped
    std::unordered_multiset<std::vector<flexible_type> >
        known_elements, test_elements_1, test_elements_2;

    for(parallel_sframe_iterator it(raw_data); !it.done(); ++it) {
      std::vector<flexible_type> row;

      for(size_t i = 0; i < raw_data.num_columns(); ++i)
        row.push_back(it.value(i));

      known_elements.insert(row);
    }

    ASSERT_EQ(data_1.size(), data_2.size());

    bool is_same = true;

    // Just make sure the two are different and no rows are dropped

    auto it_1 = data_1.get_iterator();
    auto it_2 = data_2.get_iterator();

    std::vector<v2::ml_data_entry> x_1;
    std::vector<v2::ml_data_entry> x_2;

    for(; !it_1.done(); ++it_1, ++it_2) {
      ASSERT_FALSE(it_2.done());

      it_1.fill_observation(x_1);
      it_2.fill_observation(x_2);

      std::vector<flexible_type> x_raw_1 = data_1.translate_row_to_original(x_1);
      std::vector<flexible_type> x_raw_2 = data_2.translate_row_to_original(x_2);

      if(target_column) {
        x_raw_1.push_back(flex_int(it_1.target_value()));
        x_raw_2.push_back(flex_int(it_2.target_value()));
      }

      if(x_raw_1 != x_raw_2)
        is_same = false;

      test_elements_1.insert(x_raw_1);
      test_elements_2.insert(x_raw_2);
    }
    ASSERT_TRUE(it_2.done());

    // Make sure we can be certain that they are not randomly the same
    if(data_1.size() > 14)
      ASSERT_FALSE(is_same);

    ASSERT_TRUE(test_elements_1 == known_elements);
    ASSERT_TRUE(test_elements_2 == known_elements);
  }

  void test_shuffling_1() {
    // Seven is just one complete block size
    _run_shuffling_test(7, "n");
  }

  void test_shuffling_1b() {
    _run_shuffling_test(7, "cc");
  }


  void test_shuffling_1c() {
    _run_shuffling_test(7, "cDUV");
  }

  void test_shuffling_2() {
    _run_shuffling_test(1000, "cc");
  }

  void test_shuffling_3() {
    _run_shuffling_test(500, "ccnduv");
  }

  void test_shuffling_3b() {
    _run_shuffling_test(5000, "ccu");
  }

  void test_shuffling_4() {
    _run_shuffling_test(10001, "cn");
  }


  ////////////////////////////////////////////////////////////////////////////////
  // With targets

  void test_shuffling_1_t() {
    // Seven is just one complete block size
    _run_shuffling_test(7, "n", true);
  }

  void test_shuffling_1b_t() {
    _run_shuffling_test(7, "cDUV", true);
  }

  void test_shuffling_2_t() {
    _run_shuffling_test(1000, "cc", true);
  }

  void test_shuffling_3_t() {
    _run_shuffling_test(500, "ccnduv", true);
  }

  void test_shuffling_3b_t() {
    _run_shuffling_test(5000, "ccu", true);
  }

  void test_shuffling_4_t() {
    _run_shuffling_test(10001, "cn", true);
  }
};

struct test_selection  {
 public:

  std::vector<size_t> extract_indices(const v2::ml_data& m) const {
    std::vector<size_t> out;
    std::vector<v2::ml_data_entry> x;
    for(auto it = m.get_iterator(); !it.done(); ++it) {
      it.fill_observation(x);
      out.push_back(x[0].value);
    }

    return out;
  }

  void test_simple() {

    globals::set_global("TURI_ML_DATA_TARGET_ROW_BYTE_MINIMUM", 16);

    sframe X = make_integer_testing_sframe( {"C0"}, { {0}, {1}, {2}, {3}, {4}, {5}, {6} } );
    v2::ml_data m;
    m.fill(X);

    {
      std::vector<size_t> ref = {1, 4, 6};
      v2::ml_data m2 = m.select_rows(ref);

      std::vector<size_t> rows = extract_indices(m2);

      TS_ASSERT(rows == ref);
    }


    {
      std::vector<size_t> ref = {0,1,2,3};
      v2::ml_data m2 = m.select_rows(ref);

      std::vector<size_t> rows = extract_indices(m2);

      TS_ASSERT(rows == ref);
    }

    {
      std::vector<size_t> ref = {};
      v2::ml_data m2 = m.select_rows(ref);

      std::vector<size_t> rows = extract_indices(m2);

      TS_ASSERT(rows == ref);
    }

    {
      std::vector<size_t> ref = {0, 0, 1, 1, 2, 2, 2, 3, 3, 4, 4,4,  5, 5, 6};
      v2::ml_data m2 = m.select_rows(ref);

      std::vector<size_t> rows = extract_indices(m2);

      TS_ASSERT(rows == ref);
    }
  }

  void test_random() {

    std::vector<std::vector<size_t> > base;

    for(size_t i = 0; i < 200; ++i) {
      base.push_back( {i} );
    }

    sframe X = make_integer_testing_sframe({"C1"}, base);

    v2::ml_data m;
    m.fill(X);

    std::vector<size_t> pull_indices;

    for(size_t i = 0; i < 500; ++i) {

      pull_indices.push_back(random::fast_uniform<size_t>(0, 199));

      std::sort(pull_indices.begin(), pull_indices.end());

      v2::ml_data m2 = m.select_rows(pull_indices);

      TS_ASSERT(pull_indices == extract_indices(m2));
    }
  }

};

BOOST_FIXTURE_TEST_SUITE(_test_subsample, test_subsample)
BOOST_AUTO_TEST_CASE(test_subsampling_1) {
  test_subsample::test_subsampling_1();
}
BOOST_AUTO_TEST_CASE(test_subsampling_1b) {
  test_subsample::test_subsampling_1b();
}
BOOST_AUTO_TEST_CASE(test_subsampling_2) {
  test_subsample::test_subsampling_2();
}
BOOST_AUTO_TEST_CASE(test_subsampling_3) {
  test_subsample::test_subsampling_3();
}
BOOST_AUTO_TEST_CASE(test_subsampling_3b) {
  test_subsample::test_subsampling_3b();
}
BOOST_AUTO_TEST_CASE(test_subsampling_4) {
  test_subsample::test_subsampling_4();
}
BOOST_AUTO_TEST_CASE(test_subsampling_1_t) {
  test_subsample::test_subsampling_1_t();
}
BOOST_AUTO_TEST_CASE(test_subsampling_1b_t) {
  test_subsample::test_subsampling_1b_t();
}
BOOST_AUTO_TEST_CASE(test_subsampling_2_t) {
  test_subsample::test_subsampling_2_t();
}
BOOST_AUTO_TEST_CASE(test_subsampling_3_t) {
  test_subsample::test_subsampling_3_t();
}
BOOST_AUTO_TEST_CASE(test_subsampling_3b_t) {
  test_subsample::test_subsampling_3b_t();
}
BOOST_AUTO_TEST_CASE(test_subsampling_4_t) {
  test_subsample::test_subsampling_4_t();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_test_shuffle, test_shuffle)
BOOST_AUTO_TEST_CASE(test_shuffling_1) {
  test_shuffle::test_shuffling_1();
}
BOOST_AUTO_TEST_CASE(test_shuffling_1b) {
  test_shuffle::test_shuffling_1b();
}
BOOST_AUTO_TEST_CASE(test_shuffling_2) {
  test_shuffle::test_shuffling_2();
}
BOOST_AUTO_TEST_CASE(test_shuffling_3) {
  test_shuffle::test_shuffling_3();
}
BOOST_AUTO_TEST_CASE(test_shuffling_3b) {
  test_shuffle::test_shuffling_3b();
}
BOOST_AUTO_TEST_CASE(test_shuffling_4) {
  test_shuffle::test_shuffling_4();
}
BOOST_AUTO_TEST_CASE(test_shuffling_1_t) {
  test_shuffle::test_shuffling_1_t();
}
BOOST_AUTO_TEST_CASE(test_shuffling_1b_t) {
  test_shuffle::test_shuffling_1b_t();
}
BOOST_AUTO_TEST_CASE(test_shuffling_2_t) {
  test_shuffle::test_shuffling_2_t();
}
BOOST_AUTO_TEST_CASE(test_shuffling_3_t) {
  test_shuffle::test_shuffling_3_t();
}
BOOST_AUTO_TEST_CASE(test_shuffling_3b_t) {
  test_shuffle::test_shuffling_3b_t();
}
BOOST_AUTO_TEST_CASE(test_shuffling_4_t) {
  test_shuffle::test_shuffling_4_t();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_test_selection, test_selection)
BOOST_AUTO_TEST_CASE(test_simple) {
  test_selection::test_simple();
}
BOOST_AUTO_TEST_CASE(test_random) {
  test_selection::test_random();
}
BOOST_AUTO_TEST_SUITE_END()
