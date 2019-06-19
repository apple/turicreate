/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/recsys/train_test_split.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <core/random/random.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>

namespace turi { namespace recsys {

std::pair<sframe, sframe> make_recsys_train_test_split(
    sframe data,
    const std::string& user_column_name,
    const std::string& item_column_name,
    size_t max_num_users, double item_test_proportion, size_t random_seed) {


  if(item_test_proportion < 0 || item_test_proportion > 1) {
    log_and_throw("Proportion of items to assign to test split not between 0 and 1.");
  }

  v2::ml_data::indexer_type user_indexer =
      v2::ml_data::create_indexer(user_column_name,
                                  v2::ml_column_mode::CATEGORICAL,
                                  data.column_type(data.column_index(user_column_name)),
                                  "unique");

  // Create a temporary sframe with the indexed values
  sframe user_id_sframe = sframe(
      {v2::map_to_indexed_sarray(user_indexer, data.select_column(user_column_name))},
      {"users"});

  size_t n_users = user_indexer->indexed_column_size();
  max_num_users = std::min(n_users, max_num_users);

  uint64_t inner_seed = hash64(random_seed, n_users, data.size());
  std::vector<bool> user_in_test(n_users, false);

  {
    std::vector<std::pair<uint64_t, size_t> > user_hashes(n_users);
    parallel_for(size_t(0), n_users, [&](size_t i) {
        uint64_t h = hash64(inner_seed, user_indexer->map_index_to_value(i).hash());
        user_hashes[i] = {h, i};
      });

    // We have to do it in an order determistic from the actual user
    // values since the indices and hence order are not deterministic.
    // Otherwise, the end result will still be random.
    std::nth_element(user_hashes.begin(),
                     user_hashes.begin() + max_num_users,
                     user_hashes.end());

    for(size_t i = 0; i < max_num_users; ++i) {
      user_in_test[user_hashes[i].second] = true;
    }
  }

  // Now go through and build a test set that filters
  // item_test_proportion of each selected user's items to the test
  // set.

  size_t num_segments = thread::cpu_count();

  sframe train_sf;
  sframe validation_sf;

  train_sf.open_for_write(data.column_names(), data.column_types(), "", num_segments);
  validation_sf.open_for_write(data.column_names(), data.column_types(), "", num_segments);

  parallel_sframe_iterator_initializer it_init({data, user_id_sframe});

  // Unfortunately, have to be creative with this; there can be numerical issues...
  uint64_t max_cutoff = hash64_proportion_cutoff(item_test_proportion);

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      DASSERT_LE(num_threads, num_segments);

      auto train_out = train_sf.get_output_iterator(thread_idx);
      auto validation_out = validation_sf.get_output_iterator(thread_idx);

      std::vector<flexible_type> out_row;

      for(parallel_sframe_iterator it(it_init, thread_idx, num_threads); !it.done(); ++it) {

        it.fill(0, out_row);

        size_t user_idx = it.value(1,0);
        DASSERT_LT(user_idx, user_in_test.size());

        // Don't include user_id in here; it's random.  Just use the
        // it.row_index(), which we assume is deterministic.
        uint64_t r_num = hash64(inner_seed, it.row_index());

        bool in_validation = (user_in_test[user_idx]
                              ? r_num < max_cutoff
                              : false);

        if(in_validation) {
          *validation_out = out_row;
          ++validation_out;
        } else {
          *train_out = out_row;
          ++train_out;
        }
      }
    });

  train_sf.close();
  validation_sf.close();

  return std::make_pair(train_sf, validation_sf);
}


}}
