/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/join_impl.hpp>
#include <core/system/cppipc/server/cancel_ops.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>

namespace turi {
namespace join_impl {

/****************** join_hash_table **********************/

const join_hash_table::value_type join_hash_table::empty_vt = {};

bool join_hash_table::add_row(const std::vector<flexible_type> &row) {

  size_t the_hash_key = compute_hash_from_row(row, _hash_positions);
  bool first_entry;

  auto find_ret = _hash_table.find(the_hash_key);
  if(find_ret == _hash_table.end()) {
    // Not found, add
    hash_join_row_t tmp;
    tmp.rows = {row};
    tmp.matched = false;
    std::list<value_type> tmp_list;
    tmp_list.push_back(std::move(tmp));
    _hash_table.insert(std::make_pair(the_hash_key, tmp_list));
    first_entry = true;
  } else {
    // Get an iterator to the list, loop to find join-key match
    ASSERT_GT(find_ret->second.size(), 0);
    bool matched = false;
    auto it = find_ret->second.begin();
    while(it != find_ret->second.end()) {
      ASSERT_GT(it->rows.size(), 0);
      if(join_values_equal(it->rows[0], row, _hash_positions)) {
        it->rows.push_back(row);
        matched = true;
        first_entry = false;
        break;
      }
      ++it;
    }

    // If we get here, this means the join key hashed to the same value as a
    // different join key
    if(!matched) {
      hash_join_row_t tmp;
      tmp.rows = {row};
      tmp.matched = false;
      find_ret->second.push_back(std::move(tmp));
      first_entry = true;
    }
  }

  return first_entry;
}

const join_hash_table::value_type& join_hash_table::get_matching_rows(
    const std::vector<flexible_type> &row,
    const std::vector<size_t> &hash_positions,
    bool mark_match) {

  size_t the_hash_key = compute_hash_from_row(row, hash_positions);
  auto ret = _hash_table.find(the_hash_key);

  if(ret == _hash_table.end()) {
    // Return empty hash_join_key_t if nothing is found
    return empty_vt;
  } else {
    // There's a hit on the hash! See if it is an actual match.
    auto it = ret->second.begin();
    while(it != ret->second.end()) {
      if(join_values_equal(it->rows[0], row, hash_positions)) {
        if(mark_match) {
          it->matched = true;
        }
        return *it;
      }
      ++it;
    }

    // If we're here, we didn't find an actual match
    return empty_vt;
  }
}

bool join_hash_table::join_values_equal(const std::vector<flexible_type> &row,
                                        const std::vector<flexible_type> &other,
                                        const std::vector<size_t> &hash_positions) {
  if(hash_positions.size() == 0) {
    if(row.size() == 0 && other.size() == 0) {
      return true;
    } else {
      return false;
    }
  }

  ASSERT_EQ(_hash_positions.size(), hash_positions.size());

  for(size_t i = 0; i < hash_positions.size(); ++i) {
    if(row[_hash_positions[i]] != other[hash_positions[i]]) {
      return false;
    }
  }

  return true;
}

size_t join_hash_table::num_stored_rows() {
  size_t num_rows = 0;
  size_t num_unique_join_values = 0;
  logstream(LOG_INFO) << "Number of hash values: " << _hash_table.size() << std::endl;
  for(auto it = _hash_table.begin(); it != _hash_table.end(); ++it) {
    for(auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
      num_unique_join_values++;
      num_rows += it2->rows.size();
    }
  }
  logstream(LOG_INFO) << "Number of unique join values: " << num_unique_join_values << std::endl;
  logstream(LOG_INFO) << "Number of stored rows: " << num_rows << std::endl;

  return num_rows;
}

std::unordered_map<size_t, std::list<join_hash_table::value_type>>::const_iterator join_hash_table::cbegin() {
  return _hash_table.cbegin();
}

std::unordered_map<size_t, std::list<join_hash_table::value_type>>::const_iterator join_hash_table::cend() {
  return _hash_table.cend();
}

hash_join_executor::hash_join_executor(const sframe &left,
                                       const sframe &right,
                                       const std::vector<size_t> &left_join_positions,
                                       const std::vector<size_t> &right_join_positions,
                                       join_type_t join_type,
                                       const std::map<std::string,std::string>& alter_names_right,
                                       size_t max_buffer_size) :
    _left_frame(left),
    _right_frame(right),
    _left_join_positions(left_join_positions),
    _right_join_positions(right_join_positions),
    _max_buffer_size(max_buffer_size),
    _left_join(false),
    _right_join(false),
    _reverse_output_column_order(false),
    _frames_partitioned(false),
    _alter_names_right(alter_names_right) {


  if(join_type == LEFT_JOIN || join_type == FULL_JOIN) {
    _left_join = true;
  }
  if(join_type == RIGHT_JOIN || join_type == FULL_JOIN) {
    _right_join = true;
  }

  ASSERT_EQ(_left_join_positions.size(), _right_join_positions.size());

  /* handy script for reverse lookup */
  for(size_t i = 0; i < _left_join_positions.size(); ++i) {
    auto ret = _right_to_left_join_positions.emplace(_right_join_positions[i],
                                                     _left_join_positions[i]);
    ASSERT_TRUE(ret.second);

    ret = _left_to_right_join_positions.emplace(_left_join_positions[i],
                                                _right_join_positions[i]);
    ASSERT_TRUE(ret.second);
  }

  /*
   * using names from sframe: unique and non-empty
   * the new names in result sframe should have no confilict
   *
   * this is order dependent; doing in reverse will result in
   * different result.
   *
   * since _alter_name_right only applies to original order,
   * we have to obtain the original construction
   **/

  // get orginal left sframe's column names
  auto col_names_original_order = left.column_names();
  _output_column_names = left.column_names();
  _output_column_types = left.column_types();

  std::set<std::string> new_table = {
      std::make_move_iterator(col_names_original_order.begin()),
      std::make_move_iterator(col_names_original_order.end())};

  // get orginal right sframe's column names
  col_names_original_order = right.column_names();

  /* check the name resolution is order dependent or not.
   * resolution {A -> B} is order dependent iff
   * 1. A belongs to right.column_names()
   * 2. B is not equal to keys from right.column_names()
   * 3. B should be unique to other B'
  */
  std::set<std::string> unique_names_from_right = {
      std::begin(col_names_original_order), std::end(col_names_original_order)};

  std::set<std::string> unique_names_from_user;
  for (const auto& entry : _alter_names_right) {
    if (unique_names_from_right.count(entry.first) == 0) {
          std::stringstream ss;
          ss << "user provided column name { " << entry.first
             << " } is not found in right SFrame.";
          log_and_throw(ss.str());
    }

    if (!unique_names_from_user.insert(entry.second).second) {
          std::stringstream ss;
          ss << "user provided resolution name { " << entry.second
             << " } duplicates with other resolution name.";
          log_and_throw(ss.str());
    }

    if (unique_names_from_right.count(entry.second)) {
          std::stringstream ss;
          ss << "user provided resolution name { " << entry.second
             << " } is not allowed to be same with any name in right SFrame.";
          log_and_throw(ss.str());
    }
  }

  // check the original right sframe columns
  for (auto& name : col_names_original_order) {
    // skip join_keys
    if (_right_to_left_join_positions.count(right.column_index(name)))
      continue;

    if (new_table.count(name)) {
      if (_alter_names_right.count(name)) {
        /*
         * user defined resolution shall not have any conflict with
         * all col names visited so far; but can be the same to col
         * names haven't seen so far.
         **/
        auto itr = _alter_names_right.find(name);
        if (!new_table.insert(itr->second).second) {
          std::stringstream ss;
          ss << "user provided column name { " << itr->second << " } conflicts with table name used in SFrame";
          log_and_throw(ss.str());
        }
        _output_column_types.push_back(right.column_type(right.column_index(name)));
        _output_column_names.push_back(itr->second);
        new_table.insert(itr->second);
      } else {
        _output_column_types.push_back(right.column_type(right.column_index(name)));
        // default collision resolv. see SFrame::generate_valid_column_name.
        // if SFrame::generate_valid_column_name changes, this will break.
        name.append(".1", 2);
        _output_column_names.push_back(name);
        new_table.insert(std::move(name));
      }
    } else {
      _output_column_names.push_back(name);
      _output_column_types.push_back(right.column_type(right.column_index(name)));
      new_table.insert(std::move(name));
    }
  }

  // Left should always be smaller than right
  if (get_num_cells(right) < get_num_cells(left)) {
    _reverse_output_column_order = true;
    std::swap(_left_frame, _right_frame);
    std::swap(_left_join_positions, _right_join_positions);
    std::swap(_right_to_left_join_positions, _left_to_right_join_positions);
    if(_left_join != _right_join) {
      _left_join = !_left_join;
      _right_join = !_right_join;
    }
  }
}

void hash_join_executor::init_result_frame(sframe &result_frame) {
  // Check which of the right frame's column names changed
  if (_reverse_output_column_order) {
    std::vector<std::string> res_column_names(_output_column_names.size());
    std::vector<flex_type_enum> res_column_types(_output_column_types.size());

    const auto& original_left_to_right = _right_to_left_join_positions;

    /* put the join keys into the first part  */
    for (size_t ii = 0; ii < _right_frame.num_columns(); ii++) {
      auto itr = original_left_to_right.find(ii);
      if (itr != original_left_to_right.end()) {
        res_column_names[itr->second] = _output_column_names[itr->first];
        res_column_types[itr->second] = _output_column_types[itr->first];
        _reverse_to_original.emplace(itr->second, itr->first);
      }
    }

    /* put user provide right part to left part of reverse ordered vector */
    size_t ii = 0;
    /* _right_frame is the user defined left_frame */
    for (size_t col_idx = _right_frame.num_columns(); col_idx < _output_column_names.size(); col_idx++) {
      /* skip join_keys */
      while (!res_column_names[ii].empty()) {
        ii++;
      }
      res_column_names[ii] = _output_column_names[col_idx];
      res_column_types[ii] = _output_column_types[col_idx];
      _reverse_to_original.emplace(ii, col_idx);
    }

    /* put the rest from the user defined left frame */
    for (size_t col_idx = 0U; col_idx < _right_frame.num_columns(); col_idx++) {
      /* skip join_keys */
      if (!original_left_to_right.count(col_idx)) {
        while (!res_column_names[ii].empty()) {
          ii++;
        }
        res_column_names[ii] = _output_column_names[col_idx];
        res_column_types[ii] = _output_column_types[col_idx];
        _reverse_to_original.emplace(ii, col_idx);
      }
    }
    DASSERT_EQ(_reverse_to_original.size(), _output_column_names.size());

    std::set<std::string> unique_names(std::begin(res_column_names),
                                       std::end(res_column_names));
    DASSERT_EQ(unique_names.size(), res_column_names.size());

    size_t num_segments = std::max(
        {_left_frame.num_segments(), _right_frame.num_segments(),
         thread::cpu_count() * std::max<size_t>(1, log2(thread::cpu_count()))});
    // Will throw if the SFrame is not in the state we expect
    result_frame.open_for_write(res_column_names, res_column_types, "",
                                num_segments, false);

  } else {
    size_t num_segments = std::max(
        {_left_frame.num_segments(), _right_frame.num_segments(),
         thread::cpu_count() * std::max<size_t>(1, log2(thread::cpu_count()))});
    // Will throw if the SFrame is not in the state we expect
    result_frame.open_for_write(_output_column_names, _output_column_types, "",
                                num_segments, false);
  }
}

std::vector<flexible_type> hash_join_executor::unpack_row(
    std::string val, size_t num_cols) {
  iarchive iarc(val.c_str(), val.length());
  std::vector<flexible_type> row(num_cols);
  for(auto row_it = row.begin(); row_it != row.end(); ++row_it) {
    iarc >> *row_it;
  }

  return row;
}

sframe hash_join_executor::grace_hash_join() {
  sframe result_frame;

  std::shared_ptr<sframe> grace_left;
  std::shared_ptr<sframe> grace_right;
  timer full_ti;
  timer ti;
  std::tie(grace_left, grace_right) = this->grace_partition_frames();
  logstream(LOG_INFO) << "Partitioned frames in: " << ti.current_time() << std::endl;
  this->init_result_frame(result_frame);
  ASSERT_EQ(grace_left->size(), _left_frame.size());
  ASSERT_EQ(grace_right->size(), _right_frame.size());

  size_t num_segments;
  std::vector<size_t> right_segment_lengths;
  if(_frames_partitioned) {
    num_segments = grace_left->num_segments();
    // After partitioning this needs to be true
    ASSERT_EQ(num_segments, grace_right->num_segments());
    for(size_t i = 0; i < num_segments; ++i) {
      right_segment_lengths.push_back(grace_right->segment_length(i));
    }
  } else {
    num_segments = 1;
    right_segment_lengths.push_back(grace_right->num_rows());
  }

  // Instantiate all output iterators
  std::vector<sframe::iterator> result_output_iterators(result_frame.num_segments());
  for(size_t i = 0; i < result_frame.num_segments(); ++i) {
    result_output_iterators[i] = result_frame.get_output_iterator(i);
  }

  // Split each segment of the right frame into num_segments (of output frame) pieces
  // This is so I can parallelize the hash table lookups when scanning the right
  // frame.  There is one thread per output segment, and each SFrame segment
  // must be split up into this many pieces to parallelize.
  std::vector<size_t> logical_right_segment_sizes;
  for(size_t i = 0; i < num_segments; ++i) {

    size_t elements_left = right_segment_lengths[i];
    size_t elements_per = elements_left / result_frame.num_segments();
    size_t first_of_segment_idx = i*result_frame.num_segments();

    for(size_t j = 0; j < result_frame.num_segments(); ++j) {
      if(elements_per <= elements_left) {
        logical_right_segment_sizes.push_back(elements_per);
        elements_left -= elements_per;
      } else {
        // This makes sure we don't have a bunch of segments with a tiny
        // amount of elements.  Better to have it all in one segment.
        if(elements_left > MIN_SEGMENT_LENGTH) {
          logical_right_segment_sizes.push_back(elements_left);
          elements_left = 0;
        } else {
          logical_right_segment_sizes.push_back(0);
        }
      }
    }
    // Put leftovers in the first segment
    if(elements_left > 0) {
      logical_right_segment_sizes[first_of_segment_idx] += elements_left;
    }
  }
  ASSERT_EQ(logical_right_segment_sizes.size(),
            num_segments*result_frame.num_segments());

  // Readers for the left and right SArray used in the join
  std::unique_ptr<sframe::reader_type> l_rdr;
  if(_frames_partitioned) {
    l_rdr = grace_left->get_reader();
  } else {
    l_rdr = grace_left->get_reader(num_segments);
  }
  auto r_rdr = grace_right->get_reader(logical_right_segment_sizes);

  // Iterate over each segment of the left frame and add to a hash table.
  // These segments can not be read in parallel because they are
  // meant to represent the upper bound of the memory we can read in.
  ti.start();
  for(size_t i = 0; i < num_segments; ++i) {
    // Load the entire left partition into a hash table
    join_hash_table cur_ht(_left_join_positions);
    for(auto iter = l_rdr->begin(i); iter != l_rdr->end(i); ++iter) {
      // Must unpack the row data from the serialized string it is stored as
      std::vector<flexible_type> row;
      if(_frames_partitioned) {
        row = unpack_row(std::string(iter->at(0)), _left_frame.num_columns());
      } else {
        row = *iter;
      }
      cur_ht.add_row(row);
    }

    parallel_for(0, result_frame.num_segments(),
        [&](size_t seg_num) {
          size_t cur_logical_segment = i*result_frame.num_segments()+seg_num;
          auto writer = result_output_iterators[seg_num];

          // Iterate through the logical segment of the current segment
          for(auto iter = r_rdr->begin(cur_logical_segment);
              iter != r_rdr->end(cur_logical_segment);
              ++iter) {

            // Must unpack the row data from the serialized string it is stored as
            std::vector<flexible_type> row;
            if(_frames_partitioned) {
              row = unpack_row(std::string(iter->at(0)), _right_frame.num_columns());
            } else {
              row = *iter;
            }

            // Merge any matching rows to the corresponding left row and write
            auto query_result = cur_ht.get_matching_rows(row, _right_join_positions);

            // If our matching rows query returned something, then this result
            // should be in the inner join.  If it didn't, this row should only
            // be in a right join
            if((query_result.rows.size() > 0) ||
              ((query_result.rows.size() == 0) && _right_join)) {
              // Match found! Add to the result set
              merge_rows_for_output(result_frame, writer, query_result.rows, {row});
            }
          }
        });

    // Get an output iterator for a segment...try not to overload one segment
    size_t seg_cntr = 0;
    if(_left_join) {
      for(auto iter = cur_ht.cbegin(); iter != cur_ht.cend(); ++iter) {
        for(auto iter2 = iter->second.begin(); iter2 != iter->second.end(); ++iter2) {
          if(!iter2->matched) {
            auto result_writer =
              result_output_iterators[seg_cntr % result_frame.num_segments()];
            merge_rows_for_output(result_frame,
                result_writer,
                {iter2->rows},
                std::vector<std::vector<flexible_type>>());
          }
        }
      }
    }
  }
  logstream(LOG_INFO) << "Hash join time: " << ti.current_time() << std::endl;

  result_frame.close();
  logstream(LOG_INFO) << "Full join time: " << full_ti.current_time() << std::endl;

  // If we swapped the join order for performance reasons, we need to make the
  // columns appear in the order the user was expecting.  This code does this.
  if(_reverse_output_column_order) {
    std::vector<std::shared_ptr<sarray<flexible_type>>> swapped_columns(result_frame.num_columns());
    std::vector<std::string> swapped_names(result_frame.num_columns());

    for (const auto& entry : _reverse_to_original) {
      swapped_columns[entry.second] = result_frame.select_column(entry.first);
      swapped_names[entry.second] = result_frame.column_name(entry.first);
    }

    sframe swapped_result_frame(swapped_columns, swapped_names, false);
    return swapped_result_frame;
  }

  return result_frame;
}

void hash_join_executor::merge_rows_for_output(sframe &result_frame,
                                               sframe::iterator result_iter,
                                               const std::vector<std::vector<flexible_type>> &left_rows,
                                               const std::vector<std::vector<flexible_type>> &right_rows) {
  // Size of cross product of left and right rows
  size_t num_emitted_rows = left_rows.size() * right_rows.size();
  if(num_emitted_rows == 0) {
    if(left_rows.size() == 0 && right_rows.size() == 0) {
      return;
    } else {
      // For special case of one empty vector
      num_emitted_rows = std::max(left_rows.size(), right_rows.size());
    }
  }

  // Initialize the values as missing (or NULL)
  std::vector<std::vector<flexible_type>> rows_to_emit
    (num_emitted_rows, std::vector<flexible_type>(result_frame.num_columns(),
                                                  flex_undefined()));

  // The result from these two separate loops are basically what a nested for
  // loop would've output if both vectors are non-empty.  The advantage of
  // coding it this way is that if the rows_to_emit structure is initialized
  // with NULLs, a left-join or right-join can be performed if one of the
  // passed in vectors is empty.
  if(left_rows.size()) {
    // To acheive a cross product of left and right, we must repeat the left
    // rows this many times
    size_t left_repeats = num_emitted_rows / left_rows.size();
    size_t i = 0;
    for(auto l_iter = left_rows.begin();
        l_iter != left_rows.end();
        ++l_iter, ++i) {
      for(size_t j = 0; j < left_repeats; ++j) {
        std::copy(l_iter->begin(), l_iter->end(), rows_to_emit[i].begin());
      }
    }
    ASSERT_EQ(i, rows_to_emit.size());
  }

  if(right_rows.size()) {
    size_t right_repeats = num_emitted_rows / right_rows.size();
    ASSERT_GE(right_rows[0].size(), _right_join_positions.size());

    // This is the number of values in the output frame that appear from
    // columns in the right frame.
    size_t num_values = right_rows[0].size() - _right_join_positions.size();

    size_t row_cntr = 0;
    for(size_t i = 0; i < right_repeats; ++i) {
      for(auto r_iter = right_rows.begin();
          r_iter != right_rows.end();
          ++r_iter, ++row_cntr) {

        // Where we are in the current row to emit
        auto row_iter = rows_to_emit[row_cntr].end()-num_values;

        // Iterate through each value of this row
        for(size_t j = 0; j < r_iter->size(); ++j) {
          // This isn't a join position, copy into result
          auto find_ret = _right_to_left_join_positions.find(j);
          if(find_ret == _right_to_left_join_positions.end()) {
            *row_iter = (*r_iter)[j];
            ++row_iter;
          } else {
            // Special case for right join...we want this data from the right
            if(!left_rows.size()) {
              rows_to_emit[row_cntr][find_ret->second] = (*r_iter)[j];
            }
          }
        }
      }
    }

    ASSERT_EQ(row_cntr, rows_to_emit.size());
  }

  // Emit our rows to the iterator!
  for(auto iter = rows_to_emit.begin(); iter != rows_to_emit.end(); ++iter) {
    *result_iter = *iter;
  }
}

size_t hash_join_executor::get_num_cells(const sframe &sf) {
  return (sf.num_rows() * sf.num_columns());
}

size_t hash_join_executor::choose_number_of_grace_partitions(const sframe &sf) {
  size_t num_cells = get_num_cells(sf);
  return (num_cells / _max_buffer_size) + 1;
}


std::pair<std::shared_ptr<sframe>,std::shared_ptr<sframe>> hash_join_executor::grace_partition_frames() {
  // Pick # of partitions
  // TODO: Add estimated disk and memory size to SFrames.
  // This way we can check when to do GRACE recursively
  size_t left_partitions = choose_number_of_grace_partitions(_left_frame);
  size_t right_partitions = choose_number_of_grace_partitions(_right_frame);
  size_t num_partitions = std::min(left_partitions, right_partitions);

  logstream(LOG_INFO) << "Chose " << num_partitions <<
    " partitions for GRACE hash join\n";

  // Hash join columns into separate partitions
  // (each partition is a segment of an SFrame)
  auto parted_left_frame = grace_partition_frame(_left_frame, _left_join_positions, num_partitions);
  auto parted_right_frame = grace_partition_frame(_right_frame, _right_join_positions, num_partitions);

  return std::make_pair(parted_left_frame, parted_right_frame);
}

std::shared_ptr<sframe> hash_join_executor::grace_partition_frame(
    const sframe &sf,
    const std::vector<size_t> &join_col_nums,
    size_t num_partitions) {
  //TODO: for now
  log_func_entry();
  // We don't need to partition if only 1 is needed
  if(num_partitions == 1) {
    return std::make_shared<sframe>(sf);
  } else if(num_partitions < 1) {
    log_and_throw("Cannot make < 1 partitions!");
  }

  // Open the partitioned sframe
  auto parted_array = std::make_shared<sframe>();
  parted_array->open_for_write({"data"},{flex_type_enum::STRING},"",num_partitions);

  // Get all iterators
  // Create a mutex for each iterator
  std::vector<sframe::iterator> outiter_vector(num_partitions);
  std::vector<mutex> outiter_mutexes(num_partitions);
  for(size_t i = 0; i < num_partitions; ++i) {
    outiter_vector[i] = parted_array->get_output_iterator(i);
  }

  // Iterate over each row of the given SFrame, hash on the join columns,
  // and write that row to the appropriate segment of the partitioned sframe
  auto rdr = sf.get_reader(thread::cpu_count());
  parallel_for(0, rdr->num_segments(), [&](size_t seg_num) {
    oarchive oarc;
    for(auto j = rdr->begin(seg_num); j != rdr->end(seg_num); ++j) {
      // Hash the given columns
      size_t hash_val = compute_hash_from_row(*j, join_col_nums);
      size_t which_partition = hash_val % num_partitions;

      // Serialize the row
      for(auto &k : *j) {
        oarc << k;
      }
      std::string s = std::string(oarc.buf, oarc.off);
      flexible_type f(std::move(s));
      outiter_mutexes[which_partition].lock();
      *(outiter_vector[which_partition]) = std::vector<flexible_type>{std::move(f)};
      ++outiter_vector[which_partition];
      outiter_mutexes[which_partition].unlock();
      oarc.off = 0;
    }
    free(oarc.buf);
  });

  // We're done writing. Close all output iterators.
  parted_array->close();

  _frames_partitioned = true;

  return parted_array;
}

size_t compute_hash_from_row(const std::vector<flexible_type> &row,
                             const std::vector<size_t> &positions) {
  size_t ret = 0;
  for(auto i : positions) {
    ret = hash64_combine(ret, row[i].hash());
  }

  return ret;
}

size_t hash_key(const std::vector<flexible_type>& key) {
  size_t ret = 0;
  for (size_t i = 0;i < key.size(); ++i) {
    ret = hash64_combine(ret, key[i].hash());
  }
  return ret;
}

} // end of join_impl
} // end of turicreate
