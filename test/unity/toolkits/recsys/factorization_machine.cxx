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

#include "factorization_test_helpers.hpp"

using namespace turi;
using namespace turi::recsys;

struct factorization_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"num_factors",           1 } };

    test_convergence({2, 2}, opts, "fm");
  }

  ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_really_bloody_basic_2d_8f() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"num_factors",           8 } };

    test_convergence({2, 2}, opts, "fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_really_bloody_basic_3d() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"num_factors",           1 } };

    test_convergence({2, 2, 2}, opts, "fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_many_factors() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",           1000 }
    };

    test_convergence({4, 4, 4}, opts, "fm");
  }

  void test_factorization_se_8_factors() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",           8}
    };

    test_convergence({8, 8, 8}, opts, "fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_se_many_columns() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",           1 } };

    test_convergence({8, 8, 8, 8}, opts, "fm");
  }
};

struct log_factorization_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_really_bloody_basic_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      10 },
      {"num_factors",           1 } };

    test_convergence({1, 1}, opts, "logistic_fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_many_factors_2d() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",          1000 } };

    test_convergence({8, 8}, opts, "logistic_fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_8_factors_2d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",          8 } };

    test_convergence({8, 8}, opts, "logistic_fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_many_factors_3d() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",          1000 } };

    test_convergence({8, 8, 8}, opts, "logistic_fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_8_factors_3d() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         8 } };

    test_convergence({4, 4, 4}, opts, "logistic_fm");
  }

  // // ////////////////////////////////////////////////////////////////////////////////

  void test_factorization_log_many_categories() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      200 },
      {"num_factors",         1 } };

    test_convergence({2, 50}, opts, "logistic_fm");
  }

  void test_factorization_log_many_dimensions() {
    if(!enable_expensive_tests)
      return;

    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 },
      {"num_factors",         1 } };

    test_convergence({4, 4, 4}, opts, "logistic_fm");
  }

  void test_factorization_regression() {

    std::vector<std::vector<flexible_type> > Xv(300);

    for(size_t i = 0; i < 300; ++i) {
      Xv[i] = {i % 10, i % 30, 1};
    }

    sframe data = make_testing_sframe({"user_id", "item_id", "target"}, Xv);



    std::map<std::string, flexible_type> options =
        { {"solver", "auto"},
          {"binary_target", false},
          {"target", "target"},
          {"max_iterations", 5} };

    typedef std::shared_ptr<recsys::recsys_model_base> model_ptr;
    model_ptr model(new recsys_ranking_factorization_model);

    model->init_options(options);


    model->setup_and_train(data);
  }


};

BOOST_FIXTURE_TEST_SUITE(_factorization_tests, factorization_tests)
BOOST_AUTO_TEST_CASE(test_factorization_se_really_bloody_basic_2d) {
  factorization_tests::test_factorization_se_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_factorization_se_really_bloody_basic_2d_8f) {
  factorization_tests::test_factorization_se_really_bloody_basic_2d_8f();
}
BOOST_AUTO_TEST_CASE(test_factorization_se_really_bloody_basic_3d) {
  factorization_tests::test_factorization_se_really_bloody_basic_3d();
}
BOOST_AUTO_TEST_CASE(test_factorization_se_many_factors) {
  factorization_tests::test_factorization_se_many_factors();
}
BOOST_AUTO_TEST_CASE(test_factorization_se_8_factors) {
  factorization_tests::test_factorization_se_8_factors();
}
BOOST_AUTO_TEST_CASE(test_factorization_se_many_columns) {
  factorization_tests::test_factorization_se_many_columns();
}
BOOST_AUTO_TEST_SUITE_END()
BOOST_FIXTURE_TEST_SUITE(_log_factorization_tests, log_factorization_tests)
BOOST_AUTO_TEST_CASE(test_factorization_log_really_bloody_basic_2d) {
  log_factorization_tests::test_factorization_log_really_bloody_basic_2d();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_many_factors_2d) {
  log_factorization_tests::test_factorization_log_many_factors_2d();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_8_factors_2d) {
  log_factorization_tests::test_factorization_log_8_factors_2d();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_many_factors_3d) {
  log_factorization_tests::test_factorization_log_many_factors_3d();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_8_factors_3d) {
  log_factorization_tests::test_factorization_log_8_factors_3d();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_many_categories) {
  log_factorization_tests::test_factorization_log_many_categories();
}
BOOST_AUTO_TEST_CASE(test_factorization_log_many_dimensions) {
  log_factorization_tests::test_factorization_log_many_dimensions();
}
BOOST_AUTO_TEST_CASE(test_factorization_regression) {
  log_factorization_tests::test_factorization_regression();
}
BOOST_AUTO_TEST_SUITE_END()
