/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <vector>
#include <string>
#include <random/random.hpp>
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>

#include <unity/toolkits/sparse_similarity/sparse_similarity_lookup.hpp>
#include <unity/toolkits/sparse_similarity/similarities.hpp>
#include <util/cityhash_tc.hpp>

#include "generate_sparse_data.hpp"

using namespace turi;

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void run_test(
    const std::string& similarity,
    const std::vector<std::vector<std::pair<size_t, T> > >& data) {

  auto data_sa = make_testing_sarray(data);

  size_t num_items = 0;
  for(const auto& row : data) {
    for(const auto& p : row) {
      num_items = std::max(num_items, p.first + 1);
    }
  }

  // Set the max memory usage
  size_t max_memory_usage = (sizeof(double) * num_items
                             * (std::max<size_t>(16, (data.size() / 4))));

  std::vector<std::string> training_methods =
    {"auto", "dense", "sparse",
     "nn",
     "nn:dense", "nn:sparse"};


  std::vector<std::shared_ptr<sparse_similarity_lookup> > models;

  for(size_t i = 0; i < training_methods.size(); ++i) {
    const std::string& training_method = training_methods[i];

    std::map<std::string, flexible_type> options = {
      { "max_data_passes", 20},
      { "max_item_neighborhood_size", num_items},
      { "degree_approximation_threshold", 2048},
      { "target_memory_usage", max_memory_usage},
      { "threshold", 0},
      { "sparse_density_estimation_sample_size", 10*1024 },
      { "training_method", training_method } };

    std::cout << ">>>> Now building mode " << training_methods[i] << "." << std::endl;
    auto model = sparse_similarity_lookup::create(similarity, options);
    model->train_from_sparse_matrix_sarray(num_items, data_sa);
    models.push_back(model);
  }

  // Go through and make sure all the routes are exactly the same.
  for(size_t i = 0; i < models.size(); ++i) {
    std::cout << ">>>> Now checking mode " << training_methods[i] << "." << std::endl;
    TS_ASSERT(models[0]->_debug_check_equal(*models[i]));
  }
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
void run_approximation_tests(
    const std::string& similarity,
    const std::vector<std::vector<std::pair<size_t, T> > >& data) {
  
  // Allow the use of the degree_approximation_threshold and
  // max_item_neighborhood_size approximations.  This test simply
  // makes sure that the internal consistency checks for these
  // approximation thresholds are hit.
  
  auto data_sa = make_testing_sarray(data);

  size_t num_items = 0;
  for(const auto& row : data) {
    for(const auto& p : row) {
      num_items = std::max(num_items, p.first + 1);
    }
  }

  // Set the max memory usage
  size_t max_memory_usage = (sizeof(double) * num_items
                             * (std::max<size_t>(16, (data.size() / 4))));

  std::vector<std::string> training_methods =
      {"dense", "sparse"};

  for(size_t degree_approximation_threshold = 10;
      degree_approximation_threshold < 50;
      degree_approximation_threshold += 10) {
    
    for(size_t max_item_neighborhood_size : {2, 5, 10, 20} ) { 
      for(size_t i = 0; i < training_methods.size(); ++i) {
        const std::string& training_method = training_methods[i];

        std::map<std::string, flexible_type> options = {
          { "max_data_passes", 20},
          { "max_item_neighborhood_size", max_item_neighborhood_size},
          { "degree_approximation_threshold", degree_approximation_threshold},
          { "target_memory_usage", max_memory_usage},
          { "threshold", 0},
          { "sparse_density_estimation_sample_size", 1024 },
          { "training_method", training_method } };
  
        std::cout << ">>>> Now building mode " << training_methods[i] << "." << std::endl;
        auto model = sparse_similarity_lookup::create(similarity, options);
        model->train_from_sparse_matrix_sarray(num_items, data_sa);
      }
    }
  }
}


////////////////////////////////////////////////////////////////////////////////

void run_random_test(const std::string& similarity,
                     size_t n, size_t m, double p,
                     bool allow_negative, bool binary) {

  // Determanistic seed for this test.
  random::seed(n*m + 1000000000*allow_negative + 3000000000*binary + size_t(100000000*p));

  auto data= generate(n, m, p, allow_negative, binary);

  run_test(similarity, data);
  run_approximation_tests(similarity, data);
}

////////////////////////////////////////////////////////////////////////////////


struct item_sim_consistency {
 public:

  void test_simple_1_jaccard() {

    // Test this as a corner case.
    std::vector<std::vector<std::pair<size_t, double> > > data
        = { { {0, 1}, {1, 1}, {2, 1} } };

    run_test("jaccard", data);
  }

  void test_simple_2_jaccard() {

    std::vector<std::vector<std::pair<size_t, double> > > data
        = { { {0, 1}, {1, 1}, {2, 1} },
            { {0, 1}, {1, 1}, {3, 1} } };

    run_test("jaccard", data);
  }

  void test_simple_1_cosine() {

    // Test this as a corner case.
    std::vector<std::vector<std::pair<size_t, double> > > data
        = { { {0, 1}, {1, 1}, {2, 1} } };

    run_test("cosine", data);
  }

  void test_simple_2_cosine() {

    std::vector<std::vector<std::pair<size_t, double> > > data
        = { { {0, 1}, {1, 1}, {2, 1} },
            { {0, 1}, {1, 1}, {3, 1} } };

    run_test("cosine", data);
  }

  void test_random_1_jaccard_20m20() {
    run_random_test("jaccard", 20, 20, 0.5, false, true);
  }

  void test_random_2_jaccard_100m100() {
    run_random_test("jaccard", 100, 100, 0.25, false, true);
  }

  void test_random_3_jaccard_1000m25() {
    run_random_test("jaccard", 1000, 25, 0.25, false, true);
  }

  void test_random_4_jaccard_4000m100() {
    run_random_test("jaccard", 4000, 100, 0.1, false, true);
  }

  void test_random_1_cosine_20m20() {
    run_random_test("cosine", 20, 20, 0.5, true, false);
  }

  void test_random_2_cosine_100m100() {
    run_random_test("cosine", 100, 100, 0.25, true, false);
  }

  void test_random_3_cosine_1000m25() {
    run_random_test("cosine", 1000, 25, 0.25, true, false);
  }

  void test_random_4_cosine_4000m100() {
    run_random_test("cosine", 4000, 100, 0.1, true, false);
  }

  void test_random_1_pearson_20m20() {
    run_random_test("pearson", 20, 20, 0.5, true, false);
  }

  void test_random_2_pearson_100m100() {
    run_random_test("pearson", 100, 100, 0.25, true, false);
  }

  void test_random_3_pearson_1000m25() {
    run_random_test("pearson", 1000, 25, 0.25, true, false);
  }

  void test_random_4_pearson_4000m100() {
    run_random_test("pearson", 4000, 100, 0.1, true, false);
  }


  void test_regression_cosine_finalize_prediction_correctness() {
    turi::sparse_sim::cosine cs_sim;

    int64_t val = -(turi::sparse_sim::_fixed_precision_scale_factor / 2);
    
    double out = cs_sim.finalize_prediction(val, turi::sparse_sim::cosine::final_item_data_type(), size_t(8));

    TS_ASSERT_EQUALS(out, -0.5 / 8);
  }

};

BOOST_FIXTURE_TEST_SUITE(_item_sim_consistency, item_sim_consistency)
BOOST_AUTO_TEST_CASE(test_simple_1_jaccard) {
  item_sim_consistency::test_simple_1_jaccard();
}
BOOST_AUTO_TEST_CASE(test_simple_2_jaccard) {
  item_sim_consistency::test_simple_2_jaccard();
}
BOOST_AUTO_TEST_CASE(test_simple_1_cosine) {
  item_sim_consistency::test_simple_1_cosine();
}
BOOST_AUTO_TEST_CASE(test_simple_2_cosine) {
  item_sim_consistency::test_simple_2_cosine();
}
BOOST_AUTO_TEST_CASE(test_random_1_jaccard_20m20) {
  item_sim_consistency::test_random_1_jaccard_20m20();
}
BOOST_AUTO_TEST_CASE(test_random_2_jaccard_100m100) {
  item_sim_consistency::test_random_2_jaccard_100m100();
}
BOOST_AUTO_TEST_CASE(test_random_3_jaccard_1000m25) {
  item_sim_consistency::test_random_3_jaccard_1000m25();
}
BOOST_AUTO_TEST_CASE(test_random_4_jaccard_4000m100) {
  item_sim_consistency::test_random_4_jaccard_4000m100();
}
BOOST_AUTO_TEST_CASE(test_random_1_cosine_20m20) {
  item_sim_consistency::test_random_1_cosine_20m20();
}
BOOST_AUTO_TEST_CASE(test_random_2_cosine_100m100) {
  item_sim_consistency::test_random_2_cosine_100m100();
}
BOOST_AUTO_TEST_CASE(test_random_3_cosine_1000m25) {
  item_sim_consistency::test_random_3_cosine_1000m25();
}
BOOST_AUTO_TEST_CASE(test_random_4_cosine_4000m100) {
  item_sim_consistency::test_random_4_cosine_4000m100();
}
BOOST_AUTO_TEST_CASE(test_random_1_pearson_20m20) {
  item_sim_consistency::test_random_1_pearson_20m20();
}
BOOST_AUTO_TEST_CASE(test_random_2_pearson_100m100) {
  item_sim_consistency::test_random_2_pearson_100m100();
}
BOOST_AUTO_TEST_CASE(test_random_3_pearson_1000m25) {
  item_sim_consistency::test_random_3_pearson_1000m25();
}
BOOST_AUTO_TEST_CASE(test_random_4_pearson_4000m100) {
  item_sim_consistency::test_random_4_pearson_4000m100();
}
BOOST_AUTO_TEST_CASE(test_regression_cosine_finalize_prediction_correctness) {
  item_sim_consistency::test_regression_cosine_finalize_prediction_correctness();
}
BOOST_AUTO_TEST_SUITE_END()
