/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/recsys/user_item_lists.hpp>
#include <toolkits/recsys/recsys_model_base.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <algorithm>
#include <core/parallel/pthread_tools.hpp>

namespace turi { namespace recsys {

/** Make users' (item, rating) lists by user.
 *
 *  In this case, the ml_data structure for the user-item lists is
 *  must be sorted by rows, with the first column being the user
 *  column.
 *
 *  This operation is done without loading the data into memory, as it
 *  takes advantage of the sorting in the ml_data internals.
 *
 *  The user column is assumed to be the first column, and the item
 *  column is assumed to be the second column.
 */
std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >
make_user_item_lists(const v2::ml_data& data) {

  size_t num_segments = thread::cpu_count();

  auto out = std::make_shared<sarray<std::vector<std::pair<size_t, double> > > >();

  out->open_for_write(num_segments + 1);

  atomic<size_t> num_users_indexed = 0;

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      auto it_out = out->get_output_iterator(thread_idx);

      std::vector<std::pair<size_t, double> > users_items;
      std::vector<v2::ml_data_entry> row;

      // Use the block iterator to ensure we get all users in one go.
      for(auto it = data.get_block_iterator(thread_idx, num_threads, false); !it.done();) {
        it.fill_observation(row);

        size_t item = row[recsys_model_base::ITEM_COLUMN_INDEX].index;
        double target = it.target_value();

        users_items.push_back({item, target});

        // Increment the current location.
        ++it;

        // If the new location is the start of a new block, flush the
        // last one out and clear.
        if(it.is_start_of_new_block() || it.done()) {

          // Assert that it is indeed sorted
          DASSERT_TRUE(std::is_sorted(users_items.begin(), users_items.end(),
                                      [&](const std::pair<flexible_type, flexible_type>& a,
                                         const std::pair<flexible_type, flexible_type>& b) {
                                        return a.first < b.first;
                                      }));
          // Remove any duplicate items by averaging the targets
          auto it_write = users_items.begin();
          auto it_read_b_start = users_items.begin();
          auto it_read_b_end = users_items.begin();

          while(it_read_b_start != users_items.end()) {

            // Advance the end iterator to the end of the next block,
            // tracking the current sum/count.
            size_t count = 0;
            double total = 0;
            do{
              total += (double)it_read_b_end->second;
              ++count;
              ++it_read_b_end;
            } while(it_read_b_end != users_items.end()
                    && it_read_b_end->first == it_read_b_start->first);

            *it_write = {it_read_b_start->first, total / count};
            // Write to a unique location
            ++it_write;

            it_read_b_start = it_read_b_end;
          };

          // Resize to only the unique items with averaged targets
          users_items.resize(it_write - users_items.begin());

          *it_out = users_items;
          ++it_out;
          ++num_users_indexed;
          users_items.clear();
        }
      }
    });


  auto it_out = out->get_output_iterator(num_segments);

  for(;num_users_indexed < data.metadata()->column_size(0); ++it_out, ++num_users_indexed)
    *it_out = std::vector<std::pair<size_t, double> >();

  out->close();

  return out;
}


}}
