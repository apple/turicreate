/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <string>
#include <core/random/random.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

#include <toolkits/sparse_similarity/utilities.hpp>
#include <core/util/cityhash_tc.hpp>

using namespace turi;

struct item_sim_transpose_test {

 public:

  template <typename T>
  void run_test(const std::vector<std::vector<std::pair<size_t, T> > >& data,
                size_t max_memory_usage) {

    ////////////////////////////////////////////////////////////////////////////////
    // Accumulate the counts.

    std::vector<size_t> counts;

    for(const auto& row : data) {
      for(const auto& p : row) {
        if(p.first >= counts.size()) {
          counts.insert(counts.end(), p.first + 1 - counts.size(), 0);
        }

        counts[p.first] += 1;
      }
    }

    size_t num_counts = counts.size();

    ////////////////////////////////////////////////////////////////////////////////
    // Do a manual transpose of the data into a vector.

    std::vector<std::vector<std::pair<size_t, T> > > data_t;

    data_t.resize(num_counts);

    for(size_t row_idx = 0;  row_idx < data.size(); ++row_idx) {
      const auto& row = data[row_idx];
      for(const auto& p : row) {
        data_t[p.first].push_back({row_idx, p.second});
      }
    }


    ////////////////////////////////////////////////////////////////////////////////
    // Convert to sarray
    auto data_sa = make_testing_sarray(data);

    // Transpose
    auto data_t_sa = transpose_sparse_sarray(data_sa, counts, max_memory_usage);

    auto data_t_test = testing_extract_column_non_flex(data_t_sa);

    ASSERT_EQ(data_t_test.size(), data_t.size());

    for(size_t i = 0; i < data_t.size(); ++i) {
      const auto& row_1 = data_t[i];
      const auto& row_2 = data_t_test[i];

      ASSERT_EQ(row_1.size(), row_2.size());

      for(size_t j = 0; j < row_1.size(); ++j) {
        ASSERT_TRUE(row_1[j] == row_2[j]);
      }
    }
  }

  void test_simple_transpose() {

    std::vector<std::vector<std::pair<size_t, size_t> > > data
        = { { {0, 0}, {1, 1}, {2, 2} } };

    run_test(data, 1000);

  }

  void test_simple_transpose_2() {

    std::vector<std::vector<std::pair<size_t, size_t> > > data
        = { { {0, 0}, {1, 1}, {2, 2} },
            { {0, 0}, {1, 1}, {2, 2} } };

    run_test(data, 1000);
  }

  void test_transpose_large_dense() {

    size_t n = 500;

    std::vector<std::vector<std::pair<size_t, size_t> > > data(n);

    for(size_t i = 0; i < n; ++i) {
      for(size_t j = 0; j < n; ++j) {
        data[i].push_back({j, hash64(i, j)});
      }
    }

    run_test(data, (n * n * 16) / 4);
  }

  void test_transpose_large_sparse() {

    size_t n = 500;

    std::vector<std::vector<std::pair<size_t, size_t> > > data(n);

    for(size_t i = 0; i < n; ++i) {
      for(size_t j = 0; j < n; ++j) {
        data[i].push_back({hash64(j, i) % (10*1024*1024), hash64(i, j)});
      }
      std::sort(data[i].begin(), data[i].end());
    }

    run_test(data, (n * n * 16) / 4);
  }

};

BOOST_FIXTURE_TEST_SUITE(_item_sim_transpose_test, item_sim_transpose_test)
BOOST_AUTO_TEST_CASE(test_simple_transpose) {
  item_sim_transpose_test::test_simple_transpose();
}
BOOST_AUTO_TEST_CASE(test_simple_transpose_2) {
  item_sim_transpose_test::test_simple_transpose_2();
}
BOOST_AUTO_TEST_CASE(test_transpose_large_dense) {
  item_sim_transpose_test::test_transpose_large_dense();
}
BOOST_AUTO_TEST_CASE(test_transpose_large_sparse) {
  item_sim_transpose_test::test_transpose_large_sparse();
}
BOOST_AUTO_TEST_SUITE_END()
