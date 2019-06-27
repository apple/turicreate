/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>
#include <toolkits/ml_data_2/iterators/ml_data_iterator.hpp>
#include <toolkits/ml_data_2/side_features.hpp>
#include <toolkits/ml_data_2/data_storage/util.hpp>
#include <core/util/basic_types.hpp>

using namespace turi::v2::ml_data_internal;

namespace turi { namespace v2 {


////////////////////////////////////////////////////////////////////////////////
//
//  Sorting routines
//
////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<ml_data_iterator>
ml_data::_merge_sorted_ml_data_sources(
    const std::vector<std::unique_ptr<ml_data_iterator> >& sources) {

  DASSERT_TRUE(metadata()->column_mode(0) == ml_column_mode::CATEGORICAL);
  DASSERT_TRUE(metadata()->column_mode(1) == ml_column_mode::CATEGORICAL);

  ml_data out = *this;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set up the source heap.  All we are doing is a simple
  // merge sort over a heap of the lowest values on each stack.

  struct next_element_picker {
    size_t sort_idx_1;
    size_t sort_idx_2;
    size_t src_index;

    inline bool operator<(const next_element_picker& other) const {
      // Swap operator for descending order in merge queue; thus the
      // priority_queue picks off the smallest element at each point.
      return ((sort_idx_1 == other.sort_idx_1)
              ? sort_idx_2 > other.sort_idx_2
              : sort_idx_1 > other.sort_idx_1);
    }
  };

  std::priority_queue<next_element_picker> merge_queue;

  // Init the priority_queue.
  for(size_t i = 0; i < sources.size(); ++i) {
    size_t sort_idx_1 = sources[i]->_raw_row_entry(0).index_value;
    size_t sort_idx_2 = sources[i]->_raw_row_entry(1).index_value;

    merge_queue.push({sort_idx_1, sort_idx_2, i});
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Set up the output SArray for writing out the blocks.

  out.data_blocks.reset(new sarray<row_data_block>);
  out.data_blocks->open_for_write(1);
  auto it_out = out.data_blocks->get_output_iterator(0);

  row_data_block block;
  size_t rows_in_block = 0;
  size_t total_rows = 0;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Dump everything into the blocks.

  while(!merge_queue.empty()) {

    // Grab the top element from the correct source; dump to the block.
    size_t next_element_source = merge_queue.top().src_index;

    merge_queue.pop();

    auto& it_ptr = sources[next_element_source];
    DASSERT_FALSE(it_ptr->done());

    // Get the pair of iterators refering to that data source to copy
    // it into this block.
    entry_value_iterator it = it_ptr->current_data_iter();

    append_row_to_row_data_block(rm, block, it);
    ++rows_in_block, ++total_rows;

    // Advance that iterator.
    ++(*it_ptr);

    // Refresh the queue if that iterator has more in it.  If it does
    // not, than don't add it back in.  This will effectively remove
    // that iterator from consideration.
    if(!it_ptr->done()) {
      size_t sort_idx_1 = it_ptr->_raw_row_entry(0).index_value;
      size_t sort_idx_2 = it_ptr->_raw_row_entry(1).index_value;

      merge_queue.push({sort_idx_1, sort_idx_2, next_element_source});
    }

    // If the output block is full, then write it to the output
    // iterator and reset the blocks.
    if(rows_in_block == row_block_size) {
      *it_out = block;
      ++it_out;
      block.entry_data.clear();
      rows_in_block = 0;
    }
  }

  // Flush to the output sarray if needed.
  if(!block.entry_data.empty())
    *it_out = std::move(block);

  // Clean up the output ml_data structure.
  out.data_blocks->close();

  // Clean up ourselves; get the ml_data structure in a usable state.
  out._create_block_manager();
  out._row_start = 0;
  out._row_end = total_rows;

  return std::unique_ptr<ml_data_iterator>(new ml_data_iterator(out.get_iterator(0,1)));
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

void ml_data::_sort_user_item_data_blocks() {

  if(num_rows() == 0)
    return;

  static constexpr size_t blocks_per_merge = 8;

  ////////////////////////////////////////////////////////////////////////////////
  // Overall algorithm.
  //
  // The overall algorithm is just a smart implementation of
  // merge-sort.  To schedule the parallel threads, we explicitly
  // instantiate the merge tree over the partitions of the data.  The
  // leaves of the tree are over a set of blocks of rows in which each
  // block has been sorted previously.  Processing proceeds by
  // iteratively choosing a node in which all branches have been
  // processed, then merging those branches.
  //
  // In the parallel setting, it becomes more complicated to schedule
  // it.  To make it efficient and easy, we use the standard
  // cache-oblivious algorithm design technique.  ("In computing, a
  // cache-oblivious algorithm is an algorithm designed to take
  // advantage of a CPU cache without having the size of the cache as
  // an explicit parameter" --
  // http://en.wikipedia.org/wiki/Cache-oblivious_algorithm).  In this
  // setting, this means that all processing threads do everything
  // they can with the data they are currently working with before
  // going on to new data.
  //
  // As a result, we do the merge-sort by depth first search.  Each
  // thread iteratively does the following steps:
  //
  //
  // Step 2.1: Instantiate and claim the next depth-first-search (dfs)
  // path to an unprocessed leaf.  If no new DFS path remains, then
  // exit.
  //
  // Step 2.2: Process everything on the leaf.  The leaf then holds
  // just the sorted output.
  //
  // Step 2.3: Walk up the DFS path towards the root.  If it is the
  // last thread to be processing in a node -- i.e. all branches of
  // that node have sorted outputs ready -- then merge-sort all the
  // branches into that node.
  //
  //
  // When all threads have completed these steps, the output of the
  // final node will be a sorted ml_data object.



  ////////////////////////////////////////////////////////////////////////////////-
  // Step 1: Define the merge node structure.

  struct merge_node {

    // The input source type can be one of two possible types:
    //
    // OTHER_NODE: Another merge_node.  In this case, input_indices
    // below gives the range of indices in the processing_queue of the
    // branch merge_nodes.
    //
    // ML_DATA_BLOCK: A root ml_data leaf.  In this case,
    // input_indices below gives the range of block indices from which
    // the raw data will be drawn.

    enum class input_type {OTHER_NODE, ML_DATA_BLOCK};
    input_type input_source_type;

    std::pair<size_t, size_t> input_indices;

    // Once the processing is done on this node, this holds the sorted
    // output.
    std::unique_ptr<ml_data_iterator> sorted_output;

    // Keep track of the number of indices that have been completed.
    // The thread that completes the last branch gets to work on this
    // one.  Unused in leaf nodes.
    std::unique_ptr<atomic<size_t> > n_source_indices_completed;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2. Set up the processing queue.  The entire sorting pipeline
  // is determined ahead of time, since this makes things much easier
  // to parallelize.

  std::vector<merge_node> processing_queue;

  {

    ////////////////////////////////////////////////////////////
    // Step 2.1. Set up the initial sources as the leaves.

    for(size_t i = 0; i < ceil_divide(data_blocks->size(), blocks_per_merge); ++i) {

      merge_node new_node;

      new_node.input_source_type = merge_node::input_type::ML_DATA_BLOCK;
      size_t block_start_index = i*blocks_per_merge;
      size_t block_end_index = std::min((i+1)*blocks_per_merge, data_blocks->size());

      new_node.input_indices = {block_start_index, block_end_index};

      processing_queue.push_back(std::move(new_node));
    }

    /////////////////////////////////////////////////////////////
    // Step 2.2.  Set up the all the latter merges

    // Merging happens like:
    //
    // [ n1 n2 n3 n4 n5 .... n10 ]
    //   ^                       ^
    //   merge_start             merge_end
    //
    // after one loop below:
    //
    // [ n1 n2 n3 n4 n5 .... | merge(n1, n2), ..., merge(n9,n10)]
    //
    //                         ^                                ^
    //                         merge_start                      merge_end
    // And so on.

    // The start of the segment of nodes we want to merge
    size_t merge_start = 0;

    // End condition: when everything has been merged and there is
    // only one left to merge.
    while(processing_queue.size() - merge_start > 1) {

      // Go through and merge everything up to this point
      size_t merge_end = processing_queue.size();

      size_t n_blocks = ceil_divide(merge_end - merge_start, blocks_per_merge);

      // Divide the merge block between all of these
      for(size_t i = 0; i < n_blocks; ++i) {
        size_t segment_start_index = merge_start + i*blocks_per_merge;
        size_t segment_end_index   = std::min(segment_start_index + blocks_per_merge, merge_end);

        merge_node new_node;

        new_node.input_source_type = merge_node::input_type::OTHER_NODE;
        new_node.input_indices = {segment_start_index, segment_end_index};
        new_node.n_source_indices_completed.reset(new atomic<size_t>(0));

        processing_queue.push_back(std::move(new_node));
      }

      merge_start = merge_end;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3. Initialize the dfs paths.
  //
  // The dfs_path_to_next_leaf is always set to the DFS path to the next
  // leaf node for processing.
  //
  // Ordering is done by index of the leaf node it ends up at.

  ////////////////////
  // The dfs path node
  struct dfs_path_node {

    // The current node in the processing_queue that we're dealing
    // with.
    merge_node* node;

    // The branch index we are working on within that node.  Unused in
    // the leaf node.
    size_t source_index_within_node;
  };

  std::vector<dfs_path_node> dfs_path_to_next_leaf;

  ////////////////////
  // Set it to the first leaf; this is the first input_index on all nodes.

  dfs_path_to_next_leaf.push_back({&processing_queue.back(), 0});

  while(dfs_path_to_next_leaf.back().node->input_source_type == merge_node::input_type::OTHER_NODE) {
    const auto& node = dfs_path_to_next_leaf.back().node;
    size_t next_input_source_node = node->input_indices.first;

    dfs_path_to_next_leaf.push_back( {&processing_queue[next_input_source_node], 0} );
  }


  ////////////////////////////////////////////////////////////////////////////////
  // Step 4. Run all the merges.

  mutex path_advance_lock;

  in_parallel([&](size_t, size_t) {

      while(true) {

        ////////////////////////////////////////////////////////////////////////////////
        // Step 4.1. Acquire the next path

        path_advance_lock.lock();

        // We are done; just leave.
        if(dfs_path_to_next_leaf.empty()) {
          path_advance_lock.unlock();
          return;
        }

        std::vector<dfs_path_node> current_dfs_path = dfs_path_to_next_leaf;

        ////////////////////////////////////////////////////////////////////////////////
        // Step 4.2. Advance the dfs_path_to_next_leaf to the next processing location

        // The tip of all the paths is always a leaf node, which is an
        // ML_DATA_BLOCK. The next point is the next leaf node.
        DASSERT_TRUE(dfs_path_to_next_leaf.back().node->input_source_type == merge_node::input_type::ML_DATA_BLOCK);

        ////////////////////////////////////////
        //
        // Step 4.2.1. Walk back up the dfs path towards the root.  Stop
        // at the next place we can go back down to a leaf.

        // Go up one to get off the current leaf.
        dfs_path_to_next_leaf.pop_back();

        while(true) {

          if(dfs_path_to_next_leaf.empty()) {
            // We are done after current_dfs_path is done; skip all
            // updates on the dfs_path_to_next_leaf and go process that.
            break;
          }

          // Se if we are done with this node; if so, then move one
          // more towards the root to find the next place to go down.
          merge_node*& node    = dfs_path_to_next_leaf.back().node;
          size_t& source_index = dfs_path_to_next_leaf.back().source_index_within_node;

          // Advance horizontally on this node
          ++source_index;

          // Have we hit the end?
          if(source_index == (node->input_indices.second - node->input_indices.first)) {

            // Yes, we hit the end.  Back up.
            dfs_path_to_next_leaf.pop_back();
          } else {

            // Nope, we've found a new way to descend.
            break;
          }
        }

        if(!dfs_path_to_next_leaf.empty()) {

          ////////////////////////////////////////
          // Step 4.2.2. Advance to the next branch

          ////////////////////////////////////////
          // Step 4.2.3. Head down to next leaf.  Since all nodes are
          // cleaned up by the last thread to leave a node, new
          // processing is always done starting at the leaf.

          while(dfs_path_to_next_leaf.back().node->input_source_type == merge_node::input_type::OTHER_NODE) {
            merge_node* node               = dfs_path_to_next_leaf.back().node;
            size_t next_input_source_index = dfs_path_to_next_leaf.back().source_index_within_node;

            merge_node* next_node = &(processing_queue[node->input_indices.first + next_input_source_index]);

            // This node has not been processed yet
            DASSERT_TRUE(next_node->sorted_output == nullptr);

            // Start at the end of the road
            dfs_path_to_next_leaf.push_back({next_node, 0});
          }
        }

        path_advance_lock.unlock();

        ////////////////////////////////////////////////////////////////////////////////
        // Step 4.3.  Run the current leaf node.  We always have one.
        {
          merge_node* node = current_dfs_path.back().node;

          DASSERT_TRUE(node->input_source_type == merge_node::input_type::ML_DATA_BLOCK);

          ////////////////////////////////////////////////////////////
          // Step 4.3.1: At the lowest level, we know that each block is
          // sorted.  Thus we do a heap-merge on each block.  Our
          // heap-merge routine uses ml_data_iterators as sources, so
          // set that up.
          size_t n_sources = node->input_indices.second - node->input_indices.first;
          std::vector<std::unique_ptr<ml_data_iterator> > sorted_sources(n_sources);

          for(size_t i = 0; i < n_sources; ++i) {

            // The input_indices in the node are over blocks, with
            // each block assumed to be sorted.  Thus we need to
            // convert this to rows to get a single block.
            size_t start_row_idx = (node->input_indices.first + i) * row_block_size;
            size_t end_row_idx   = std::min(start_row_idx + row_block_size, _original_num_rows);

            // Set up an iterator only on the rows in that block.
            sorted_sources[i].reset(
                new ml_data_iterator(
                    this->absolute_slice(start_row_idx, end_row_idx).get_iterator(0, 1)));
          }

          ////////////////////////////////////////////////////////////
          // Step 4.3.2: Run the heap-merge from each of the sources.

          // We should never be overwriting existing input
          DASSERT_TRUE(node->sorted_output == nullptr);

          if(sorted_sources.size() == 1) {
            node->sorted_output = std::move(sorted_sources[0]);
          } else {
            node->sorted_output = _merge_sorted_ml_data_sources(sorted_sources);
          }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Step 4.4.  Back up the path, cleaning up all nodes on which
        // we are the last ones to leave.

        while(true) {

          ////////////////////////////////////////////////////////////
          // Step 4.4.1.  Pop the node, so we walk back up the stack.

          current_dfs_path.pop_back();

          if(current_dfs_path.empty())
            break;

          merge_node* node = current_dfs_path.back().node;

          DASSERT_TRUE(node->input_source_type == merge_node::input_type::OTHER_NODE);

          size_t local_completion_index = ++( *(node->n_source_indices_completed));

          size_t n_sources = node->input_indices.second - node->input_indices.first;

          DASSERT_LE(local_completion_index, n_sources);

          if(local_completion_index < n_sources) {
            // We are done; another thread / later run will do the
            // merge on this node.
            break;

            ////////////////////////////////////////////////////////////
            // Step 4.4.2: We are the last thread to leave this processing
            // node, so we get cleanup duty -- merge all the branches.
          } else {

            //////////////////////////////////////////////////
            // Step 4.4.2.1 -- set up the input sources from the nodes
            // the current node depends on.
            std::vector<std::unique_ptr<ml_data_iterator> > sorted_sources(n_sources);

            for(size_t i = 0; i < n_sources; ++i) {
              merge_node* branch_node = &(processing_queue[node->input_indices.first + i]);

              DASSERT_TRUE(branch_node->sorted_output != nullptr);

              // These won't be needed after the merge, so the move is appropriate.
              sorted_sources[i] = std::move(branch_node->sorted_output);
            }

            //////////////////////////////////////////////////
            // Step 4.4.2.2 -- Run the merge sort; save the outpuao;t in
            // our current output.

            if(sorted_sources.size() == 1) {
              node->sorted_output = std::move(sorted_sources[0]);
            } else {
              node->sorted_output = _merge_sorted_ml_data_sources(sorted_sources);
            }
          }
        }
      } // End while of main loop.
    });

  ////////////////////////////////////////////////////////////////////////////////
  // Step 5. Checks.

#ifndef NDEBUG

  // Ensure that all our invariant conditions are satisfied.
  for(size_t i = 0; i < processing_queue.size(); ++i) {
    merge_node* node = &(processing_queue[i]);

    if(i != processing_queue.size() - 1)
      DASSERT_TRUE(node->sorted_output == nullptr);
    else
      DASSERT_TRUE(processing_queue.back().sorted_output != nullptr);

    size_t n_sources = node->input_indices.second - node->input_indices.first;

    if(node->n_source_indices_completed != nullptr)
      DASSERT_TRUE( *(node->n_source_indices_completed) == n_sources);
  }

#endif

  ////////////////////////////////////////////////////////////////////////////////
  // Step 6.  Get the data. Now the last merge_node in the processing
  // queue contains the

  const ml_data& sorted_data = processing_queue.back().sorted_output->ml_data_source();

  DASSERT_EQ(sorted_data.num_rows(), num_rows());

  this->data_blocks   = sorted_data.data_blocks;
  this->block_manager = sorted_data.block_manager;
}

}}
