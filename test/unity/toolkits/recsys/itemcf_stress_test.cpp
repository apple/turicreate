/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/*
 * Copyright (c) 2013 Turi
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.turicreate.com
 *
 */
#include <vector>
#include <string>
#include <random/random.hpp>
#include <unity/toolkits/recsys/models.hpp>
#include <sframe/testing_utils.hpp>
#include <util/testing_utils.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>

#include <numerics/armadillo.hpp>

using namespace turi;

int main(int argc, char** argv) {

  while(true) {


  size_t n_items = 28*1000*1000;
  size_t n_users = 25*1000*1000;
  size_t n_observations = 130*1000*1000;
  size_t top_k = 5;


  auto tr_dist = [](size_t k) {
    random::fast_uniform<size_t>(0, k);
    return random::fast_uniform<size_t>(0, std::max<size_t>(1, k));
  };

  size_t n_threads = thread::cpu_count();

  sframe data;
  data.open_for_write({"users", "items"},
                     {flex_type_enum::INTEGER, flex_type_enum::INTEGER}, "", n_threads);

  in_parallel([&](size_t thread_idx, size_t n_threads) {

      auto it_out = data.get_output_iterator(thread_idx);

      std::vector<flexible_type> row;

      for(size_t i = 0; i < n_observations / n_threads; ++i, ++it_out) {
        *it_out = row = {tr_dist(n_users), tr_dist(n_items)};
      }
    });

  data.close();

  auto model = std::make_shared<recsys::recsys_itemcf>();

  std::map<std::string, flexible_type> opts;
  opts["item_id"] = "items";
  opts["user_id"] = "users";
  opts["target"] = "";
  opts["similarity_type"] = "jaccard";
  opts["training_method"] = "sgraph";
  opts["only_top_k"] = top_k;

  model->init_options(opts);
  model->setup_and_train(data);

  size_t topk = 5;
  sframe restriction_sf = sframe();
  sframe exclusion_sf = sframe();
  sframe new_user_data = sframe();
  sframe new_item_data = sframe();
  sframe new_observations = sframe();
  bool exclude_training_interactions = true;

  sframe recs = model->recommend(sframe(), topk,
                                 restriction_sf,
                                 exclusion_sf,
                                 new_observations,
                                 new_user_data, new_item_data,
                                 exclude_training_interactions);


  }

  return 0;
}
