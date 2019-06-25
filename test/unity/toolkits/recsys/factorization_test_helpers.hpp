/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TEST_FACTORIZATION_TEST_HELPERS_H_
#define TURI_TEST_FACTORIZATION_TEST_HELPERS_H_

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
#include <model_server/lib/variant.hpp>


#include <cfenv>

using namespace turi;
using namespace turi::recsys;

void _test_convergence(
    const std::vector<size_t>& n_categorical_values,
    std::map<std::string, flexible_type> opts,
    const std::string& model_type,
    bool include_side_features) {

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

  std::vector<std::string> column_names = {"user_id", "item_id"};

  DASSERT_NE(n_categorical_values[0], 0);
  DASSERT_NE(n_categorical_values[1], 0);

  for(size_t i = 2; i < n_categorical_values.size(); ++i) {
    column_names.push_back("C-" + std::to_string(i));
  }

  lm_data_generator lmdata(column_names, n_categorical_values, opts);
  sframe train_data = lmdata.generate(n_observations, target_column_name, 0, 0.1);
  sframe test_data  = lmdata.generate(n_observations, target_column_name, 1, 0.1);

  ASSERT_TRUE(train_data.num_rows() == n_observations); 
  ASSERT_TRUE(test_data.num_rows() == n_observations); 

  lm_data_generator lmdata_user({"user_id", "U2"}, {n_categorical_values[0], 16}, opts);
  sframe user_data = lmdata_user.generate(100, "U3", 0, 0.1);

  lm_data_generator lmdata_item({"item_id", "I2"}, {n_categorical_values[1], 16}, opts);
  sframe item_data = lmdata_item.generate(100, "I3", 0, 0.1);
  
  std::map<std::string, flexible_type> options =
      { {"solver", "auto"},
        {"binary_target", binary_target},
        {"target", target_column_name},
        {"sgd_sampling_block_size", 2},
        {"max_iterations", 5} };

  opts.erase("y_mode");

  if(model_type == "mf" || model_type == "logistic_mf")
    opts.erase("only_2_factor_terms");

  options.insert(opts.begin(), opts.end());

  // Instantiate some alternate versions to make sure the save and load work
  typedef std::shared_ptr<recsys::recsys_model_base> model_ptr;

restart_model_training:

  model_ptr model(new recsys_ranking_factorization_model);

  model->init_options(options);

  ASSERT_TRUE(train_data.num_rows() == n_observations); 
  ASSERT_TRUE(test_data.num_rows() == n_observations); 
  
  if(include_side_features)
    model->setup_and_train(train_data, user_data, item_data);
  else
    model->setup_and_train(train_data);

  // All we do is test that the training decreased it, and that
  // predict is the same across save and load

  std::map<std::string, variant_type> state = model->get_state();

  ASSERT_TRUE(state.count("coefficients"));
  ASSERT_TRUE(state.count("training_stats"));

  auto training_stats
      = variant_get_value<std::map<std::string, variant_type> >(state["training_stats"]);

  double initial_objective = variant_get_value<double>(training_stats["initial_objective_value"]);
  double final_objective = variant_get_value<double>(training_stats["final_objective_value"]);
  double initial_sgd_step = variant_get_value<double>(training_stats["sgd_step_size"]);

  if(final_objective < initial_objective) {
    if(options["max_iterations"] != 25) {
      options["max_iterations"] = 25;
      goto restart_model_training;
    } else {
      ASSERT_LT(final_objective, initial_objective);
    }
  }

  if(train_data.size() != 0)
    ASSERT_LT(1e-16, initial_sgd_step);

  std::vector<model_ptr> all_models =
      {model,
       model_ptr(new recsys_ranking_factorization_model)};


  sframe y_hat_sf_ref = model->predict(model->create_ml_data(train_data));
  std::vector<double> y_hat_ref = testing_extract_column<double>(y_hat_sf_ref.select_column(0));

  save_and_load_object((*all_models[1]), model);

  for(model_ptr m : all_models) {

    // ASSERT_TRUE(model->get_state() == m->get_state());

    sframe y_hat_sf = m->predict(m->create_ml_data(train_data));
    std::vector<double> y_hat = testing_extract_column<double>(y_hat_sf.select_column(0));

    ASSERT_EQ(y_hat.size(), y_hat_ref.size());

    for(size_t i = 0; i < y_hat.size(); ++i) {
      ASSERT_EQ(y_hat[i], y_hat_ref[i]);
    }

    if(options.count("num_factors") && options.at("num_factors") > 0) {
      sframe out_sim_items = m->get_similar_items(item_data.select_column("item_id"), 5);
      sframe out_sim_users = m->get_similar_users(user_data.select_column("user_id"), 5);
    }
  }
}


void test_convergence(
    const std::vector<size_t>& n_categorical_values,
    const std::map<std::string, flexible_type>& _opts,
    const std::string& model_type) {

  static mutex cout_lock;
  const char* reg_type_v[]   = {"normal", "weighted"};
  const char* solver_v[]     = {"sgd", "adagrad"};
  double rank_reg_v[]        = {0.1, 0.0};
  double reg_v[]             = {0.0, 0.01, 100.0};
  bool use_side_features_v[] = {false, true};

  parallel_for(size_t(0), size_t(2*2*2*3*2), [&](size_t main_idx) {
      const char* solver     = solver_v[(main_idx / (2*2*3*2)) % 2];
      const char* reg_type   = reg_type_v[(main_idx / (2*3*2)) % 2];
      double rank_reg        = rank_reg_v[(main_idx / (3*2)) % 2];
      double reg             = reg_v[(main_idx / 2) % 3];
      bool use_side_features = use_side_features_v[main_idx % 2];

      std::map<std::string, flexible_type> opts = _opts;

      opts["solver"] = solver;
      opts["regularization_type"] = reg_type;
      opts["ranking_regularization"] = rank_reg;
      opts["regularization"] = reg;

      cout_lock.lock();
      std::cerr << "############################################################" << std::endl;
      std::cerr << model_type
                << ": solver=" << solver
                << ": reg_type=" << reg_type
                << "; rank_reg=" << rank_reg
                << "; reg=" << reg
                << "; side=" << use_side_features
                << std::endl;
      cout_lock.unlock();

      _test_convergence(n_categorical_values, opts, model_type, use_side_features);
  });
}


#endif /* _FACTORIZATION_TEST_HELPERS_H_ */
