/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sframe_compact.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_index_file.hpp>
#include <core/storage/sframe_data/sarray_index_file.hpp>
#include <core/storage/sframe_data/sarray_v2_block_manager.hpp>
#include <core/storage/sframe_data/sarray_v2_block_writer.hpp>
#include <core/storage/sframe_data/sarray_v2_block_types.hpp>
#include <core/storage/sframe_data/sframe_saving_impl.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/logging/assertions.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
namespace turi {
using namespace sframe_saving_impl;
/**
 * There are many strategies for saving an SFrame, and the objective here
 * is to pick the right strategy for the right occassion, falling back to
 * the naive strategies when we run out of options.
 */

/**
 * This is the most naive of all saving strategies.
 *
 */
void sframe_save_naive(const sframe& sf_source,
                       std::string index_file) {
  std::vector<std::string> my_names;
  std::vector<flex_type_enum> my_types;
  for(size_t i = 0; i < sf_source.num_columns(); ++i) {
    my_names.push_back(sf_source.column_name(i));
    my_types.push_back(sf_source.column_type(i));
  }

  // target sframe.
  sframe new_sf;
  size_t num_write_segments = SFRAME_DEFAULT_NUM_SEGMENTS;
  if (sf_source.num_segments() == 0) num_write_segments = 0;

  new_sf.open_for_write(my_names, my_types,
                        index_file, SFRAME_DEFAULT_NUM_SEGMENTS);
  if (sf_source.num_segments() == 0) {
    new_sf.close();
    return;
  }

  size_t step = sf_source.num_rows() / num_write_segments;
  auto reader = sf_source.get_reader();
  parallel_for(0, num_write_segments, [&](size_t i) {
    auto out = new_sf.get_output_iterator(i);
    size_t start = i * step;
    size_t end = (i + 1) * step;
    if (i == num_write_segments - 1) end = sf_source.num_rows();
    sframe_rows rows;
    while(start < end) {
      size_t limit = end - start;
      if (limit > sframe_config::SFRAME_READ_BATCH_SIZE) {
        limit = sframe_config::SFRAME_READ_BATCH_SIZE;
      }
      reader->read_rows(start, start + limit, rows);
      (*out) = rows;
      ++out;
      start += limit;
    }
  });
  new_sf.close();
}




void sframe_save_blockwise(const sframe& sf_source,
                           std::string index_file) {
  // this will hit the sframe at a lower level
  // This is slightly complicated and slightly annoying.
  //
  // SFrame:
  // An SFrame is an arbitrary collection of columns listed in a
  // sframe_index_file_information datastructure.
  // Each column is denoted by a column file (it may not be a real file)
  //
  // Column file: This may not be a real file. Each column is made up of
  // a collection of segments.
  //
  // Segments : Each segment is made up of a collection of blocks.
  //
  //
  // Essentially, we are going to maintain a collection of columns and advance
  // each column through their segments and blocks.
  //
  // Now, the input number of segments may not match the output number of
  // segments and so we probably need to do some shuffling around here.
  // Also, while we are here, it might be nice to reorder the blocks a bit

  // initialize reader and writer
  auto& block_manager = v2_block_impl::block_manager::get_instance();
  v2_block_impl::block_writer writer;

  // call it indexfile.0000
  std::string base_name;
  size_t last_dot = index_file.find_last_of(".");
  if (last_dot != std::string::npos) {
    base_name = index_file.substr(0, last_dot);
  } else {
    base_name = index_file;
  }
  auto index = base_name + ".sidx";
  auto segment_file = base_name + ".0000";
  // we are going to emit only 1 segment. We should be rather IO bound anyway
  writer.init(index, 1, sf_source.num_columns());
  writer.open_segment(0, segment_file);


  // this is going to be a max heap with each entry referencing a column.
  std::vector<column_blocks> cols;
  try {
    for (size_t i = 0;i < sf_source.num_columns(); ++i) {
      column_blocks col;
      auto cur_column = sf_source.select_column(i);
      col.column_index = cur_column->get_index_info();
      if (col.column_index.segment_files.size() > 0) {
        col.segment_address =
            block_manager.open_column(col.column_index.segment_files[0]);
        // the block address is basically a tuple beginning with
        // the column address
        col.num_blocks_in_current_segment =
            block_manager.num_blocks_in_column(col.segment_address);
        col.next_row = 0;
        col.column_number = i;
        col.num_segments = col.column_index.segment_files.size();
        if (col.current_block_number >= col.num_blocks_in_current_segment) {
          advance_column_blocks_to_next_block(block_manager, col);
        }
        if (!col.eof) cols.push_back(col);
      }

      writer.get_index_info().columns[i].metadata = col.column_index.metadata;
    }
    // we are going to reorder the blocks so that the column with the lowest
    // row number get written first. So this is to be a min-heap
    auto comparator = [](
        const column_blocks& a,
        const column_blocks& b)->bool {
      return a.next_row > b.next_row;
    };
    std::make_heap(cols.begin(), cols.end(), comparator);

    while(!cols.empty()) {
      auto cur = *(cols.begin());
      std::pop_heap(cols.begin(), cols.end(), comparator);
      cols.pop_back();
      // read a block
      v2_block_impl::block_info* infoptr = nullptr;
      v2_block_impl::block_info info;
      v2_block_impl::block_address block_address
                          {std::get<0>(cur.segment_address),
                           std::get<1>(cur.segment_address),
                           cur.current_block_number};
      auto data = block_manager.read_block(block_address , &infoptr);
      info = *infoptr;
      // write to segment 0. We have only 1 segment
      writer.write_block(0, cur.column_number, data->data(), info);
      // increment the block number
      advance_column_blocks_to_next_block(block_manager, cur);
      // increment the row number
      cur.next_row += info.num_elem;
      // if there are still blocks. push it back
      if (!cur.eof) {
        cols.push_back(cur);
        std::push_heap(cols.begin(), cols.end(), comparator);
      }
    };

    // close writers.
    writer.close_segment(0);
    writer.write_index_file();
    auto output_index = writer.get_index_info();

    // ok. now we need to write the actual frame index file
    // get the original frame index
    // and fill in the column data from the writer output
    auto frame_index = sf_source.get_index_info();
    frame_index.column_files.clear();
    for (auto col : output_index.columns) {
      frame_index.column_files.push_back(col.index_file);
    }
    write_sframe_index_file(index_file, frame_index);
  } catch (...) {
    // cleanup. close any open columns
    for(auto& col: cols) {
      try {
        block_manager.close_column(col.segment_address);
      } catch (...) { }
    }
    throw;
  }
}

void sframe_save(const sframe& sf_source,
                 std::string index_file) {
  // if there are any columns on sarray v1 format, we use the naive form
  bool has_legacy_sframe = false;
  for (size_t i = 0;i < sf_source.num_columns(); ++i) {
    auto cur_column = sf_source.select_column(i);
    if (cur_column->get_index_info().version < 2) has_legacy_sframe = true;
  }

  if (has_legacy_sframe) {
    sframe_save_naive(sf_source, index_file);
  } else {
    sframe_fast_compact(sf_source);
    sframe_save_blockwise(sf_source, index_file);
  }
}


void sframe_save_weak_reference(const sframe& sf_source,
                                std::string index_file) {
  /*
   * The algorithm is not very complicated, but is annoying because
   * once again we have to touch the sframe at a lower level than I would like.
   *
   * What are are going to do is to build a list of all the segment files
   * that make up the sframe, and:
   *  - If they are on the same target protocol, (like hdfs:// s3://)
   *    Action: keep and do nothing.
   *  - if they are on a different target protocol
   *    Action: We need to relocate the segment.
   *    This is a little annoying.
   *
   * The difficulty with relocating segments is that
   * the many-to-many relationship between columns in an SFrame,
   * and columns in a segment makes things slightly annoying.
   *  - There may be a lot more columns in the segment than we need.
   *  - A column in an SFrame are subcolumns of many segments. Some of which
   *  may be already on the right protocol, some may not.
   *
   * So, we are not going to to this. We are going to do something simpler
   * Instead, we are going to implement a slightly simpler version.
   *
   * We look at the sframe one entire column at a time.
   *  - If the entire column is already on the right protocol, Do nothing.
   *  - Else If only part or none of the column is on the right protocol,
   *  we add the column to a temporary SFrame.
   *
   * Finally,
   * If the temporary SFrame is non-empty, we use the full sframe_save to
   * save it to the target in temp directory provided.
   * Now, everything we need should be on the target protocol, and we rebuild
   * the index files.
   */
  std::string base_name;
  size_t last_dot = index_file.find_last_of(".");
  if (last_dot != std::string::npos) {
    base_name = index_file.substr(0, last_dot);
  } else {
    base_name = index_file;
  }

  std::string output_protocol = fileio::get_protocol(index_file);

  // column_segment_to_be_relocated[column_number of sframe][segment_number]
  // is true if the segment is to be relocated
  std::vector<std::vector<bool> > column_segment_to_be_relocated;
  // column_was_relocated[i] is true if column i was relocated
  std::vector<bool> column_was_relocated;
  // column_index_files[i] point to the location of the sidx for the column
  std::vector<std::string> column_index_files;

  size_t num_columns = sf_source.num_columns();
  column_segment_to_be_relocated.resize(num_columns);
  column_was_relocated.resize(num_columns, false);
  column_index_files.resize(num_columns);
  auto uuid_generator = boost::uuids::random_generator();
  // go through all the columns collect the metadata I need (fill in the
  // structures above)
  {
    auto sf_source_index_info = sf_source.get_index_info();
    for (size_t i = 0;i < sf_source.num_columns(); ++i) {
      // fill in the metadata for this column
      auto column_index = sf_source.select_column(i)->get_index_info();
      column_index_files[i] = column_index.index_file;
      column_segment_to_be_relocated[i].resize(column_index.segment_files.size());
      for (size_t j = 0;j < column_index.segment_files.size(); ++j) {
        auto input_protocol = fileio::get_protocol(column_index.segment_files[j]);
        column_segment_to_be_relocated[i][j] = input_protocol != output_protocol;
      }
    }
  }
  // done!
  // now to perform all relocations
  sframe temp_sf;
  for (size_t i = 0;i < num_columns; ++i) {
    bool to_relocate = std::any_of(column_segment_to_be_relocated[i].begin(),
                                   column_segment_to_be_relocated[i].end(),
                                   [](const bool& r) { return r; });
    column_was_relocated[i] = to_relocate;
    if (to_relocate) {
      temp_sf = temp_sf.add_column(sf_source.select_column(i),
                                   sf_source.column_name(i));
    } else {
      // we prefer the column to hang around after termination.
      // if they were marked for deletion (for instance, overwriting an
      // existing dir archive, unmark them.
      if (!column_index_files[i].empty()) {
        auto index = parse_v2_segment_filename(column_index_files[i]).first;
        fileio::file_handle_pool::get_instance().
            unmark_file_for_delete(index);
      }
      auto column_index = sf_source.select_column(i)->get_index_info();
      for (size_t j = 0;j < column_index.segment_files.size(); ++j) {
        auto segment_file = parse_v2_segment_filename(column_index.segment_files[j]).first;
        fileio::file_handle_pool::get_instance().
            unmark_file_for_delete(segment_file);
      }

      // convert to a group index of 1 column
      group_index_file_information group_index;
      group_index.version = 2;
      group_index.nsegments = column_index.segment_files.size();
      group_index.segment_files = column_index.segment_files;

      group_index.columns.push_back(column_index);
      std::string group_index_filename = base_name + "-column-" + std::to_string(i) + ".sidx";
      write_array_group_index_file(group_index_filename, group_index);
      column_index_files[i] = group_index_filename + ":0";
    }
  }
  if (temp_sf.num_columns() > 0) {
    auto suffix = boost::lexical_cast<std::string>(uuid_generator());
    std::string temp_sf_output_index = base_name + "-" +  suffix + ".frame_idx";
    sframe_save(temp_sf, temp_sf_output_index);
    // reload it. so we get the new segment information.
    temp_sf = sframe(temp_sf_output_index);
  }

  // ok. so temp_sf contains all the columns that were relocated.
  // we need to get their segment information
  {
    size_t column_ctr = 0;
    auto temp_sf_index_info = temp_sf.get_index_info();
    for (size_t i = 0;i < num_columns; ++i) {
      if (column_was_relocated[i]) {
        auto cur_column = temp_sf.select_column(column_ctr);
        auto column_index = cur_column->get_index_info();
        column_index_files[i] = column_index.index_file;
        ++column_ctr;
      }
    }
  }

  //  relocation complete. column_index_filesnow contains everything I care about
  //  on the target protocol.  now we can generate the frame index.
  //
  // The convenience of handling it column by column is that we have all the
  // array "sidx" files in place. We just need to build a frame index that
  // references them.
  // we get the root sframe's frame index rewrite it and save it

  // for every sidx we move to the target protocol, we keep a map just in case
  // it is reused. (remember! there is no 1-1 mapping between columns and
  // arrays)
  std::map<std::string, std::string> target;
  for (size_t i = 0;i < num_columns; ++i) {
    std::pair<std::string, size_t> column_index_and_subcolumn =
        parse_v2_segment_filename(column_index_files[i]);
    if (column_index_and_subcolumn.second == (size_t)(-1)) {
      column_index_and_subcolumn.second = 0;
    }
    if (fileio::get_protocol(column_index_files[i]) != output_protocol) {
      // save it to the target location
      if (target.count(column_index_and_subcolumn.first) == 0) {
        // need to copy it. Make another uuid
        auto suffix = boost::lexical_cast<std::string>(uuid_generator());
        std::string temp_sidx_output_index = base_name + "-" + suffix + ".sidx";
        fileio::copy(column_index_and_subcolumn.first, temp_sidx_output_index);
        target[column_index_and_subcolumn.first] = temp_sidx_output_index;
      }
    } else {
      target[column_index_and_subcolumn.first] = column_index_and_subcolumn.first;
    }

    column_index_files[i] = target[column_index_and_subcolumn.first] + ":" +
        boost::lexical_cast<std::string>(column_index_and_subcolumn.second);
  }

  // finally save the frame index
  auto new_frame_index_info = sf_source.get_index_info();
  for (size_t i = 0;i < column_index_files.size(); ++i) {
    new_frame_index_info.column_files[i] = column_index_files[i];
  }
  write_sframe_index_file(index_file, new_frame_index_info);

}

}
