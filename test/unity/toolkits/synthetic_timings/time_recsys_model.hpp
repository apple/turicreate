/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_TIME_MODEL_H_
#define TURI_RECSYS_TIME_MODEL_H_

#include <vector>
#include <string>
#include <core/random/random.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/recsys/models.hpp>
#include <toolkits/util/data_generators.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>
#include <timer/timer.hpp>

using namespace turi;
using namespace turi::recsys;

template <typename Model>
void do_timing_run(size_t n_users, size_t n_items, size_t n_observations,
                   const std::map<std::string, flexible_type>& data_gen_options,
                   std::map<std::string, flexible_type> model_options) {

  lm_data_generator lmdata({"user_id", "item_id"}, {n_users, n_items}, data_gen_options);

  sframe train_data = lmdata.generate(n_observations, "target", 0, 0);
  sframe test_data = lmdata.generate(n_observations, "target", 1, 0);

  // Test Jaccard similarity
  std::shared_ptr<recsys_model_base> model(new Model);

  model_options["target"] = "target";

  model->init_options(model_options);

  {
    timer tt;

    tt.start();
    model->setup_and_train(train_data);

    std::cerr << ">>>>>>>>>>> Train time was "
              << tt.current_time_millis()
              << "ms <<<<<<<<<<<<<<<" << std::endl;
  }

  v2::ml_data test_data_ml;

  {
    timer tt;

    tt.start();
    test_data_ml = model->create_ml_data(test_data);

    std::cerr << ">>>>>>>>>>> Conversion time of test set to ml_data was "
              << tt.current_time_millis()
              << "ms <<<<<<<<<<<<<<<" << std::endl;
  }

  {
    timer tt;

    tt.start();
    model->predict(test_data_ml);

    std::cerr << ">>>>>>>>>>> Prediction time was "
              << tt.current_time_millis()
              << "ms <<<<<<<<<<<<<<<" << std::endl;
  }

  {
    timer tt;

    size_t max_user_id = 500;

    std::vector<flexible_type> user_list(max_user_id);
    for(size_t i = 0; i < max_user_id; ++i)
      user_list[i] = i;

    std::shared_ptr<sarray<flexible_type> > user_dvs = make_testing_sarray(flex_type_enum::INTEGER, user_list);
    sframe users_query = sframe({user_dvs}, {model_options["user_id"]});

    tt.start();
    model->recommend(users_query, 5);

    std::cerr << ">>>>>>>>>>> Top 5 rank time was "
              << tt.current_time_millis() / max_user_id
              << "ms / user<<<<<<<<<<<<<<<" << std::endl;

    tt.start();

    model->recommend(users_query, 100);

    std::cerr << ">>>>>>>>>>> Top 100 rank time was "
              << tt.current_time_millis() / max_user_id
              << "ms / user<<<<<<<<<<<<<<<" << std::endl;
  }
}

#endif /* _TEST_MODEL_H_ */
