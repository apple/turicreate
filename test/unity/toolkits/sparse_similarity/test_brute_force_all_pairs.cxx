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

#include <toolkits/sparse_similarity/neighbor_search.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/logging/assertions.hpp>

#include "generate_sparse_data.hpp"

using namespace turi;

static double calc_similarity(
    const sparse_sim::jaccard&,
    const std::vector<std::pair<size_t, double> >& x1,
    const std::vector<std::pair<size_t, double> >& x2) {
  
  double v12 = 0;
  double v1 = 0, v2 = 0;

  auto it_1 = x1.begin();
  auto it_2 = x2.begin();

  size_t biggest_idx = std::numeric_limits<size_t>::max();

  while(it_1 != x1.end() || it_2 != x2.end()) {

    size_t it_2_index = it_2 != x2.end() ? it_2->first : biggest_idx;
    while(it_1 != x1.end() && it_1->first < it_2_index) {
      v1 += (it_1->second != 0 ? 1 : 0);
      ++it_1;
    }

    size_t it_1_index = it_1 != x1.end() ? it_1->first : biggest_idx;
    while(it_2 != x2.end() && it_2->first < it_1_index) {
      v2 += (it_2->second != 0 ? 1 : 0);
      ++it_2;
    }

    while(it_1 != x1.end() && it_2 != x2.end() && it_1->first == it_2->first) {
      v1 += (it_1->second != 0 ? 1 : 0);
      v2 += (it_2->second != 0 ? 1 : 0);
      v12 += (it_1->second != 0 && it_2->second != 0) ? 1 : 0;
      ++it_1, ++it_2;
    }
  }

  return (v1 + v2 == 0) ? 0 : v12 / (v1 + v2 - v12);
}

static double calc_similarity(
    const sparse_sim::cosine&,
    const std::vector<std::pair<size_t, double> >& x1,
    const std::vector<std::pair<size_t, double> >& x2) {

  double v12 = 0;
  double v1 = 0, v2 = 0;

  auto it_1 = x1.begin();
  auto it_2 = x2.begin();

  size_t biggest_idx = std::numeric_limits<size_t>::max();

  while(it_1 != x1.end() || it_2 != x2.end()) {

    size_t it_2_index = it_2 != x2.end() ? it_2->first : biggest_idx;
    while(it_1 != x1.end() && it_1->first < it_2_index) {
      v1 += std::pow(it_1->second, 2);
      ++it_1;
    }

    size_t it_1_index = it_1 != x1.end() ? it_1->first : biggest_idx;
    while(it_2 != x2.end() && it_2->first < it_1_index) {
      v2 += std::pow(it_2->second, 2);
      ++it_2;
    }

    while(it_1 != x1.end() && it_2 != x2.end() && it_1->first == it_2->first) {
      v1 += std::pow(it_1->second, 2);
      v2 += std::pow(it_2->second, 2);
      v12 += it_1->second * it_2->second;
      ++it_1, ++it_2;
    }
  }

  return v12 / std::max(1e-16, std::sqrt(v1 * v2));
}

////////////////////////////////////////////////////////////////////////////////

template <typename Similarity, typename T>
void run_test(const std::vector<std::vector<std::pair<size_t, T> > >& data_1,
              const std::vector<std::vector<std::pair<size_t, T> > >& data_2) {

  Similarity similarity;

  auto data_1_sa = make_testing_sarray(data_1);
  auto data_2_sa = make_testing_sarray(data_2);

  if(&data_1 == &data_2) {
    data_2_sa = data_1_sa;
  }

  size_t n = data_1.size();
  size_t m = data_2.size();

  size_t num_dimensions = 0;
  for(const auto& row : data_1) {
    for(const auto& p : row) {
      num_dimensions = std::max(num_dimensions, p.first + 1);
    }
  }

  for(const auto& row : data_2) {
    for(const auto& p : row) {
      num_dimensions = std::max(num_dimensions, p.first + 1);
    }
  }

  // Set the max memory usage
  size_t max_memory_usage = (sizeof(double) * num_dimensions
                             * (std::max<size_t>(16, (data_2.size() / 16))));

  std::vector<int> hit(n * m, false);

  auto process_function_full = [&](size_t ref_idx, size_t query_idx, double value) {
    DASSERT_LT(ref_idx, n);
    DASSERT_LT(query_idx, m);

    size_t idx = ref_idx * m + query_idx;

    ASSERT_FALSE(hit[idx]);

    hit[idx] = true;

    TURI_ATTRIBUTE_UNUSED_NDEBUG double calc_sim = calc_similarity(
      similarity, data_1[ref_idx], data_2[query_idx]);
    DASSERT_DELTA(calc_sim, value, 2e-5);
  };

  // Don't skip things.
  auto skip_pair = [](size_t, size_t) { return false; };

  all_pairs_similarity(data_1_sa, data_2_sa, similarity,
                       process_function_full, max_memory_usage, skip_pair);

  TURI_ATTRIBUTE_UNUSED_NDEBUG auto bool_not = [](bool x) { return !x; };
  DASSERT_TRUE(std::count_if(hit.begin(), hit.end(), bool_not) == 0);

  ////////////////////////////////////////////////////////////////////////////////
  // Now, test that masks are used correctly.

  auto test_mask = [&](const dense_bitset& query_mask) {

    hit.assign(n*m, false);

    auto process_function_dense_bitset = [&](size_t ref_idx, size_t query_idx, double value) {
      DASSERT_LT(ref_idx, n);
      DASSERT_LT(query_idx, m);

      size_t idx = ref_idx * m + query_idx;

      DASSERT_TRUE(query_mask.get(query_idx));
      DASSERT_FALSE(hit[idx]);

      hit[idx] = true;

      TURI_ATTRIBUTE_UNUSED_NDEBUG double calc_sim = calc_similarity(
        similarity, data_1[ref_idx], data_2[query_idx]);
      DASSERT_TRUE(query_mask.get(query_idx));
      DASSERT_DELTA(calc_sim, value, 2e-5);
    };

    all_pairs_similarity(data_1_sa, data_2_sa, similarity,
                         process_function_dense_bitset, max_memory_usage, skip_pair, &query_mask);

    // Make sure we've only queried the ones with the mask on.
    for(size_t i = 0; i < n; ++i) {
      for(size_t j = 0; j < m; ++j) {
        TURI_ATTRIBUTE_UNUSED_NDEBUG size_t idx = i * m + j;

        if(query_mask.get(j)) {
          DASSERT_TRUE(hit[idx]);
        } else {
          DASSERT_FALSE(hit[idx]);
        }
      }
    }
  };

  {
    dense_bitset query_mask(m);

    // Make sure it works with no entries at all.
    test_mask(query_mask);

    query_mask.invert();

    // Make sure it works with all entries.
    test_mask(query_mask);

  }

  // Make sure it works with a random subset
  if(m >= 2) {
    dense_bitset query_mask(m);

    query_mask.set_bit(1);

    // Make sure it works with no entries at all.
    for(size_t i = 2; i < m; ++i) {
      if(random::fast_uniform<int>(0, 1) == 0) {
        query_mask.set_bit(i);
      }
    }

    test_mask(query_mask);
  }
}

////////////////////////////////////////////////////////////////////////////////

template <typename Similarity>
void run_random_test(size_t n, size_t m, double p,
                     bool allow_negative, bool binary) {

  // Determanistic seed for this test.
  random::seed(n*m + 1000000000*allow_negative + 3000000000*binary + size_t(100000000*p));

  auto data_1 = generate(n, m, p, allow_negative, binary);
  auto data_2 = generate(n, m, p, allow_negative, binary);

  run_test<Similarity>(data_1, data_1);
  run_test<Similarity>(data_2, data_2);
  run_test<Similarity>(data_1, data_2);
}

////////////////////////////////////////////////////////////////////////////////


struct item_sim_all_pairs_test {
 public:

  void test_simple_1_jaccard() {

    std::vector<std::vector<std::pair<size_t, double> > > data
        = { { {0, 1}, {1, 1}, {2, 1} } };

    run_test<sparse_sim::jaccard>(data, data);
  }

  void test_simple_2_jaccard() {

    std::vector<std::vector<std::pair<size_t, double> > > data_1
        = { { {0, 1}, {1, 1}, {2, 1} } };
    std::vector<std::vector<std::pair<size_t, double> > > data_2
        = { { {0, 1}, {1, 1}, {3, 1} } };

    run_test<sparse_sim::jaccard>(data_1, data_2);
  }

  void test_simple_3_jaccard() {

    std::vector<std::vector<std::pair<size_t, double> > > data_1
        = { { {0, 1}, {1, 1}, {2, 1} },
            { {0, 1}, {1, 1}, {3, 1} } };
    std::vector<std::vector<std::pair<size_t, double> > > data_2
        = { { {0, 1}, {1, 1}, {3, 1} },
            { {0, 1}, {4, 1} } };

    run_test<sparse_sim::jaccard>(data_1, data_2);
  }

  void test_simple_1_cosine() {

    std::vector<std::vector<std::pair<size_t, double> > > data
        = { { {0, 1}, {1, 0.5}, {2, -0.5} } };

    run_test<sparse_sim::cosine>(data, data);
  }

  void test_simple_2_cosine() {

    std::vector<std::vector<std::pair<size_t, double> > > data_1
        = { { {0, 1}, {1, 0.5}, {2, 0.4} } };
    std::vector<std::vector<std::pair<size_t, double> > > data_2
        = { { {0, 1}, {1, 0.2}, {3, -1} } };

    run_test<sparse_sim::cosine>(data_1, data_2);
  }

  void test_simple_3_cosine() {

    std::vector<std::vector<std::pair<size_t, double> > > data_1
        = { { {0, 1}, {1, 1}, {2, 0.5} },
            { {0, -0.5}, {1, 0.1}, {3, 0.2} } };
    std::vector<std::vector<std::pair<size_t, double> > > data_2
        = { { {0, 1}, {1, 0.1}, {3, 1} },
            { {0, 0.1}, {4, 0.3} } };

    run_test<sparse_sim::cosine>(data_1, data_2);
  }

  void test_random_1_jaccard_20m20() {
    run_random_test<sparse_sim::jaccard>(20, 20, 0.5, false, true);
  }

  void test_random_2_jaccard_100m100() {
    run_random_test<sparse_sim::jaccard>(100, 100, 0.25, false, true);
  }

  void test_random_3_jaccard_1000m25() {
    run_random_test<sparse_sim::jaccard>(1000, 25, 0.25, false, true);
  }

  void test_random_1_cosine_20m20() {
    run_random_test<sparse_sim::cosine>(20, 20, 0.5, true, false);
  }

  void test_random_2_cosine_100m100() {
    run_random_test<sparse_sim::cosine>(100, 100, 0.25, true, false);
  }

  void test_random_3_cosine_1000m25() {
    run_random_test<sparse_sim::cosine>(1000, 25, 0.25, true, false);
  }


};

BOOST_FIXTURE_TEST_SUITE(_item_sim_all_pairs_test, item_sim_all_pairs_test)
BOOST_AUTO_TEST_CASE(test_simple_1_jaccard) {
  item_sim_all_pairs_test::test_simple_1_jaccard();
}
BOOST_AUTO_TEST_CASE(test_simple_2_jaccard) {
  item_sim_all_pairs_test::test_simple_2_jaccard();
}
BOOST_AUTO_TEST_CASE(test_simple_3_jaccard) {
  item_sim_all_pairs_test::test_simple_3_jaccard();
}
BOOST_AUTO_TEST_CASE(test_simple_1_cosine) {
  item_sim_all_pairs_test::test_simple_1_cosine();
}
BOOST_AUTO_TEST_CASE(test_simple_2_cosine) {
  item_sim_all_pairs_test::test_simple_2_cosine();
}
BOOST_AUTO_TEST_CASE(test_simple_3_cosine) {
  item_sim_all_pairs_test::test_simple_3_cosine();
}
BOOST_AUTO_TEST_CASE(test_random_1_jaccard_20m20) {
  item_sim_all_pairs_test::test_random_1_jaccard_20m20();
}
BOOST_AUTO_TEST_CASE(test_random_2_jaccard_100m100) {
  item_sim_all_pairs_test::test_random_2_jaccard_100m100();
}
BOOST_AUTO_TEST_CASE(test_random_3_jaccard_1000m25) {
  item_sim_all_pairs_test::test_random_3_jaccard_1000m25();
}
BOOST_AUTO_TEST_CASE(test_random_1_cosine_20m20) {
  item_sim_all_pairs_test::test_random_1_cosine_20m20();
}
BOOST_AUTO_TEST_CASE(test_random_2_cosine_100m100) {
  item_sim_all_pairs_test::test_random_2_cosine_100m100();
}
BOOST_AUTO_TEST_CASE(test_random_3_cosine_1000m25) {
  item_sim_all_pairs_test::test_random_3_cosine_1000m25();
}
BOOST_AUTO_TEST_SUITE_END()
