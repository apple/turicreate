/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
const bool enable_expensive_tests = false;

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <string>
#include <functional>

#include <core/random/random.hpp>

#include <toolkits/recsys/models/factorization_models.hpp>
#include <toolkits/util/data_generators.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <model_server/lib/variant.hpp>


#include <cfenv>

using namespace turi;
using namespace turi::recsys;

#include "factorization_test_helpers.hpp"

struct matrix_factorization_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"num_factors",         1 } };

    test_convergence({1, 1}, opts, "mf");
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_mf_no_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"num_factors",         0 } };

    test_convergence({1, 1}, opts, "mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_8_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         8 } };

    test_convergence({8, 1}, opts, "mf");
  }
  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_many_factors() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         1000 } };

    test_convergence({8, 1}, opts, "mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_many_categories() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         1 } };

    test_convergence({2, 50}, opts, "mf");
  }

  void test_mf_se_many_columns() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         1 } };

    test_convergence({16, 1, 1, 1}, opts, "mf");
  }
};

struct log_mf_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_mf_log_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"num_factors",         1 } };

    test_convergence({1, 1}, opts, "logistic_mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_log_many_factors_2d() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         1000 } };

    test_convergence({20, 20}, opts, "logistic_mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_log_many_factors_3d() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         8 } };

    test_convergence({20, 20, 20}, opts, "logistic_mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_log_many_categories() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         1 } };

    test_convergence({2, 50}, opts, "logistic_mf");
  }

  void test_mf_log_many_dimensions() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         1 } };

    test_convergence({16, 16, 16, 16}, opts, "logistic_mf");
  }
};


struct nmf_tests  {
 public:

  void test_nmf_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"nmf",                 true},
      {"num_factors",         1 } };

    test_convergence({2, 2}, opts, "mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_nmf_many_factors() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"nmf",                 true},
      {"num_factors",         1000 } };

    test_convergence({8, 8}, opts, "mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_nmf_fm_many_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"nmf",                 true},
      {"num_factors",         16 } };

    test_convergence({4, 4, 4}, opts, "fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_nmf_many_categories() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"nmf",                 true},
      {"num_factors",         1 } };

    test_convergence({1, 40}, opts, "mf");
  }
};

// // ////////////////////////////////////////////////////////////////////////////////

struct regressions  {
 public:
  void test_initialization_regression() {
    sframe X = make_testing_sframe({"user_id", "item_id", "target"},
                                   { {1, 1, 0},
                                     {2, 1, 1},
                                     {2, 2, 2},
                                     {3, 3, 3},
                                     {3, 4, 4},
                                     {3, 5, 5} } );

    std::vector<double> initialization_values;

    std::shared_ptr<recsys::recsys_model_base> model(new recsys_ranking_factorization_model);

    model->init_options({
        {"ranking_regularization", 0.25},
        {"num_factors", 32},
        {"max_iterations", 25},
        {"regularization", 0.0},
        {"random_seed", 0}
            });
    model->setup_and_train(X);

    std::map<std::string, variant_type> state = model->get_state();

    auto training_stats
        = variant_get_value<std::map<std::string, variant_type> >(state["training_stats"]);

    TURI_ATTRIBUTE_UNUSED_NDEBUG double initial_objective =
      variant_get_value<double>(training_stats["initial_objective_value"]);
    variant_get_value<double>(training_stats["final_objective_value"]);
    variant_get_value<double>(training_stats["sgd_step_size"]);


    DASSERT_LT(initial_objective, 100);
  }

  void test_sgd_regularization_oddity() {

    // The side column exactly predicts the target column; 
    sframe obs_data = make_testing_sframe( {"user", "item", "side", "target"},
                                           { {10, 20, 1,  1},
                                             {10, 21, 3,  3},
                                             {10, 22, 8,  8},
                                             {11, 20, 5,  5},
                                             {11, 21, 20, 20},
                                             {11, 22, 2,  2},
                                             {12, 20, 1,  1},
                                             {12, 21, 5,  5},
                                             {12, 22, 12, 12},
                                             {13, 20, 2,  2},
                                             {13, 21, 10, 10},
                                                 // This one is 23, so each user has one unrated item
                                             {13, 23, 10, 10}, 
                                                         
                                             {10, 20, -1,  -1},
                                             {10, 21, -3,  -3},
                                             {10, 22, -8,  -8},
                                             {11, 20, -5,  -5},
                                             {11, 21, -20, -20},
                                             {11, 22, -2,  -2},
                                             {12, 20, -1,  -1},
                                             {12, 21, -5,  -5},
                                             {12, 22, -12, -12},
                                             {13, 20, -2,  -2},
                                             {13, 21, -10, -10},
                                                 // This one is 23, so each user has one unrated item
                                             {13, 23, -10, -10} } );

    auto model = new recsys::recsys_factorization_model();

    ////////////////////////////////////////////////////////////
    // Set the options

    std::map<std::string, flexible_type> opts;
    opts["item_id"] = "item";
    opts["user_id"] = "user";
    opts["target"] = "target";
    opts["num_factors"] = 0;
    opts["max_iterations"] = 1000;
    opts["sgd_convergence_threshold"] = 0; 
    opts["linear_regularization"] = 0;
    opts["regularization"] = 0;
    opts["sgd_step_size"] = 0;
    
    model->init_options(opts);

    ////////////////////////////////////////////////////////////
    // Train the model

    // A trick to make sure that things are run single threaded and
    // are then deterministic.
    parallel_for(size_t(0), size_t(16), [&](size_t i) {
        if(i == 0) 
          model->setup_and_train(obs_data);
      });
      
    // Now with side data. 
    // The side column exactly predicts the target column;
    {
      sframe res_back = model->predict(model->create_ml_data(obs_data));

      std::cerr << res_back.column_names() << std::endl; 

      std::vector<double> true_scores = testing_extract_column<double>(obs_data.select_column("target"));
      std::vector<double> pred_scores = testing_extract_column<double>(res_back.select_column(0));

      ASSERT_EQ(true_scores.size(), pred_scores.size());

      // Make sure each of these equals zero
      for(size_t i = 0; i < true_scores.size(); ++i)
        TS_ASSERT_DELTA(true_scores[i], pred_scores[i], 0.05);
    }
  }
  
};

BOOST_FIXTURE_TEST_SUITE(_matrix_factorization_tests, matrix_factorization_tests)
BOOST_AUTO_TEST_CASE(test_mf_se_really_bloody_basic_2d) {
  matrix_factorization_tests::test_mf_se_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_mf_se_mf_no_factors) {
  matrix_factorization_tests::test_mf_se_mf_no_factors();
}
BOOST_AUTO_TEST_CASE(test_mf_se_8_factors) {
  matrix_factorization_tests::test_mf_se_8_factors();
}
BOOST_AUTO_TEST_CASE(test_mf_se_many_factors) {
  matrix_factorization_tests::test_mf_se_many_factors();
}
BOOST_AUTO_TEST_CASE(test_mf_se_many_categories) {
  matrix_factorization_tests::test_mf_se_many_categories();
}
BOOST_AUTO_TEST_CASE(test_mf_se_many_columns) {
  matrix_factorization_tests::test_mf_se_many_columns();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_log_mf_tests, log_mf_tests)
BOOST_AUTO_TEST_CASE(test_mf_log_really_bloody_basic_2d) {
  log_mf_tests::test_mf_log_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_mf_log_many_factors_2d) {
  log_mf_tests::test_mf_log_many_factors_2d();
}
BOOST_AUTO_TEST_CASE(test_mf_log_many_factors_3d) {
  log_mf_tests::test_mf_log_many_factors_3d();
}
BOOST_AUTO_TEST_CASE(test_mf_log_many_categories) {
  log_mf_tests::test_mf_log_many_categories();
}
BOOST_AUTO_TEST_CASE(test_mf_log_many_dimensions) {
  log_mf_tests::test_mf_log_many_dimensions();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_nmf_tests, nmf_tests)
BOOST_AUTO_TEST_CASE(test_nmf_really_bloody_basic_2d) {
  nmf_tests::test_nmf_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_nmf_many_factors) {
  nmf_tests::test_nmf_many_factors();
}
BOOST_AUTO_TEST_CASE(test_nmf_fm_many_factors) {
  nmf_tests::test_nmf_fm_many_factors();
}
BOOST_AUTO_TEST_CASE(test_nmf_many_categories) {
  nmf_tests::test_nmf_many_categories();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_regressions, regressions)
BOOST_AUTO_TEST_CASE(test_initialization_regression) {
  regressions::test_initialization_regression();
}
BOOST_AUTO_TEST_CASE(test_sgd_regularization_oddity) {
  regressions::test_sgd_regularization_oddity();
}
BOOST_AUTO_TEST_SUITE_END()
