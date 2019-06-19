/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <core/random/random.hpp>
#include <unity/server/toolkits/recsys/data.hpp>
#include <unity/server/toolkits/recsys/models.hpp>
#include <unity/server/toolkits/recsys/models/itemcf.hpp>
#include <unity/server/toolkits/recsys/data_generators.hpp>
#include <timer/timer.hpp>
#include <numerics/armadillo.hpp>
#include <core/storage/sframe_data/sframe.hpp>

const bool use_strings = false;

using namespace turi;
using namespace turi::recsys;

int main(int argc, char **argv) {

  if(argc != 4) {
    std::cerr << "Call format: " << argv[0] << " <n_users> <n_items> <n_observations> " << std::endl;
    exit(1);
  }

  size_t n_items = std::atoi(argv[1]);
  size_t n_users = std::atoi(argv[2]);
  size_t n_obs   = std::atoi(argv[3]);

  std::vector<flexible_type> users_3(n_obs), items_3(n_obs);

  sframe sf_1, sf_2, sf_3;

  for(sframe* sf_ptr : {&sf_1, &sf_2, &sf_3} ) {
    std::vector<flexible_type> users(n_obs), items(n_obs);

    for(size_t i = 0; i < n_obs; ++i) {

      users[i] = random::fast_uniform<size_t>(0, n_users-1);
      size_t item_1 = random::fast_uniform<size_t>(0, n_items-1);
      if(use_strings)
        items[i] = std::to_string(item_1) + "_" + std::to_string(size_t(hash64(item_1)));
      else
        items[i] = item_1;
    }

    dataframe_t raw_data;

    raw_data.set_column("users", users, flex_type_enum::INTEGER);
    raw_data.set_column("items", items, use_strings ? flex_type_enum::STRING : flex_type_enum::INTEGER);

    *sf_ptr = sframe(raw_data);
  }

  recsys_data train_data;

  std::cerr << ">>>>>>>>>>> Data Loaded <<<<<<<<<<<<<<" << std::endl;

  {
    timer tt;

    tt.start();
    train_data.set_primary_schema({
        recsys::schema_entry("users", recsys::schema_entry::CATEGORICAL, flex_type_enum::INTEGER),
            recsys::schema_entry("items", recsys::schema_entry::CATEGORICAL, flex_type_enum::INTEGER) });

    train_data.set_primary_observations(sf_1);

    train_data.finish();

    std::cerr << ">>>>>>>>>>> Initial load time was "
              << tt.current_time_millis()
              << "ms <<<<<<<<<<<<<<<" << std::endl;
  }

  // {
  //   recsys_data train_data_2 = train_data.clone_capsule();
  //   timer tt;

  //   tt.start();

  //   train_data_2.set_primary_observations(sf_1);
  //   train_data_2.finish();

  //   std::cerr << ">>>>>>>>>>> Hot load time, no new users = "
  //             << tt.current_time_millis()
  //             << "ms <<<<<<<<<<<<<<<" << std::endl;
  // }

  {
    recsys_data train_data_2 = train_data.clone_capsule();

    timer tt;

    tt.start();

    train_data.set_primary_observations(sf_2);

    train_data_2.finish();

    std::cerr << ">>>>>>>>>>> Hot load time, possible new users = "
              << tt.current_time_millis()
              << "ms <<<<<<<<<<<<<<<" << std::endl;
  }

  // {
  //   recsys_data train_data_2 = train_data.clone_capsule();

  //   timer tt;

  //   tt.start();


  //    if(test_sframe)
  //     train_data.set_primary_observations(sf_3);
  //   else
  //     train_data.set_primary_observations(raw_data_3);

  //    train_data_2.finish();

  //   std::cerr << ">>>>>>>>>>> Hot load time, all new users = "
  //             << tt.current_time_millis()
  //             << "ms <<<<<<<<<<<<<<<" << std::endl;
  // }

  return 0;
}
