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
#include <functional>

#include <core/random/random.hpp>

#include <toolkits/recsys/models.hpp>
#include <toolkits/util/data_generators.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>


#include <cfenv>

using namespace turi;
using namespace turi::recsys;


template <typename Model>
void run_test_new_users(const std::map<std::string, flexible_type>& opts
                        = std::map<std::string, flexible_type>()) {

  std::feraiseexcept(FE_ALL_EXCEPT);

  size_t n_items = 100;
  size_t n_users = 100;
  size_t n_obs   = 50;

  sframe train_data, test_data, test_data_2;

  random::seed(0);

  for(sframe* sf_ptr : {&train_data, &test_data, &test_data_2} ) {
    std::vector<std::vector<flexible_type> > data(n_obs);

    for(size_t i = 0; i < n_obs; ++i) {
      size_t user = random::fast_uniform<size_t>(0, n_users-1);
      size_t item = random::fast_uniform<size_t>(0, n_items-1);

      data[i] = {user, item, 1.0 / (1.0 + user + item)};
    }

    *sf_ptr = make_testing_sframe({"user", "item", "target"},
                                  {flex_type_enum::INTEGER, flex_type_enum::INTEGER, flex_type_enum::FLOAT},
                                  data);
  }

  std::shared_ptr<recsys::recsys_model_base> model(new Model);

  std::map<std::string, flexible_type> opts2;
  opts2["item_id"] = "item";
  opts2["user_id"] = "user";
  opts2["target"] = "target";
  model->init_options(opts2);
  model->setup_and_train(train_data);

  model->predict(model->create_ml_data(test_data));
  model->predict(model->create_ml_data(test_data_2));

  sframe indexed_test_data = v2::map_to_indexed_sframe(model->metadata, test_data);

  auto user_indexer = model->metadata->indexer(model->USER_COLUMN_INDEX);
  const std::string& user_column_name = model->metadata->column_name(model->USER_COLUMN_INDEX);

  sframe users_1({v2::map_to_indexed_sarray(
      user_indexer, test_data.select_column(user_column_name))},
    {"user"});

  sframe users_2({v2::map_to_indexed_sarray(
      user_indexer, test_data_2.select_column(user_column_name))},
    {"user"});

  model->recommend(users_1, 5);

  model->recommend(users_2, 5);
}


struct recsys_new_user_tests  {
 public:
  // ////////////////////////////////////////////////////////////////////////////////

  void test_new_users_factorization_model() {
    run_test_new_users<recsys::recsys_factorization_model>({{"max_iterations", 5}});
  }

  void test_new_users_matrix_factorization() {
    run_test_new_users<recsys::recsys_factorization_model>(
      { {"max_iterations", 5},
        {"side_data_factorization", false}});
  }

  void test_new_users_ranking_factorization_model() {
    run_test_new_users<recsys::recsys_ranking_factorization_model>({{"max_iterations", 5}});
  }

  void test_new_users_matrix_ranking_factorization() {
    run_test_new_users<recsys::recsys_ranking_factorization_model>(
      { {"max_iterations", 5},
        {"side_data_factorization", false}});
  }

  void test_new_users_popularity() {
    run_test_new_users<recsys::recsys_popularity>();
  }

  void test_new_users_itemcf_jaccard() {
    run_test_new_users<recsys::recsys_itemcf>(
      { {"similarity_type", "jaccard"} });
  }

  void test_new_users_itemcf_jaccard_topk() {
    run_test_new_users<recsys::recsys_itemcf>(
      { {"similarity_type", "jaccard"},
        {"only_top_k", 100} });
  }

  void test_new_users_itemcf_cosine() {
    run_test_new_users<recsys::recsys_itemcf>(
      { {"similarity_type", "cosine"} });
  }

  void test_new_users_itemcf_cosine_topk() {
    run_test_new_users<recsys::recsys_itemcf>(
      { {"similarity_type", "cosine"},
        {"only_top_k", 100} });
  }


  void test_new_users_itemcf_pearson() {
    run_test_new_users<recsys::recsys_itemcf>(
      { {"similarity_type", "pearson"} });
  }

  void test_new_users_itemcf_pearson_topk() {
    run_test_new_users<recsys::recsys_itemcf>(
      { {"similarity_type", "pearson"},
        {"only_top_k", 100} });
  }
};

BOOST_FIXTURE_TEST_SUITE(_recsys_new_user_tests, recsys_new_user_tests)
BOOST_AUTO_TEST_CASE(test_new_users_factorization_model) {
  recsys_new_user_tests::test_new_users_factorization_model();
}
BOOST_AUTO_TEST_CASE(test_new_users_matrix_factorization) {
  recsys_new_user_tests::test_new_users_matrix_factorization();
}
BOOST_AUTO_TEST_CASE(test_new_users_ranking_factorization_model) {
  recsys_new_user_tests::test_new_users_ranking_factorization_model();
}
BOOST_AUTO_TEST_CASE(test_new_users_matrix_ranking_factorization) {
  recsys_new_user_tests::test_new_users_matrix_ranking_factorization();
}
BOOST_AUTO_TEST_CASE(test_new_users_popularity) {
  recsys_new_user_tests::test_new_users_popularity();
}
BOOST_AUTO_TEST_CASE(test_new_users_itemcf_jaccard) {
  recsys_new_user_tests::test_new_users_itemcf_jaccard();
}
BOOST_AUTO_TEST_CASE(test_new_users_itemcf_jaccard_topk) {
  recsys_new_user_tests::test_new_users_itemcf_jaccard_topk();
}
BOOST_AUTO_TEST_CASE(test_new_users_itemcf_cosine) {
  recsys_new_user_tests::test_new_users_itemcf_cosine();
}
BOOST_AUTO_TEST_CASE(test_new_users_itemcf_cosine_topk) {
  recsys_new_user_tests::test_new_users_itemcf_cosine_topk();
}
BOOST_AUTO_TEST_CASE(test_new_users_itemcf_pearson) {
  recsys_new_user_tests::test_new_users_itemcf_pearson();
}
BOOST_AUTO_TEST_CASE(test_new_users_itemcf_pearson_topk) {
  recsys_new_user_tests::test_new_users_itemcf_pearson_topk();
}
BOOST_AUTO_TEST_SUITE_END()
