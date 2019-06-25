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

#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <toolkits/recsys/models/linear_models/linear_model.hpp>
#include <toolkits/recsys/models/linear_models/factorization_model.hpp>
#include <toolkits/recsys/models/linear_models/matrix_factorization.hpp>
#include <toolkits/util/data_generators.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterator.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>


#include <cfenv>

using namespace turi;
using namespace turi::recsys;

void run_exact_test(
    const std::vector<size_t>& n_categorical_values,
    std::map<std::string, flexible_type> opts,
    const std::string& model_type) {

  bool binary_target = false;

  if(model_type == "linear" || model_type == "fm" || model_type == "mf") {
    binary_target = false;
    opts["y_mode"] = "squared_error";
  } else if(model_type == "logistic"  || model_type == "logistic_fm" || model_type == "logistic_mf") {
    binary_target = true;
    opts["y_mode"] = "logistic";
  } else {
    DASSERT_TRUE(false);
  }

  if(model_type == "mf" || model_type == "logistic_mf")
    opts["only_2_factor_terms"] = true;

  std::feraiseexcept(FE_ALL_EXCEPT);

  size_t n_observations = opts.at("n_observations");
  opts.erase("n_observations");

  std::string target_column_name = "target";

  std::vector<std::string> column_names = {"user", "item"};

  DASSERT_NE(n_categorical_values[0], 0);
  DASSERT_NE(n_categorical_values[1], 0);

  for(size_t i = 2; i < n_categorical_values.size(); ++i) {
    column_names.push_back("C-" + std::to_string(i));
  }

  lm_data_generator lmdata(column_names, n_categorical_values, opts);

  sframe train_data = lmdata.generate(n_observations, target_column_name, 0, 0);
  sframe test_data = lmdata.generate(n_observations, target_column_name, 1, 0);

  std::map<std::string, flexible_type> options =
      { {"optimization_method", "auto"},
        {"binary_target", binary_target},
        {"target", target_column_name},
        {"regularization", 0},
        {"sgd_step_size", 0},
        {"max_iterations", binary_target ? 200 : 100},
        {"sgd_convergence_threshold", 1e-10},

        // Add in the ranking regularizer
        {"ranking_regularization", 0.1},
        {"unobserved_rating_value", 0}
      };

  opts.erase("y_mode");

  if(model_type == "mf" || model_type == "logistic_mf")
    opts.erase("only_2_factor_terms");

  options.insert(opts.begin(), opts.end());

  if(model_type == "fm" || model_type == "logistic_fm"
     || model_type == "mf" || model_type == "logistic_mf") {
    options["linear_regularization"] = 0;
  }

  // Instantiate some alternate versions to make sure the save and load work
  typedef std::shared_ptr<recsys::recsys_model_base> model_ptr;

  auto new_model = [&]() -> model_ptr {
    model_ptr ret;

    if(model_type == "linear" || model_type == "logistic") {
      ret.reset(new recsys_linear_model);
    } else if(model_type == "fm" || model_type == "logistic_fm") {
      ret.reset(new recsys_factorization_model);
    } else if(model_type == "mf" || model_type == "logistic_mf") {
      ret.reset(new recsys_matrix_factorization);
    } else {
      ASSERT_TRUE(false);
    }

    return ret;
  };

  model_ptr model = new_model();

  model->init_option_info();

  model->set_options(options);

  model->setup_and_train(train_data);

  // Instantiate some alternate versions to make sure the save and load work

  std::vector<model_ptr> all_models =
      {model,
       new_model(),
       model_ptr((recsys::recsys_model_base*)(model->clone()))};

  ////////////////////////////////////////
  {
    // Save it
    dir_archive archive_write;
    archive_write.open_directory_for_write("linear_regression_cxx_tests");

    turi::oarchive oarc(archive_write);

    oarc << *model;

    archive_write.close();

    // Load it
    dir_archive archive_read;
    archive_read.open_directory_for_read("linear_regression_cxx_tests");

    turi::iarchive iarc(archive_read);

    iarc >> (*all_models[1]);
  }

  for(model_ptr m : all_models) {

    sframe y_hat_sf = m->predict(model->create_ml_data(test_data));
    std::vector<double> y_hat = testing_extract_column<double>(y_hat_sf.select_column(0));

    {
      size_t i = 0;

      for(ml_data_iterator it(m->create_ml_data(test_data)); !it.done(); ++it, ++i) {
        double y = it.target_value();

        if(!binary_target) {
          if(y > 0)
            TS_ASSERT_LESS_THAN(y_hat[i], 1.1 * (y + 0.5));
        } else {
          if(y == 0)
            TS_ASSERT_LESS_THAN(y_hat[i], 0.75);
        }
      }
    }
  }
}

struct linear_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_regression_se_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 } };

    run_exact_test({1, 1}, opts, "linear");
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_regression_se_basic_3d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 } };

    // run_exact_test({1,1,0}, opts, "linear");
  }


  ////////////////////////////////////////////////////////////////////////////////

  void test_regression_se_basic_5d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      250 } };

    // run_exact_test({1,1,0,0,0}, opts, "linear");
  }

  // ////////////////////////////////////////////////////////////////////////////////

  void test_regression_se_multiuser_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      500 } };

    // run_exact_test({2,2,0,0,0}, opts, "linear");
  }

  // ////////////////////////////////////////////////////////////////////////////////

  void test_regression_se_large_no_side() {

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100000 } };

    run_exact_test({100,100}, opts, "linear");
  }


  // // ////////////////////////////////////////////////////////////////////////////////

  void test_regression_se_large_some_side() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      5*10*10 } };

    run_exact_test({10,10,0,0,0}, opts, "linear");
  }
};

struct log_linear_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_regression_log_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 } };

    run_exact_test({1, 1}, opts, "logistic");
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_regression_log_basic_3d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 } };

    // run_exact_test({1,1,0}, opts, "logistic");
  }

  // ////////////////////////////////////////////////////////////////////////////////

  void test_regression_log_large_no_side() {

    if(enable_expensive_tests) {
      std::map<std::string, flexible_type> opts = {
        {"n_observations",     10*100*100 } };

      run_exact_test({100,100}, opts, "logistic");
    }
  }
};


struct factorization_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"n_factors",           1 } };

    run_exact_test({1, 1}, opts, "fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_many_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           5 } };

    run_exact_test({1, 8}, opts, "fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_many_columns() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           1 } };

    run_exact_test({16, 1, 1, 1}, opts, "fm");
  }
};

struct log_factorization_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"n_factors",           1 } };

    run_exact_test({1, 1}, opts, "logistic_fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_many_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",          5 } };

    run_exact_test({1, 8}, opts, "logistic_fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_many_categories() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      2000 },
      {"n_factors",           1 } };

    run_exact_test({2, 50}, opts, "logistic_fm");
  }

  void test_factorization_log_many_dimensions() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           1 } };

    run_exact_test({16, 1, 1, 1}, opts, "logistic_fm");
  }
};

struct matrix_factorization_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"n_factors",           1 } };

    run_exact_test({1, 1}, opts, "mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_many_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           5 } };

    run_exact_test({8, 1}, opts, "mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_se_many_categories() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           1 } };

    run_exact_test({2, 50}, opts, "mf");
  }

  void test_mf_se_many_columns() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           1 } };

    run_exact_test({16, 1, 1, 1}, opts, "mf");
  }
};

struct log_mf_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_mf_log_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"n_factors",           1 } };

    run_exact_test({1, 1}, opts, "logistic_mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_log_many_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           5 } };

    run_exact_test({5, 5}, opts, "logistic_mf");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_mf_log_many_categories() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           1 } };

    run_exact_test({2, 30}, opts, "logistic_mf");
  }

  void test_mf_log_many_dimensions() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      1000 },
      {"n_factors",           1 } };

    run_exact_test({16, 1, 1, 1}, opts, "logistic_mf");
  }
};

struct class pure_ranking  {
//  public:

//   void run_pure_ranking_test(size_t n_users, size_t n_items,
//                              size_t n_items_train, size_t n_items_test,
//                              std::map<std::string, flexible_type> opts,
//                              const std::string& model_type) {

//     std::feraiseexcept(FE_ALL_EXCEPT);

//     std::vector<std::string> column_names = {"user", "item"};

//     opts["y_mode"] = "squared_error";
//     lm_data_generator lmdata(column_names, {n_users, n_items}, opts);
//     opts.erase("y_mode");

//     sframe train_data, test_data;

//     std::tie(train_data, test_data) = lmdata.generate_for_ranking(
//         n_items_train, n_items_test, 0, 0);

//     std::shared_ptr<recsys_model_base> model;

//     std::map<std::string, flexible_type> options =
//         { {"optimization_method", "auto"},
//           {"sgd_step_size", 0},
//           {"max_iterations", 20},
//           {"sgd_convergence_threshold", 1e-6},

//           // No target column; so should be able to properly rank
//           // things.
//         };

//     options.insert(opts.begin(), opts.end());

//     // Instantiate some alternate versions to make sure the save and load work
//     typedef std::shared_ptr<recsys::recsys_model_base> model_ptr;

//     auto new_model = [&]() {
//       model_ptr ret;

//       if(model_type == "linear" || model_type == "logistic") {
//         ret.reset(new recsys_linear_model);
//       } else if(model_type == "fm" || model_type == "logistic_fm") {
//         ret.reset(new recsys_factorization_model);
//       } else if(model_type == "mf" || model_type == "logistic_mf") {
//         ret.reset(new recsys_matrix_factorization);
//       } else {
//         ASSERT_TRUE(false);
//       }

//       return ret;
//     };

//     model = new_model();

//     model->set_options(options);

//     model->setup_and_train(train_data);

//     std::vector<model_ptr> all_models =
//         {model,
//          new_model(),
//          model_ptr((recsys::recsys_model_base*)(model->clone()))};

//     ////////////////////////////////////////
//     {
//       // Save it
//       dir_archive archive_write;
//       archive_write.open_directory_for_write("linear_regression_cxx_tests");

//       turi::oarchive oarc(archive_write);

//       oarc << *model;

//       archive_write.close();

//       // Load it
//       dir_archive archive_read;
//       archive_read.open_directory_for_read("linear_regression_cxx_tests");

//       turi::iarchive iarc(archive_read);

//       iarc >> (*all_models[1]);
//     }

//     auto build_map = [&](const sframe& user_item_data) {
//       std::map<size_t, std::set<size_t> > user_map;

//       for(parallel_sframe_iterator it(user_item_data); !it.done(); ++it) {
//         size_t user = it.value(0);
//         size_t item = it.value(1);

//         user_map[user].insert(item);
//       }

//       return user_map;
//     };

//     auto true_user_map = build_map(test_data);

//     for(model_ptr m : all_models) {

//       sframe rec_out = m->recommend(nullptr, n_items_test);

//       auto user_map = build_map(rec_out);

//       for(size_t i = 0; i < n_users; ++i) {

//         // ASSERT_EQ(user_map[i], true_user_map[i]);
//       }
//     }
//   }

//   void test_linear_1() {
//     std::map<std::string, flexible_type> opts = {
//       {"_num_sampled_negative_examples", 1} };

//     run_pure_ranking_test(100, 10, 2, 1, opts, "linear");
//   }

//   void test_linear_2() {
//     std::map<std::string, flexible_type> opts;

//     run_pure_ranking_test(1000, 50, 20, 5, opts, "linear");
//   }

//   ////////////////////////////////////////////////////////////
//   // Test the different aspects of the procedure to sample negative
//   // points.  There a number of internal asserts to check that the
//   // sampling is consistent; below are the set of things which would
//   // likely hit the corner cases of the sampling algorithm.

//   void test_linear_most_items_rated_1() {
//     std::map<std::string, flexible_type> opts = {
//       {"_num_sampled_negative_examples", 1} };

//     run_pure_ranking_test(1000, 32, 31, 1, opts, "linear");
//   }

//   void test_linear_most_items_rated_2() {
//     std::map<std::string, flexible_type> opts = {
//       {"_num_sampled_negative_examples", 16} };

//     run_pure_ranking_test(10, 32, 31, 1, opts, "linear");
//   }

//     void test_linear_most_items_rated_1b() {
//     std::map<std::string, flexible_type> opts = {
//       {"_num_sampled_negative_examples", 1} };

//     run_pure_ranking_test(10, 65, 63, 1, opts, "linear");
//   }

//   void test_linear_most_items_rated_2b() {
//     std::map<std::string, flexible_type> opts = {
//       {"_num_sampled_negative_examples", 16} };

//     run_pure_ranking_test(10, 65, 64, 1, opts, "linear");
//   }

//   void test_linear_most_items_rated_3() {
//     std::map<std::string, flexible_type> opts = {
//       {"_num_sampled_negative_examples", 4} };

//     run_pure_ranking_test(10, 100, 90, 4, opts, "linear");
//   }

//   void test_linear_few_items_rated_1() {
//     std::map<std::string, flexible_type> opts = {
//       {"_num_sampled_negative_examples", 4} };

//     run_pure_ranking_test(10, 1000, 2, 1, opts, "linear");
//   }

//   ////////////////////////////////////////////////////////////
//   // Test matrix factorization

//   void test_mf_1() {
//     std::map<std::string, flexible_type> opts = {
//       {"n_factors",           1 } };

//     run_pure_ranking_test(10, 10, 4, 2, opts, "mf");
//   }

//   void test_mf_2() {
//     std::map<std::string, flexible_type> opts = {
//       {"n_factors",           4 } };

//     run_pure_ranking_test(100, 10, 4, 2, opts, "mf");
//   }

//   void test_mf_3() {
//     std::map<std::string, flexible_type> opts = {
//       {"n_factors",           1 } };

//     run_pure_ranking_test(10, 50, 20, 5, opts, "mf");
//   }

//   void test_fm_1() {
//     std::map<std::string, flexible_type> opts = {
//       {"n_factors",           1 } };

//     run_pure_ranking_test(10, 10, 4, 2, opts, "fm");
//   }

//   void test_fm_2() {
//     std::map<std::string, flexible_type> opts = {
//       {"n_factors",           4 } };

//     run_pure_ranking_test(100, 10, 4, 2, opts, "fm");
//   }

//   void test_fm_3() {
//     std::map<std::string, flexible_type> opts = {
//       {"n_factors",           1 } };

//     run_pure_ranking_test(10, 50, 20, 5, opts, "fm");
//   }


// };

BOOST_FIXTURE_TEST_SUITE(_linear_tests, linear_tests)
BOOST_AUTO_TEST_CASE(test_regression_se_really_bloody_basic_2d) {
  linear_tests::test_regression_se_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_regression_se_basic_3d) {
  linear_tests::test_regression_se_basic_3d();
}
BOOST_AUTO_TEST_CASE(test_regression_se_basic_5d) {
  linear_tests::test_regression_se_basic_5d();
}
BOOST_AUTO_TEST_CASE(test_regression_se_multiuser_basic_2d) {
  linear_tests::test_regression_se_multiuser_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_regression_se_large_no_side) {
  linear_tests::test_regression_se_large_no_side();
}
BOOST_AUTO_TEST_CASE(test_regression_se_large_some_side) {
  linear_tests::test_regression_se_large_some_side();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_log_linear_tests, log_linear_tests)
BOOST_AUTO_TEST_CASE(test_regression_log_really_bloody_basic_2d) {
  log_linear_tests::test_regression_log_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_regression_log_basic_3d) {
  log_linear_tests::test_regression_log_basic_3d();
}
BOOST_AUTO_TEST_CASE(test_regression_log_large_no_side) {
  log_linear_tests::test_regression_log_large_no_side();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_factorization_tests, factorization_tests)
BOOST_AUTO_TEST_CASE(test_factorization_se_really_bloody_basic_2d) {
  factorization_tests::test_factorization_se_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_factorization_se_many_factors) {
  factorization_tests::test_factorization_se_many_factors();
}
BOOST_AUTO_TEST_CASE(test_factorization_se_many_columns) {
  factorization_tests::test_factorization_se_many_columns();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_log_factorization_tests, log_factorization_tests)
BOOST_AUTO_TEST_CASE(test_factorization_log_really_bloody_basic_2d) {
  log_factorization_tests::test_factorization_log_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_many_factors) {
  log_factorization_tests::test_factorization_log_many_factors();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_many_categories) {
  log_factorization_tests::test_factorization_log_many_categories();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_many_dimensions) {
  log_factorization_tests::test_factorization_log_many_dimensions();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_matrix_factorization_tests, matrix_factorization_tests)
BOOST_AUTO_TEST_CASE(test_mf_se_really_bloody_basic_2d) {
  matrix_factorization_tests::test_mf_se_really_bloody_basic_2d();
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
BOOST_AUTO_TEST_CASE(test_mf_log_many_factors) {
  log_mf_tests::test_mf_log_many_factors();
}
BOOST_AUTO_TEST_CASE(test_mf_log_many_categories) {
  log_mf_tests::test_mf_log_many_categories();
}
BOOST_AUTO_TEST_CASE(test_mf_log_many_dimensions) {
  log_mf_tests::test_mf_log_many_dimensions();
}
BOOST_AUTO_TEST_SUITE_END()
