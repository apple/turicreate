/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/data_storage/ml_data_block_manager.hpp>
#include <ml/ml_data/ml_data.hpp>

namespace turi { namespace ml_data_internal {

ml_data_block_manager::ml_data_block_manager(
    std::shared_ptr<ml_metadata> _metadata,
    const row_metadata& _rm,
    size_t _row_block_size,
    const std::shared_ptr<sarray<ml_data_internal::row_data_block> >& data_blocks,
    const std::vector<std::shared_ptr<sarray<flexible_type> > >& untranslated_columns)
    : metadata(_metadata)
    , rm(_rm)
    , row_block_size(_row_block_size)
{
  // Set up the row reader
  data_reader = data_blocks->get_reader();

  // Set up the untranslated column readers
  untranslated_column_readers.resize(untranslated_columns.size());

  for(size_t i = 0; i < untranslated_columns.size(); ++i) {
    untranslated_column_readers[i] = untranslated_columns[i]->get_reader();
  }
}

/** Returns a block corresponding to the block index.  Loads from
 *  disk if not in cache.
 */
std::shared_ptr<ml_data_block> ml_data_block_manager::get_block(size_t block_index) {

  std::unique_lock<turi::mutex> guard(this->cache_lock);

  // Possibly clear out the expired weak pointers. This step just
  // needs to be done periodically to make sure that the
  // row_block_cache doesn't fill up with empty weak pointers.  The
  // 256 doesn't really matter here, it would work the same if it was
  // larger or smaller, just need something to say it happens
  // periodically.
  if((++num_accesses) % 256 == 0) {
    for(auto it = row_block_cache.begin(); it != row_block_cache.end();) {
      if(it->second.expired()) {
        // Advances to the next element, or m_file_handles.end();
        it = row_block_cache.erase(it);
      } else {
        ++it;
      }
    }
  }

  auto it = row_block_cache.find(block_index);
  std::shared_ptr<ml_data_block> ret;

  if(it != row_block_cache.end()) {

    // Have it in cache, try to promote the weak_ptr.  If this fails,
    // erase the expired version; ret will be a null pointer in this
    // case.
    if (!(ret = it->second.lock())) {
      row_block_cache.erase(it);
    }
  }

  // It's not in cache, so create it.
  if(ret == nullptr) {

    // Release the lock for the reading portion below; no need to lock
    // everything while that is happening.
    guard.unlock();

    // Need to instantiate it.
    std::vector<row_data_block> row_block_buffer;

    data_reader->read_rows(block_index, block_index + 1, row_block_buffer);

    ////////////////////////////////////////////////////////////
    // Step 1.1: Do we have any untranslated columns?

    std::vector<std::vector<flexible_type> >
        untranslated_column_buffers(untranslated_column_readers.size());

    if(!untranslated_column_buffers.empty()) {

      // Fill out the untranslated column buffers
      size_t row_start_idx = block_index * row_block_size;
      size_t row_end_idx = (block_index + 1) * row_block_size;

      for(size_t i = 0; i < untranslated_column_readers.size(); ++i) {
        untranslated_column_readers[i]->read_rows(
            row_start_idx, row_end_idx, untranslated_column_buffers[i]);
      }
    }

    ret.reset(new ml_data_block
              {metadata,
                    rm,
                    std::move(row_block_buffer[0]),
                    std::move(untranslated_column_buffers)});

    // Reaquire the lock on the cache.
    guard.lock();
    auto new_it = row_block_cache.insert({block_index, ret});

    auto map_it = new_it.first;
    bool ret_value_inserted = new_it.second;

    if(!ret_value_inserted) {
      // Return the existing one to keep memory usage down.

      std::shared_ptr<ml_data_block> alt_ret;

      if((alt_ret = map_it->second.lock()) != nullptr) {
        ret = alt_ret;
      } else {

        // The one in there is bad; replace it.
        row_block_cache.erase(map_it);
        row_block_cache.insert({block_index, ret});
      }
    }
  }

  return ret;
}

}}
