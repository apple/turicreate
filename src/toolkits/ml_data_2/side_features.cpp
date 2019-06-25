/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <toolkits/ml_data_2/side_features.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_row_translation.hpp>
#include <core/parallel/atomic.hpp>
#include <model_server/lib/flex_dict_view.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <core/util/try_finally.hpp>

using namespace turi::v2::ml_data_internal;

namespace turi { namespace v2 {


ml_data_side_features::ml_data_side_features(
    const std::vector<ml_data_internal::column_metadata_ptr>& _main_metadata)

    : main_metadata(_main_metadata)
    , side_lookups(main_metadata.size())
    , current_column_index(main_metadata.size())
    , _full_metadata(main_metadata)
{
  // Construct a map of column names to column metadata
  for(size_t i = 0; i < main_metadata.size(); ++i)
    main_column_name_lookup[main_metadata[i]->name] = i;
}




////////////////////////////////////////////////////////////////////////////////

void ml_data_side_features::add_and_index_side_data(
    sframe unindexed_side_sframe,
    const std::map<std::string, ml_column_mode>& mode_overrides,
    const std::map<std::string, flexible_type>& options,
    bool training_mode,
    bool immutable_metadata,
    const std::string& forced_join_column) {

  if(unindexed_side_sframe.num_columns() == 0
     || (!training_mode && unindexed_side_sframe.num_rows() == 0))
    return;

  ////////////////////////////////////////////////////////////////////////////////

  // First find the column we need to join on.
  size_t side_join_column_index = size_t(-1);
  size_t main_join_column_index = size_t(-1);
  std::string join_column_name;

  if(!forced_join_column.empty()) {

    auto it = main_column_name_lookup.find(forced_join_column);

    if(it == main_column_name_lookup.end()) {
      log_and_throw(std::string("Join of side information requested on column ")
                    + forced_join_column + ", but this column is not present in the main data.");
    }

    main_join_column_index = it->second;

    if(!unindexed_side_sframe.contains_column(forced_join_column)) {
      log_and_throw(std::string("Join of side information requested on column ")
                    + forced_join_column + ", but this column is not present in the side data.");
    }

    join_column_name = forced_join_column;
    side_join_column_index = unindexed_side_sframe.column_index(join_column_name);

  } else {
    for(size_t i = 0; i < unindexed_side_sframe.num_columns(); ++i) {
      const std::string& column_name = unindexed_side_sframe.column_name(i);

      auto it = main_column_name_lookup.find(column_name);

      if(it != main_column_name_lookup.end()) {
        if(side_join_column_index != size_t(-1)) {
          log_and_throw(std::string("Join of side information attempted on both ")
                        + join_column_name + " and "
                        + column_name + "; joining must currently be on a single column.");
        }
        side_join_column_index = i;
        main_join_column_index = it->second;
        join_column_name = column_name;
      }
    }
  }

  if(side_join_column_index == size_t(-1))
    log_and_throw(std::string("No column found to join on. Exactly one column name "
                              "much match a column name in the main data to determine the join."));

  if(unindexed_side_sframe.num_columns() == 1) {
    logprogress_stream << "WARNING: No additional columns provided in side information for feature "
                       << join_column_name << "; ignoring." << std::endl;
    return;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Now, do we need a new schema for the side information

  column_side_info& si = side_lookups[main_join_column_index];

  std::vector<std::string> unjoined_names;

  if(training_mode) {

    // First test if we need to uniquify any of the column names.
    if(options.at("uniquify_side_column_names")) {
      uniquify_side_column_names(unindexed_side_sframe, si.column_name_map, join_column_name);
    }

    // Build up the list of side column names
    for(size_t i = 0; i < unindexed_side_sframe.num_columns(); ++i) {
      const std::string& column_name = unindexed_side_sframe.column_name(i);
      if(column_name != join_column_name)
        unjoined_names.push_back(column_name);
    }

    // Now construct the metadata
    std::vector<column_metadata_ptr> metadata_vect;

    for(size_t i = 0; i < unindexed_side_sframe.num_columns(); ++i) {
      if(i != side_join_column_index) {
        column_metadata_ptr cm(new column_metadata);
        cm->setup(
            false,
            unindexed_side_sframe.column_name(i),
            unindexed_side_sframe.select_column(i),
            mode_overrides,
            options);

        if(cm->is_untranslated_column())
          ASSERT_MSG(false, "Untranslated columns are not allowed in the side information.");

        metadata_vect.push_back(cm);
      }
    }

    si.rm.setup(metadata_vect, false);

    DASSERT_EQ(current_column_index, _full_metadata.size());
    _full_metadata.insert(_full_metadata.end(), si.rm.metadata_vect.begin(), si.rm.metadata_vect.end());

    si.column_index_start = current_column_index;
    current_column_index += si.rm.metadata_vect.size();

    // Zero out the maximum row size
    si.max_row_size = 0;

  } else {

    // Remap the column names if applicable.
    if(!si.column_name_map.empty()) {
      for(size_t i = 0; i < unindexed_side_sframe.num_columns(); ++i) {
        auto it = si.column_name_map.find(unindexed_side_sframe.column_name(i));

        if(it != si.column_name_map.end()) {
          DASSERT_TRUE(it->first != join_column_name);
          DASSERT_TRUE(it->second != join_column_name);

          unindexed_side_sframe.set_column_name(i, it->second);
        }
      }
    }

    const std::vector<std::string>& raw_column_names = unindexed_side_sframe.column_names();
    std::set<std::string> raw_column_name_set(raw_column_names.begin(), raw_column_names.end());

    std::vector<std::string> missing_columns;

    raw_column_name_set.erase(join_column_name);

    if(si.rm.metadata_vect.empty()) {
      bool ignore = options.at("ignore_new_columns_after_train");

      std::ostringstream ss;

      ss << "Side data provided on column '" << join_column_name
         << "'; no side data was provided at setup";

      if(ignore) {
        ss << "; Discarding.";
        logstream(LOG_WARNING) << ss.str() << std::endl;
        return;
      } else {
        ss << "." << std::endl;
        log_and_throw(ss.str());
      }
    }

    for(size_t c_idx = 0; c_idx < si.rm.metadata_vect.size(); ++c_idx) {
      const std::string& column_name = si.rm.metadata_vect[c_idx]->name;

      auto it = raw_column_name_set.find(column_name);

      if(it == raw_column_name_set.end())
        missing_columns.push_back(column_name);
      else
        raw_column_name_set.erase(it);
    }

    if(!missing_columns.empty()) {
      std::ostringstream ss;

      ss << "Provided data joined on " << join_column_name << " missing required columns: ";

      for(size_t i = 0; i < missing_columns.size() - 1; ++i) {
        ss << missing_columns[i] << ", ";
      }
      ss << missing_columns.back() << ".";

      log_and_throw(ss.str());
    }

    if(!raw_column_name_set.empty()) {
      std::ostringstream ss;

      bool ignore = options.at("ignore_new_columns_after_train");

      if(ignore) {
        ss << "Discarding additional columns present in side data on column "
           << join_column_name << " that do not match schema: ";
      } else {
        ss << "Additional columns present in side data on column "
           << join_column_name << " that do not match schema: ";
      }

      {
        size_t i = 0;
        for(const auto& s : raw_column_name_set) {
          ss << s;
          ++i;
          if(i != raw_column_name_set.size())
            ss << ", ";
        }
      }

      ss << ".";

      if(ignore) {
        logstream(LOG_WARNING) << ss.str() << std::endl;
      } else {
        log_and_throw(ss.str());
      }

      for(const std::string& c : raw_column_name_set) {
        unindexed_side_sframe = unindexed_side_sframe.remove_column(
            unindexed_side_sframe.column_index(c));
      }
    }

    // Build up the list of side column names
    for(size_t i = 0; i < unindexed_side_sframe.num_columns(); ++i) {
      const std::string& column_name = unindexed_side_sframe.column_name(i);
      if(column_name != join_column_name)
        unjoined_names.push_back(column_name);
    }

    training_mode = false;
  }

  bool track_statistics = training_mode ? true : false;
  missing_value_action mva = get_missing_value_action(options, training_mode);

  ////////////////////////////////////////////////////////////////////////////////

  // Get the indexed versions of the join column.  The other we'll
  // compute on the fly.
  column_metadata_ptr join_column_metadata = main_metadata[main_join_column_index];

  sframe side_data_sf = unindexed_side_sframe.select_columns(unjoined_names);

  sframe join_column_sf = map_to_indexed_sframe(
      {join_column_metadata->indexer},
      unindexed_side_sframe.select_columns({join_column_name}),
      !immutable_metadata);

  ////////////////////////////////////////////////////////////////////////////////
  // Make sure that the the current data_lookup_map is large enough to
  // hold the current number of categories.  If there are new values in
  // the side data, the above operations may have changed them.

  DASSERT_LE(si.data_lookup_map.size(), main_metadata[main_join_column_index]->column_size());
  si.data_lookup_map.resize(main_metadata[main_join_column_index]->column_size(), nullptr);

  ////////////////////////////////////////////////////////////////////////////////

  // Provide a safe way to get a new block of data.  We now have each
  // thread creating its own block of data.
  mutex new_data_aquire_lock;

  auto get_new_data_block = [this, &new_data_aquire_lock]() {
    std::shared_ptr<row_data_block> new_data_block_ptr(new row_data_block);

    new_data_aquire_lock.lock();
    raw_row_storage.push_back(new_data_block_ptr);
    new_data_aquire_lock.unlock();

    return new_data_block_ptr;
  };

  parallel_sframe_iterator_initializer join_column_it_init({join_column_sf, side_data_sf});

  ////////////////////////////////////////////////////////////////////////////////
  // Set up statistics tracking, if needed.

  scoped_finally indexer_finalizer;
  scoped_finally statistics_finalizer;

  // Initialize the metadata and key parts of ml_data
  //
  // The problem is that we always must make sure that things
  for(const auto& m : si.rm.metadata_vect) {
    std::shared_ptr<column_indexer> indexer = m->indexer;

    indexer->initialize();
    indexer_finalizer.add([indexer](){indexer->finalize();});

    if(track_statistics) {
      std::shared_ptr<column_statistics> statistics = m->statistics;

      statistics->initialize();
      statistics_finalizer.add([statistics](){statistics->finalize();});
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Track the maximum row size; simple front loading of things for
  // efficient allocation.
  size_t max_num_threads = thread::cpu_count();
  std::vector<size_t> max_row_size_by_thread(max_num_threads, 0);

  const size_t num_columns = side_data_sf.num_columns();

  in_parallel([&](size_t thread_idx, size_t num_threads) {
      // size_t thread_idx = 0; size_t num_threads = 1;  {

      // To fill these blocks of data, which is what we are doing
      // here, we sequentially grab blocks of 10000 rows of the data,
      // then keep track of how much is needed for the expanded size
      // while filling the buffer with the raw flexible_type objects.
      // We then allocate a new data block and fill it with the data
      // in the row buffer.
      //
      // The format is given by the internal format described in
      // ml_data_row_format.hpp.

      const size_t row_buffer_size = 10000;
      std::vector<size_t> join_index_buffer(row_buffer_size);
      std::vector<std::vector<flexible_type> > column_buffers(num_columns);
      std::vector<size_t> row2data_idx_map;

      parallel_sframe_iterator it(join_column_it_init, thread_idx, num_threads);

      // The main loop over all the data
      while(!it.done()) {

        ////////////////////////////////////////////////////////////////////////////////
        // Move everything from the side data block to the buffer

        size_t rows_in_buffer = 0;

        for(std::vector<flexible_type>& column : column_buffers)
          column.resize(row_buffer_size);

        for(size_t row_buffer_index = 0;
            row_buffer_index < row_buffer_size && !it.done();
            ++row_buffer_index, ++it) {

          // Save the index of the lookup location. it.value(0) is the
          // first column in parallel_sframe_iterator, which here is
          // the joined data column.  With the indexing provided by
          // column_metadata, this is the index.
          join_index_buffer[row_buffer_index] = it.value(0);

          for(size_t c_idx = 0; c_idx < num_columns; ++c_idx) {
            column_buffers[c_idx][row_buffer_index] = it.move_value(1, c_idx);
          }

          ++rows_in_buffer;
        }

        for(std::vector<flexible_type>& column : column_buffers)
          column.resize(rows_in_buffer);

        ////////////////////////////////////////////////////////////////////////////////
        // Put everything from the column_buffers into a block of raw data.

        std::shared_ptr<row_data_block> new_data = get_new_data_block();

        size_t max_row_size =
            fill_row_buffer_from_column_buffer(
                row2data_idx_map,
                *new_data,
                si.rm,
                column_buffers,
                thread_idx,
                track_statistics,
                immutable_metadata,
                mva);

        max_row_size_by_thread[thread_idx] = std::max(max_row_size_by_thread[thread_idx], max_row_size);

        ////////////////////////////////////////////////////////////////////////////////
        // Now go through and record the start of each row in the
        // location in data_lookup_map given by the join_index.  Also,
        // record join index.

        for(size_t i = 0; i < rows_in_buffer; ++i) {
          size_t idx = join_index_buffer[i];
          if(idx < si.data_lookup_map.size())
            si.data_lookup_map[idx] = new_data->entry_data.data() + row2data_idx_map[i];
        }

      }
    });

  si.max_row_size =
      std::max(si.max_row_size,
               *std::max_element(max_row_size_by_thread.begin(), max_row_size_by_thread.end()));

  // Finalize the lookups and statistics
  indexer_finalizer.execute_and_clear();
  if(track_statistics)
    statistics_finalizer.execute_and_clear();
}

  /** Uniquify the side column names.
   */
void ml_data_side_features::uniquify_side_column_names(
    sframe& side_sframe,
    std::map<std::string, std::string>& column_name_map,
    const std::string& join_name) const {

  std::vector<std::string> side_names = side_sframe.column_names();

  std::set<std::string> existing_columns;

  for(size_t i = 0; i < _full_metadata.size(); ++i)
    existing_columns.insert(_full_metadata[i]->name);

  TURI_ATTRIBUTE_UNUSED_NDEBUG bool join_column_detected = false;

  DASSERT_TRUE(column_name_map.empty());

  for(std::string& n : side_names) {

    if(n == join_name) {
      join_column_detected = true;
      continue;
    }

    if(existing_columns.count(n)) {

      auto make_name = [&](size_t i) { return n + "." + std::to_string(i); };

      // As long as it's in there, increment the count
      size_t idx = 1;
      while( existing_columns.count(make_name(idx)) ) { ++idx; }

      std::string new_name = make_name(idx);

      column_name_map[n] = new_name;

      n = new_name;
      existing_columns.insert(new_name);
    }
  }

  DASSERT_TRUE(join_column_detected);

  if(!column_name_map.empty()) {

    std::vector<std::shared_ptr<sarray<flexible_type> > > columns(side_sframe.num_columns());

    for(size_t i = 0; i < side_sframe.num_columns(); ++i) {
      columns[i] = side_sframe.select_column(i);
    }

    side_sframe = sframe(columns, side_names);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Returns the number of columns joined off of column
/// main_column_index in the main data.

size_t ml_data_side_features::num_columns(size_t main_column_index) const {

  DASSERT_LT(main_column_index, side_lookups.size());
  return side_lookups[main_column_index].rm.metadata_vect.size();
}

////////////////////////////////////////////////////////////////////////////////

/** This function is needed to remap things for the metadata
 *  select_columns function.  When selecting a subset of columns,
 *  this copies over the metadat in order to make it worthwhile.
 */
std::shared_ptr<ml_data_side_features> ml_data_side_features::copy_with_new_main_columns(
    const std::vector<ml_data_internal::column_metadata_ptr>& new_columns) const {

  // Create new side data with these columns
  auto ret = std::shared_ptr<ml_data_side_features>(new ml_data_side_features(new_columns));

  // Copy the raw storage over.
  ret->raw_row_storage = raw_row_storage;

  // Now, go through and set the proper columns.
  for(size_t i = 0; i < new_columns.size(); ++i) {
    const std::string& name = new_columns[i]->name;

    DASSERT_TRUE(main_column_name_lookup.count(name) != 0);

    size_t original_index = main_column_name_lookup.at(name);

    ret->side_lookups[i] = side_lookups[original_index];

    size_t column_range_lb = side_lookups[original_index].column_index_start;
    size_t column_range_ub = column_range_lb + side_lookups[original_index].rm.metadata_vect.size();

    size_t dest_lb = ret->_full_metadata.size();

    // Copy over the metadata so that we are in good shape
    ret->_full_metadata.insert(ret->_full_metadata.end(),
                               _full_metadata.begin() + column_range_lb,
                               _full_metadata.begin() + column_range_ub);

    ret->side_lookups[i].column_index_start = dest_lb;
  }

  // Set up the rest of the internal stuff over
  ret->current_column_index = ret->_full_metadata.size();

  // We're done!
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

/// Serialization -- save to an archive.  We assume that the main
/// metadata is already saved.  The side metadata is serialized.
void ml_data_side_features::save_without_metadata(turi::oarchive& oarc) const {

  size_t version = 1;
  oarc << version;

  std::map<std::string, flexible_type> data;

  data["main_metadata_size"] = main_metadata.size();

  // Need to know how big a buffer of side information we need to
  // prepare on deserialization -- this is where all the raw values
  // will go.

  size_t total_storage_space_needed = 0;
  for(const auto& raw_block : raw_row_storage)
    total_storage_space_needed += raw_block->entry_data.size();

  data["total_storage_space_needed"] = total_storage_space_needed;
  data["side_lookups_size"] = side_lookups.size();

  variant_deep_save(data, oarc);

  size_t items_used = 0;

  for(const column_side_info& csl : side_lookups) {

    std::map<std::string, variant_type> data;

    data["csl_column_index_start"]   = to_variant(csl.column_index_start);
    data["csl_data_lookup_map_size"] = to_variant(csl.data_lookup_map.size());
    data["csl_max_row_size"]         = to_variant(csl.max_row_size);
    data["csl_column_name_map"]      = to_variant(csl.column_name_map);

    variant_deep_save(data, oarc);

    oarc << csl.rm.metadata_vect;

    // Now the tricky part.  This only references raw data, so dump it
    // in the row with the lookup map.  This way, we can deserialize
    // it in order, setting up the lookup map pointer each time we
    // load a row.

    for(size_t i = 0; i < csl.data_lookup_map.size(); ++i) {

      // A verification number
      oarc << size_t(0x05322323UL);

      if(csl.data_lookup_map[i] == nullptr) {
        oarc << size_t(0);
      } else {

        // Record the size of the row
        size_t row_size = get_row_data_size(csl.rm, csl.data_lookup_map[i]);
        oarc << row_size;
        turi::serialize(oarc, csl.data_lookup_map[i], row_size * sizeof(entry_value));
        items_used += row_size;
        DASSERT_LE(items_used, total_storage_space_needed);
      }
    }
  }

  oarc << current_column_index;

  // Dump out a random number for error checking purposes
  oarc << size_t(0x0F0F0F0F);
}

/// Serialization -- load from an archive.  We assume that the main
/// metadata is already present.  The side metadata is deserialized.
void ml_data_side_features::load_with_metadata_present(turi::iarchive& iarc) {

  size_t version;

  iarc >> version;

  ASSERT_MSG(version == 1, "Loading this version not implmented.");

  std::map<std::string, flexible_type> data;
  variant_deep_load(data, iarc);

  size_t main_metadata_size = variant_get_value<size_t>(data.at("main_metadata_size"));

  if(main_metadata_size != main_metadata.size())
    log_and_throw("ERROR: metadata missmatch in side feature deserialization.");

  // Prepare our buffer for dumping the raw data
  size_t total_storage_space_needed = variant_get_value<size_t>(data.at("total_storage_space_needed"));

  std::shared_ptr<row_data_block> raw_data(new row_data_block);
  raw_data->entry_data.resize(total_storage_space_needed);

  size_t side_lookups_size = variant_get_value<size_t>(data.at("side_lookups_size"));
  side_lookups.resize(side_lookups_size);

  size_t raw_data_pos = 0;

  for(column_side_info& csl : side_lookups) {

    std::map<std::string, variant_type> csl_data;

    variant_deep_load(csl_data, iarc);

#define __EXTRACT(n) csl.n = variant_get_value<decltype(csl.n)>(csl_data.at("csl_"#n))

    __EXTRACT(column_index_start);
    __EXTRACT(max_row_size);
    __EXTRACT(column_name_map);

#undef __EXTRACT

    std::vector<column_metadata_ptr> mv;

    iarc >> mv;

    csl.rm.setup(mv, false);

    // Add the metadata to the global index
    if(csl.column_index_start != 0) // Indicates it's an unused side column
      DASSERT_EQ(_full_metadata.size(), csl.column_index_start);

    _full_metadata.insert(_full_metadata.end(), csl.rm.metadata_vect.begin(), csl.rm.metadata_vect.end());

    size_t data_map_size = variant_get_value<size_t>(csl_data["csl_data_lookup_map_size"]);
    csl.data_lookup_map.assign(data_map_size, nullptr);

    for(size_t i = 0; i < csl.data_lookup_map.size(); ++i) {
      size_t verification_number;

      // A verification number
      iarc >> verification_number;
      DASSERT_EQ(verification_number, size_t(0x05322323UL));

      size_t row_size;
      iarc >> row_size;

      if(row_size != 0) {

        DASSERT_LE(raw_data_pos + row_size, raw_data->entry_data.size());
        entry_value* write_location = &(raw_data->entry_data[raw_data_pos]);
        csl.data_lookup_map[i] = write_location;
        turi::deserialize(iarc, write_location, row_size * sizeof(entry_value));

        raw_data_pos += row_size;
      }
    }
  }

  iarc >> current_column_index;

  size_t check_number;
  iarc >> check_number;
  if(check_number != 0x0F0F0F0F)
    log_and_throw("Deserialization error loading side data class.");

  raw_row_storage = {raw_data};
}

}}
