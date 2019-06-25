/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_BLOCK_MANAGER_H_
#define TURI_DML_DATA_BLOCK_MANAGER_H_

#include <ml/ml_data/data_storage/ml_data_row_format.hpp>

namespace turi {
namespace ml_data_internal {


/** This struct holds two components -- the first is the translated
 *  row data, which gives the compact format for rows converted to
 *  numerical format.  This row format is described in
 *  ml_data_row_format.hpp.
 *
 *  The second is the untranslated_columns list.  If there are
 *  untranslated columns, then this will hold those values.
 *
 */
struct ml_data_block {
  std::shared_ptr<ml_metadata> metadata;
  row_metadata rm;
  row_data_block translated_rows;
  std::vector<std::vector<flexible_type> > untranslated_columns;
};

/**  Implements a cache for the row block pool.  This is (1) an
 *   optimization to make iteration generally faster, especially on
 *   small ml_data instances when the blocks are likely to overlap,
 *   and (2) needed to enable the use of row references as a way to
 *   refer to a part of a block.
 */
class ml_data_block_manager {
 public:

  /** Constructor + opens the readers.
   */
  ml_data_block_manager(
      std::shared_ptr<ml_metadata> metadata,
      const row_metadata& rm,
      size_t row_block_size,
      const std::shared_ptr<sarray<ml_data_internal::row_data_block> >& data_blocks,
      const std::vector<std::shared_ptr<sarray<flexible_type> > >& untranslated_columns);

  /** Returns a block corresponding to the block index.  Loads from
   *  disk if not in cache.
   */
  std::shared_ptr<ml_data_block> get_block(size_t block_index);

  typedef std::shared_ptr<typename sarray<ml_data_internal::row_data_block>::reader_type> block_reader;

  block_reader get_reader() const { return data_reader; }

 private:

  /**  The metadata associated with the current block.
   */
  std::shared_ptr<ml_metadata> metadata;

  /**  The row metadata for the row block.
   */
  row_metadata rm;

  /** The number of rows in each block.
   */
  size_t row_block_size = -1;

  /** A reader which is shared by any ml_data_iterators; thus it does
   *  not need to be opened multiple times.
   */
  block_reader data_reader;

  /** Readers for the storage containers that are shared by any of the
   *  ml_data_iterators; thus they do not need to be opened multiple
   *  times.
   */
  std::vector<std::shared_ptr<typename sarray<flexible_type>::reader_type> > untranslated_column_readers;

  /** Lock for the cache.
   */
  turi::mutex cache_lock;

  /** A simple counter to make sure that periodically we flush out
   *  expired pointers from the cache.
   */
  size_t num_accesses = 0;

  /**  The map of cached blocks.
   */
  std::map<size_t, std::weak_ptr<ml_data_block> > row_block_cache;

};

}}

#endif
