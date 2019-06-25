#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <string>
#include <functional>

#include <core/random/random.hpp>

#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <toolkits/recsys/models/factorization_models.hpp>
#include <toolkits/util/data_generators.hpp>
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

  if(model_type == "mf" || model_type == "logistic_mf")
    opts["only_2_factor_terms"] = true;

  std::feraiseexcept(FE_ALL_EXCEPT);

  size_t n_observations = opts.at("n_observations");
  opts.erase("n_observations");

  std::string target_column_name = "target";

  std::vector<std::string> column_names = {"user_id", "item_id"};

  DASSERT_NE(n_categorical_values[0], 0);
  DASSERT_NE(n_categorical_values[1], 0);

  /* for(size_t i = 2; i < n_categorical_values.size(); ++i) { */
  /*   column_names.push_back("C-" + std::to_string(i)); */
  /* } */

  lm_data_generator lmdata(column_names, n_categorical_values, opts);

  sframe train_data = lmdata.generate(n_observations, target_column_name, 0, 0);
  sframe test_data = lmdata.generate(n_observations, target_column_name, 1, 0);

  std::map<std::string, flexible_type> options =
      { {"solver", "auto"},
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

  if(model_type == "mf" || model_type == "logistic_mf")
    opts.erase("only_2_factor_terms");

  options.insert(opts.begin(), opts.end());

  typedef std::shared_ptr<recsys::recsys_model_base> model_ptr;
  auto new_model = [&]() -> model_ptr {
    model_ptr ret;
    ret.reset(new recsys_ranking_factorization_model);
    return ret;
  };

  model_ptr model = new_model();
  model->init_options(options);
  model->setup_and_train(train_data);

  auto model2 = new_model();

  {
    // Save it
    dir_archive archive_write;
    archive_write.open_directory_for_write("recsys_get_similar_cxx_tests");

    turi::oarchive oarc(archive_write);

    oarc << *model;

    archive_write.close();

    // Load it
    dir_archive archive_read;
    archive_read.open_directory_for_read("recsys_get_similar_cxx_tests");

    turi::iarchive iarc(archive_read);

    iarc >> (*model2);
    sframe y_hat_sf = model2->predict(model->create_ml_data(test_data));
  }

  logprogress_stream << "Getting similar items" << std::endl;
  auto chosen_items = std::vector<flexible_type>{0};
  auto items_sa = make_testing_sarray(flex_type_enum::INTEGER, chosen_items);
  auto chosen_users = std::vector<flexible_type>{0};
  auto users_sa = make_testing_sarray(flex_type_enum::INTEGER, chosen_users);

  size_t k = 5;
  sframe result_1 = model->get_similar_items(items_sa, k);
  sframe result_2 = model2->get_similar_items(items_sa, k);
  sframe result_3 = model->get_similar_users(users_sa, k);
  sframe result_4 = model2->get_similar_users(users_sa, k);
}

struct get_similar_tests  {
 public:

  ////////////////////////////////////////////////////////////////////////////////

  void test_mf() {
    std::map<std::string, flexible_type> opts = {
      {"n_observations",      100 } };

    run_exact_test({10, 10}, opts, "mf");
  }
};

BOOST_FIXTURE_TEST_SUITE(_get_similar_tests, get_similar_tests)
BOOST_AUTO_TEST_CASE(test_mf) {
  get_similar_tests::test_mf();
}
BOOST_AUTO_TEST_SUITE_END()
